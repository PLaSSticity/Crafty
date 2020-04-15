#include "tpcctables.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <vector>

#include "tpcc_assert.h"
#include "stlutil.h"

using std::vector;

bool CustomerByNameOrdering::operator()(const Customer* a, const Customer* b) {
    if (a->c_w_id < b->c_w_id) return true;
    if (a->c_w_id > b->c_w_id) return false;
    assert(a->c_w_id == b->c_w_id);

    if (a->c_d_id < b->c_d_id) return true;
    if (a->c_d_id > b->c_d_id) return false;
    assert(a->c_d_id == b->c_d_id);

    int diff = strcmp(a->c_last, b->c_last);
    if (diff < 0) return true;
    if (diff > 0) return false;
    assert(diff == 0);

    // Finally delegate to c_first
    return strcmp(a->c_first, b->c_first) < 0;
}

TPCCTables::~TPCCTables() {
}

__attribute__((transaction_safe)) char* tm_strncpy(char *dst, const char *src, size_t n)
{
    if (n != 0) {
        char *d = dst;
        const char *s = src;
        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n) {
                    *d++ = 0;
                }
                break;
            }
        } while (--n);
    }
    return dst;
}

__attribute__((transaction_safe)) char* tm_strstr(char *s2, char *s1) {
  int i, j;
  int flag = 0;

  if ((s2 == NULL || s1 == NULL)) return NULL;

  for( i = 0; s2[i] != '\0'; i++) {
    if (s2[i] == s1[0]) {
      for (j = i; ; j++) {
        if (s1[j-i] == '\0'){ flag = 1; break;}
        if (s2[j] == s1[j-i]) continue;
        else break;
      }
    }
    if (flag == 1) break;
  }

  if (flag) return (s2+i);
  else return NULL;
}

__attribute__((transaction_safe)) void * tm_memcpy(void *dst, const void *src, size_t len)
{
    size_t i;
    if ((uintptr_t)dst % sizeof(long) == 0 &&
        (uintptr_t)src % sizeof(long) == 0 &&
        len % sizeof(long) == 0)
    {
        long *d = (long*)dst;
        const long *s = (const long*)src;
        for (i=0; i<len/sizeof(long); i++) {
            d[i] = s[i];
        }
    }
    else {
        char *d = (char*)dst;
        const char *s = (const char*)src;
        for (i=0; i<len; i++) {
            d[i] = s[i];
        }
    }
    return dst;
}

int64_t TPCCTables::stockLevel(int64_t warehouse_id, int64_t district_id, int64_t threshold) {
    /* EXEC SQL SELECT d_next_o_id INTO :o_id FROM district
        WHERE d_w_id=:w_id AND d_id=:d_id; */
    //~ printf("stock level %d %d %d\n", warehouse_id, district_id, threshold);

    /* EXEC SQL SELECT COUNT(DISTINCT (s_i_id)) INTO :stock_count FROM order_line, stock
        WHERE ol_w_id=:w_id AND ol_d_id=:d_id AND ol_o_id<:o_id AND ol_o_id>=:o_id-20
            AND s_w_id=:w_id AND s_i_id=ol_i_id AND s_quantity < :threshold;*/


    // retrieve up to 300 tuples from order line, using ( [o_id-20, o_id), d_id, w_id, [1, 15])
    //   and for each retrieved tuple, read the corresponding stock tuple using (ol_i_id, w_id)
    // NOTE: This is a cheat because it hard codes the maximum number of orders.
    // We really should use the ordered b-tree index to find (0, o_id-20, d_id, w_id) then iterate
    // until the end. This will also do less work (wasted finds). Since this is only 4%, it probably
    // doesn't matter much

    // TODO: Test the performance more carefully. I tried: std::set, std::hash_set, std::vector
    // with linear search, and std::vector with binary search using std::lower_bound. The best
    // seemed to be to simply save all the s_i_ids, then sort and eliminate duplicates at the end.
    //int64_t s_i_ids[10000];
	long s_i_ids[100000];
    // Average size is more like ~30.
    uint64_t counter_s = 0; int x;
/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
    int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 1;
    TM_BEGIN();
        District* d = findDistrict(warehouse_id, district_id);
	//printf("address of d is %p\n",d);
        int64_t o_id = FAST_PATH_SHARED_READ(d->d_next_o_id);
        // Iterate over [o_id-20, o_id)
        int64_t stock_quantity;
	Stock * stock;
        for (int64_t order_id = o_id - STOCK_LEVEL_ORDERS; order_id < o_id; ++order_id) {
            for (int line_number = 1; line_number <= Order::MAX_OL_CNT; ++line_number) {
                OrderLine* line = findOrderLine(warehouse_id, district_id, order_id, line_number);
		line = (OrderLine*)FAST_PATH_SHARED_READ_P(line);
                if (line == NULL) {
                    // We can break since we have reached the end of the lines for this order.
                    // TODO: A btree iterate in (w_id, d_id, o_id) order would be a clean way to do this
                    break;
                }
                // Check if s_quantity < threshold
                stock = findStock(warehouse_id, line->ol_i_id);
		stock_quantity = FAST_PATH_SHARED_READ(stock->s_quantity);
		//stock_quantity = stock->s_quantity;
                if (stock_quantity < threshold) {
                    s_i_ids[counter_s] = line->ol_i_id;
                    counter_s++;
			//printf("%d\n",counter_s);
                }
            }
        }
	if(stock_quantity>0)
		stock->s_w_id = 1;
     TM_END();
	/*if(stock_quantity>0)
		counter_s++;
	else
		counter_s--;*/
	//printf("%d\n",temp);
/*    } */

    // Filter out duplicate s_i_id: multiple order lines can have the same item
    /*std::sort(s_i_ids.begin(), s_i_ids.end());
    int num_distinct = 0;
    int64_t last = -1;  // NOTE: This relies on -1 being an invalid s_i_id
    for (size_t i = 0; i < s_i_ids.size(); ++i) {
        if (s_i_ids[i] != last) {
            last = s_i_ids[i];
            num_distinct += 1;
        }
    }*/

    return 0;
}

void TPCCTables::orderStatus(int64_t warehouse_id, int64_t district_id, int64_t customer_id, OrderStatusOutput* output) {
  //~ printf("order status %d %d %d\n", warehouse_id, district_id, customer_id);
    //__transaction_atomic {
    // FIXME(nmld): transaction block here
    int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 1;
    TM_BEGIN();
        internalOrderStatus(findCustomer(warehouse_id, district_id, customer_id), output);
    TM_END();
    //}
}

void TPCCTables::orderStatus(int64_t warehouse_id, int64_t district_id, const char* c_last, OrderStatusOutput* output) {
    //~ printf("order status %d %d %s\n", warehouse_id, district_id, c_last);
    int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 1;
    TM_BEGIN();
    Customer* customer = findCustomerByName(warehouse_id, district_id, c_last);
    internalOrderStatus(customer, output);
    TM_END();
}

__attribute__((transaction_safe)) void TPCCTables::internalOrderStatus(Customer* customer, OrderStatusOutput* output) {
    output->c_id = customer->c_id;
    // retrieve from customer: balance, first, middle, last
    output->c_balance = FAST_PATH_SHARED_READ_D(customer->c_balance);

    // Find the row in the order table with largest o_id
    Order* order = findLastOrderByCustomer(customer->c_w_id, customer->c_d_id, customer->c_id);
    output->o_id = order->o_id;
    output->o_carrier_id = FAST_PATH_SHARED_READ(order->o_carrier_id);

    for (int64_t line_number = 1; line_number <= order->o_ol_cnt; ++line_number) {
        OrderLine* line = findOrderLine(customer->c_w_id, customer->c_d_id, order->o_id, line_number);
	//line = (OrderLine*)FAST_PATH_SHARED_READ_P(line);
	if(line==NULL)
		break;
	 if (line->ol_amount < 0.0) {
		customer->c_credit_lim = line->ol_amount;
	 }
    }
}

bool TPCCTables::newOrder(int64_t warehouse_id, int64_t district_id, int64_t customer_id,
        const std::vector<NewOrderItem>& items, const char* now, NewOrderOutput* output,
        TPCCUndo** undo) {
    uint64_t items_size = items.size();
    int64_t* warehouse_set = (int64_t*) malloc(items_size * sizeof(int64_t));
    Item** item_tuples = (Item**) malloc(items_size * sizeof(Item*));

/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
        // perform the home part
       int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 0;
       TM_BEGIN();
        bool result = newOrderHome(warehouse_id, district_id, customer_id, items, now, output, undo, item_tuples);
        if (!result) {
	    TM_END();
            return false;
        }
        // Process all remote warehouses
        for (size_t i = 0; i < items_size; ++i) {
            if (items[i].ol_supply_w_id != warehouse_id) {
                warehouse_set[i] = items[i].ol_supply_w_id;
            }
        }

        for (size_t i = 0; i < items_size; ++i) {
            result = newOrderRemote(warehouse_id, warehouse_set[i], items, NULL, undo, item_tuples);
        }
      TM_END();
/*    } */

    free(warehouse_set);
    return true;
}

template <typename T>
__attribute__((transaction_safe)) inline uint64_t getVectorSize(const vector<T>& items) {
    return items.size();
}

template<typename T>
__attribute__((transaction_pure)) inline const T& getVectorItem(const vector<T>& items, uint64_t pos) {
    return items[pos];
}

__attribute__((transaction_safe)) bool TPCCTables::newOrderHome(int64_t warehouse_id, int64_t district_id, int64_t customer_id,
        const vector<NewOrderItem>& items, const char* now,
        NewOrderOutput* output, TPCCUndo** undo, Item** item_tuples) {
    //~ printf("new order %d %d %d %d %s\n", warehouse_id, district_id, customer_id, items.size(), now);
    // 2.4.3.4. requires that we display c_last, c_credit, and o_id for rolled back transactions:
    // read those values first
    District* d = findDistrict(warehouse_id, district_id);
    output->d_tax = d->d_tax;
    	output->o_id = FAST_PATH_SHARED_READ(d->d_next_o_id);

    Customer* c = findCustomer(warehouse_id, district_id, customer_id);
    output->c_discount = c->c_discount;

    // CHEAT: Validate all items to see if we will need to abort
    uint64_t vec_size = getVectorSize(items);
    if (!findAndValidateItems(items, item_tuples)) {
        return false;
    }


    // Check if this is an all local transaction
    // TODO: This loops through items *again* which is slightly inefficient
    bool all_local = true;
    for (int i = 0; i < vec_size; ++i) {
        if (getVectorItem(items, i).ol_supply_w_id != warehouse_id) {
            all_local = false;
            break;
        }
    }

    // We will not abort: update the status and the database state, allocate an undo buffer
    output->status[0] = '\0';

    // Modify the order id to assign it
    FAST_PATH_SHARED_WRITE(d->d_next_o_id, d->d_next_o_id + 1);

    Warehouse* w = findWarehouse(warehouse_id);
    output->w_tax = w->w_tax;

    Order order;
    order.o_w_id = warehouse_id;
    order.o_d_id = district_id;
    order.o_id = output->o_id;
    order.o_c_id = customer_id;
    order.o_carrier_id = Order::NULL_CARRIER_ID;
    order.o_ol_cnt = static_cast<int64_t>(items.size());
    order.o_all_local = all_local ? 1 : 0;
    tm_strncpy(order.o_entry_d, now, Clock::DATETIME_SIZE+1);
    Order* o = insertOrder(order);
    NewOrder* no = insertNewOrder(warehouse_id, district_id, output->o_id);

    OrderLine line;
    line.ol_o_id = output->o_id;
    line.ol_d_id = district_id;
    line.ol_w_id = warehouse_id;
    int idx;
    for (idx = 0; idx < DATETIME_SIZE+1; idx++) {
        line.ol_delivery_d[idx] = 0;
    }

    output->total = 0;
    for (int i = 0; i < items.size(); ++i) {
        line.ol_number = i+1;
        line.ol_i_id = items[i].i_id;
        line.ol_supply_w_id = items[i].ol_supply_w_id;
        line.ol_quantity = items[i].ol_quantity;

        // Vertical Partitioning HACK: We read s_dist_xx from our local replica, assuming that
        // these columns are replicated everywhere.
        // TODO: I think this is unrealistic, since it will occupy ~23 MB per warehouse on all
        // replicas. Try the "two round" version in the future.
        Stock* stock = findStock(items[i].ol_supply_w_id, items[i].i_id);
        int c;
        for (c = 0; c < Stock::DIST+1; c++) {
            line.ol_dist_info[c] = stock->s_dist[district_id][c];
        }
        // Since we *need* to replicate s_dist_xx columns, might as well replicate s_data
        // Makes it 290 bytes per tuple, or ~28 MB per warehouse.
        bool stock_is_original = (tm_strstr(stock->s_data, (char*)"ORIGINAL") != NULL);
        if (stock_is_original && tm_strstr(item_tuples[i]->i_data, (char*)"ORIGINAL") != NULL) {
        } else {
        }

        OrderLine* ol = insertOrderLine(line);
    }

    // Perform the "remote" part for this warehouse
    // TODO: It might be more efficient to merge this into the loop above, but this is simpler.
    bool result = newOrderRemote(warehouse_id, warehouse_id, items, NULL, undo, item_tuples);

    return true;
}

__attribute__((transaction_safe)) bool TPCCTables::newOrderRemote(int64_t home_warehouse, int64_t remote_warehouse,
        const vector<NewOrderItem>& items, std::vector<int64_t>* out_quantities, TPCCUndo** undo, Item** item_tuples) {
    // Validate all the items: needed so that we don't need to undo in order to execute this
    // TODO: item_tuples is unused. Remove?
    uint64_t vec_size = getVectorSize(items);
    if (!findAndValidateItems(items, item_tuples)) {
        return false;
    }

    for (int i = 0; i < getVectorSize(items); ++i) {
        const NewOrderItem& item = getVectorItem(items, i);

        // Skip items that don't belong to remote warehouse
        if (item.ol_supply_w_id != remote_warehouse) {
            continue;
        }

        // update stock
        Stock* stock = findStock(item.ol_supply_w_id, item.i_id);
	int64_t stock_quantity;
		stock_quantity = FAST_PATH_SHARED_READ(stock->s_quantity);
        if (stock_quantity >= item.ol_quantity + 10) {
	    FAST_PATH_SHARED_WRITE(stock->s_quantity,stock_quantity-item.ol_quantity);
        } else {
	    FAST_PATH_SHARED_WRITE(stock->s_quantity,stock_quantity-item.ol_quantity+91);
        }
	FAST_PATH_SHARED_WRITE(stock->s_ytd,stock->s_ytd+item.ol_quantity);
	FAST_PATH_SHARED_WRITE(stock->s_order_cnt,stock->s_order_cnt+1);
        // newOrderHome calls newOrderRemote, so this is needed
        if (item.ol_supply_w_id != home_warehouse) {
            // remote order
            stock->s_remote_cnt += 1;
        }
    }

    return true;
}

__attribute__((transaction_safe)) bool TPCCTables::findAndValidateItems(const vector<NewOrderItem>& items,
        Item** item_tuples) {
    // CHEAT: Validate all items to see if we will need to abort
    for (int i = 0; i < getVectorSize(items); ++i) {
        Item* item = findItem(getVectorItem(items, i).i_id);
        item_tuples[i] = item;
        if (item == NULL) {
            return false;
        }
    }
    return true;
}


void TPCCTables::payment(int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t customer_id, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
    //~ printf("payment %d %d %d %d %d %f %s\n", warehouse_id, district_id, c_warehouse_id, c_district_id, customer_id, h_amount, now);
/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
    int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 0;
    TM_BEGIN();
        Customer* customer = findCustomer(c_warehouse_id, c_district_id, customer_id);
        paymentHome(warehouse_id, district_id, c_warehouse_id, c_district_id, customer_id, h_amount,
                now, output, undo);
        internalPaymentRemote(warehouse_id, district_id, customer, h_amount, output, undo);
     TM_END();
/*    } */
}

void TPCCTables::payment(int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, const char* c_last, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
    //~ printf("payment %d %d %d %d %s %f %s\n", warehouse_id, district_id, c_warehouse_id, c_district_id, c_last, h_amount, now);
    Customer* customer = findCustomerByName(c_warehouse_id, c_district_id, c_last);
    paymentHome(warehouse_id, district_id, c_warehouse_id, c_district_id, customer->c_id, h_amount,
            now, output, undo);
    internalPaymentRemote(warehouse_id, district_id, customer, h_amount, output, undo);
}

#define COPY_ADDRESS(src, dest, prefix) \
    Address::copy( \
            dest->prefix ## street_1, dest->prefix ## street_2, dest->prefix ## city, \
            dest->prefix ## state, dest->prefix ## zip,\
            src->prefix ## street_1, src->prefix ## street_2, src->prefix ## city, \
            src->prefix ## state, src->prefix ## zip)

#define ZERO_ADDRESS(output, prefix) \
    output->prefix ## street_1[0] = '\0'; \
    output->prefix ## street_2[0] = '\0'; \
    output->prefix ## city[0] = '\0'; \
    output->prefix ## state[0] = '\0'; \
    output->prefix ## zip[0] = '\0'

__attribute__((transaction_safe)) static void zeroWarehouseDistrict(PaymentOutput* output) {
    // Zero the warehouse and district data
    // TODO: I should split this structure, but I'm lazy
    ZERO_ADDRESS(output, w_);
    ZERO_ADDRESS(output, d_);
}

void TPCCTables::paymentRemote(int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t c_id, float h_amount, PaymentOutput* output,
        TPCCUndo** undo) {
    Customer* customer = findCustomer(c_warehouse_id, c_district_id, c_id);
    internalPaymentRemote(warehouse_id, district_id, customer, h_amount, output, undo);
    zeroWarehouseDistrict(output);
}
void TPCCTables::paymentRemote(int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, const char* c_last, float h_amount, PaymentOutput* output,
        TPCCUndo** undo) {
    Customer* customer = findCustomerByName(c_warehouse_id, c_district_id, c_last);
    internalPaymentRemote(warehouse_id, district_id, customer, h_amount, output, undo);
    zeroWarehouseDistrict(output);
}

__attribute__((transaction_safe)) void TPCCTables::paymentHome(int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t customer_id, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
    Warehouse* w = findWarehouse(warehouse_id);
    FAST_PATH_SHARED_WRITE(w->w_ytd,w->w_ytd+h_amount);

    District* d = findDistrict(warehouse_id, district_id);
    FAST_PATH_SHARED_WRITE(d->d_ytd,d->d_ytd+h_amount);

    // Insert the line into the history table
    History h;
    h.h_w_id = warehouse_id;
    h.h_d_id = district_id;
    h.h_c_w_id = c_warehouse_id;
    h.h_c_d_id = c_district_id;
    h.h_c_id = customer_id;
    h.h_amount = h_amount;

    // Zero all the customer fields: avoid uninitialized data for serialization
    output->c_credit_lim = 0;
    output->c_discount = 0;
    output->c_balance = 0;
    output->c_first[0] = '\0';
    output->c_middle[0] = '\0';
    output->c_last[0] = '\0';
    ZERO_ADDRESS(output, c_);
    output->c_phone[0] = '\0';
    output->c_since[0] = '\0';
    output->c_credit[0] = '\0';
    output->c_data[0] = '\0';
}

__attribute__((transaction_safe)) void TPCCTables::internalPaymentRemote(int64_t warehouse_id, int64_t district_id, Customer* c,
        float h_amount, PaymentOutput* output, TPCCUndo** undo) {
    FAST_PATH_SHARED_WRITE(c->c_balance,c->c_balance-h_amount);
    FAST_PATH_SHARED_WRITE(c->c_ytd_payment,c->c_ytd_payment+h_amount);
    FAST_PATH_SHARED_WRITE(c->c_payment_cnt,c->c_payment_cnt+1);

    output->c_credit_lim = c->c_credit_lim;
    output->c_discount = c->c_discount;
    output->c_balance = c->c_balance;
}

#undef ZERO_ADDRESS
#undef COPY_ADDRESS

// forward declaration for delivery
static int64_t makeNewOrderKey(int64_t w_id, int64_t d_id, int64_t o_id);

void TPCCTables::delivery(int64_t warehouse_id, int64_t carrier_id, const char* now,
        std::vector<DeliveryOrderInfo>* orders, TPCCUndo** undo) {
    //~ printf("delivery %d %d %s\n", warehouse_id, carrier_id, now);

/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
    int tle_budget; int htm_budget; int rot_budget; int exec_mode; int ro = 0;
    TM_BEGIN();
        for (int64_t d_id = 1; d_id <= District::NUM_PER_WAREHOUSE; ++d_id) {
            // Find and remove the lowest numbered order for the district

            int64_t key = makeNewOrderKey(warehouse_id, d_id, 1);
            NewOrder* neworder;
            int64_t foundKey = -1;
            if (!neworders_.findLastLessThan_tm(key, &neworder, &foundKey)) {
                neworder = NULL;
            }

            if (neworder == NULL || neworder->no_d_id != d_id || neworder->no_w_id != warehouse_id) {
                // No orders for this district
                // TODO: 2.7.4.2: If this occurs in max(1%, 1) of transactions, report it (???)
                continue;
            }
            int64_t o_id = neworder->no_o_id;

            neworders_.del_tm(foundKey);
            // FIXME: delete neworder

            Order* o = findOrder(warehouse_id, d_id, o_id);
	    FAST_PATH_SHARED_WRITE(o->o_carrier_id,carrier_id);

            float total = 0;
            // TODO: Select based on (w_id, d_id, o_id) rather than using ol_number?
            for (int64_t i = 1; i <= o->o_ol_cnt; ++i) {
                OrderLine* line = findOrderLine(warehouse_id, d_id, o_id, i);
		char date_now[DATETIME_SIZE+1];
                tm_strncpy(date_now, now, Clock::DATETIME_SIZE);
                total += line->ol_amount;
            }

            Customer* c = findCustomer(warehouse_id, d_id, o->o_c_id);
	    FAST_PATH_SHARED_WRITE(c->c_balance,c->c_balance+total);
	    FAST_PATH_SHARED_WRITE(c->c_delivery_cnt,c->c_delivery_cnt+1);
    }
    TM_END();
/*    } */
}

template <typename T>
static void restoreFromMap(const T& map) {
    for (typename T::const_iterator i = map.begin(); i != map.end(); ++i) {
        // Copy the original data back
        *(i->first) = *(i->second);
    }
}

template <typename T>
static void eraseTuple(const T& set, TPCCTables* tables,
        void (TPCCTables::*eraseFPtr)(typename T::value_type)) {
    for (typename T::const_iterator i = set.begin(); i != set.end(); ++i) {
        // Invoke eraseFPtr on each value
        (tables->*eraseFPtr)(*i);
    }
}


/*template <typename T>
__attribute__((transaction_safe)) static T* insert(BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree, int64_t key, const T& item) {
    T* copy = (T*) malloc(sizeof(T));
    tm_memcpy(copy, &item, sizeof(T));
    tree->insert(key, copy);
    return copy;
}*/

template <typename T>
__attribute__((transaction_safe)) static T* insert_tm(BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree, int64_t key, const T& item) {
    T* copy = (T*) malloc(sizeof(T));
    tm_memcpy(copy, &item, sizeof(T));
    tree->insert_tm(key, copy);
    return copy;
}

/*template <typename T>
__attribute__((transaction_safe))  static T* find(const BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>& tree, int64_t key) {
    T* output = NULL;
    if (tree.find(key, &output)) {
        return output;
    }
    return NULL;
}*/

template <typename T>
__attribute__((transaction_safe))  static T* find_tm(const BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>& tree, int64_t key) {
    T* output = NULL;
     //printf("looking fo %lu, %d\n",key,tree.find_tm(key, &output));
    if (tree.find_tm(key, &output)) {
        return output;
    }
    return NULL;
}

template <typename T, typename KeyType>
static void erase(BPlusTree<KeyType, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree,
        KeyType key, const T* value) {
    T* out = NULL;
    ASSERT(tree->find(key, &out));
    ASSERT(out == value);
    bool result = tree->del(key);
    ASSERT(result);
    ASSERT(!tree->find(key));
}

template <typename T, typename KeyType>
static void erase_tm(BPlusTree<KeyType, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree,
        KeyType key, const T* value) {
    T* out = NULL;
    ASSERT(tree->find(key, &out));
    ASSERT(out == value);
    bool result = tree->del(key);
    ASSERT(result);
    ASSERT(!tree->find(key));
}

void TPCCTables::insertItem(Item* item) {
    items_.insert_tm(item->i_id, item);
}
__attribute__((transaction_safe)) Item* TPCCTables::findItem(int64_t id) {
    return find_tm(items_, id);
}

void TPCCTables::insertWarehouse(const Warehouse& w) {
    insert_tm(&warehouses_, w.w_id, w);
}
__attribute__((transaction_safe)) Warehouse* TPCCTables::findWarehouse(int64_t id) {
    return find_tm(warehouses_, id);
}

__attribute__((transaction_safe)) static int64_t makeStockKey(int64_t w_id, int64_t s_id) {
    return s_id + (w_id * Stock::NUM_STOCK_PER_WAREHOUSE);
}

void TPCCTables::insertStock(const Stock& stock) {
    insert_tm(&stock_, makeStockKey(stock.s_w_id, stock.s_i_id), stock);
}
Stock* TPCCTables::findStock(int64_t w_id, int64_t s_id) {
    return find_tm(stock_, makeStockKey(w_id, s_id));
}

__attribute__((transaction_safe))  static int64_t makeDistrictKey(int64_t w_id, int64_t d_id) {
    return d_id + (w_id * District::NUM_PER_WAREHOUSE);
}

void TPCCTables::insertDistrict(const District& district) {
    insert_tm(&districts_, makeDistrictKey(district.d_w_id, district.d_id), district);
}

__attribute__((transaction_safe)) District* TPCCTables::findDistrict(int64_t w_id, int64_t d_id) {
    return find_tm(districts_, makeDistrictKey(w_id, d_id));
}

__attribute__((transaction_safe)) static int64_t makeCustomerKey(int64_t w_id, int64_t d_id, int64_t c_id) {
    return (w_id * District::NUM_PER_WAREHOUSE + d_id)
            * Customer::NUM_PER_DISTRICT + c_id;
}

void TPCCTables::insertCustomer(const Customer& customer) {
    Customer* c = insert_tm(&customers_, makeCustomerKey(customer.c_w_id, customer.c_d_id, customer.c_id), customer);
    assert(customers_by_name_.find(c) == customers_by_name_.end());
    customers_by_name_.insert(c);
}
__attribute__((transaction_safe)) Customer* TPCCTables::findCustomer(int64_t w_id, int64_t d_id, int64_t c_id) {
    return find_tm(customers_, makeCustomerKey(w_id, d_id, c_id));
}

Customer* TPCCTables::findCustomerByName(int64_t w_id, int64_t d_id, const char* c_last) {
    // select (w_id, d_id, *, c_last) order by c_first
    Customer c;
    c.c_w_id = w_id;
    c.c_d_id = d_id;
    strcpy(c.c_last, c_last);
    c.c_first[0] = '\0';
    CustomerByNameSet::const_iterator it = customers_by_name_.lower_bound(&c);

    // go to the "next" c_last
    // TODO: This is a GROSS hack. Can we do better?
    int length = static_cast<int>(strlen(c_last));
    if (length == Customer::MAX_LAST) {
        c.c_last[length-1] = static_cast<char>(c.c_last[length-1] + 1);
    } else {
        c.c_last[length] = 'A';
        c.c_last[length+1] = '\0';
    }
    CustomerByNameSet::const_iterator stop = customers_by_name_.lower_bound(&c);

    Customer* customer = NULL;
    // Choose position n/2 rounded up (1 based addressing) = floor((n-1)/2)
    if (it != stop) {
        CustomerByNameSet::const_iterator middle = it;
        ++it;
        int i = 0;
        while (it != stop) {
            // Increment the middle iterator on every second iteration
            if (i % 2 == 1) {
                ++middle;
            }
            ++it;
            ++i;
        }
        // There were i+1 matching last names
        customer = *middle;
    }

    return customer;
}

__attribute__((transaction_safe)) static int64_t makeOrderKey(int64_t w_id, int64_t d_id, int64_t o_id) {
    // TODO: This is bad for locality since o_id is in the most significant position. Larger keys?
    return (o_id * District::NUM_PER_WAREHOUSE + d_id)
            * Warehouse::MAX_WAREHOUSE_ID + w_id;
}

__attribute__((transaction_safe)) static int64_t makeOrderByCustomerKey(int64_t w_id, int64_t d_id, int64_t c_id, int64_t o_id) {
    int64_t top_id = (w_id * District::NUM_PER_WAREHOUSE + d_id) * Customer::NUM_PER_DISTRICT
            + c_id;
    return (((int64_t) top_id) << 32) | o_id;
}

/*__attribute__((transaction_safe)) Order* TPCCTables::insertOrder(const Order& order) {
    Order* tuple = insert(&orders_, makeOrderKey(order.o_w_id, order.o_d_id, order.o_id), order);
    // Secondary index based on customer id
    int64_t key = makeOrderByCustomerKey(order.o_w_id, order.o_d_id, order.o_c_id, order.o_id);
    orders_by_customer_.insert(key, tuple);
    return tuple;
}*/


__attribute__((transaction_safe)) Order* TPCCTables::insertOrder(const Order& order) {
    Order* tuple = insert_tm(&orders_, makeOrderKey(order.o_w_id, order.o_d_id, order.o_id), order);
    // Secondary index based on customer id
    int64_t key = makeOrderByCustomerKey(order.o_w_id, order.o_d_id, order.o_c_id, order.o_id);
    orders_by_customer_.insert_tm(key, tuple);
    return tuple;
}


__attribute__((transaction_safe)) Order* TPCCTables::findOrder(int64_t w_id, int64_t d_id, int64_t o_id) {
    return find_tm(orders_, makeOrderKey(w_id, d_id, o_id));
}

__attribute__((transaction_safe)) Order* TPCCTables::findLastOrderByCustomer(const int64_t w_id, const int64_t d_id, const int64_t c_id) {
    Order* order = NULL;

    // Increment the (w_id, d_id, c_id) tuple
    int64_t key = makeOrderByCustomerKey(w_id, d_id, c_id, 1);
    key += ((int64_t)1) << 32;

    bool found = orders_by_customer_.findLastLessThan_tm(key, &order);
    return order;
}

__attribute__((transaction_safe)) static int64_t makeOrderLineKey(int64_t w_id, int64_t d_id, int64_t o_id, int64_t number) {
    // TODO: This may be bad for locality since o_id is in the most significant position. However,
    // Order status fetches all rows for one (w_id, d_id, o_id) tuple, so it may be fine,
    // but stock level fetches order lines for a range of (w_id, d_id, o_id) values
    return ((o_id * District::NUM_PER_WAREHOUSE + d_id)
            * Warehouse::MAX_WAREHOUSE_ID + w_id) * Order::MAX_OL_CNT + number;
}

/*OrderLine* TPCCTables::insertOrderLine(const OrderLine& orderline) {
    int64_t key = makeOrderLineKey(
            orderline.ol_w_id, orderline.ol_d_id, orderline.ol_o_id, orderline.ol_number);
    return insert(&orderlines_, key, orderline);
}*/

OrderLine* TPCCTables::insertOrderLine(const OrderLine& orderline) {
    int64_t key = makeOrderLineKey(
            orderline.ol_w_id, orderline.ol_d_id, orderline.ol_o_id, orderline.ol_number);
    return insert_tm(&orderlines_, key, orderline);
}

__attribute__((transaction_safe)) OrderLine* TPCCTables::findOrderLine(int64_t w_id, int64_t d_id, int64_t o_id, int64_t number) {
    return find_tm(orderlines_, makeOrderLineKey(w_id, d_id, o_id, number));
}

__attribute__((transaction_safe)) static int64_t makeNewOrderKey(int64_t w_id, int64_t d_id, int64_t o_id) {
    int64_t upper_id = w_id * Warehouse::MAX_WAREHOUSE_ID + d_id;
    int64_t id = static_cast<int64_t>(upper_id) << 32 | o_id;
    return id;
}

/*__attribute__((transaction_safe)) NewOrder* TPCCTables::insertNewOrder(int64_t w_id, int64_t d_id, int64_t o_id) {
    NewOrder* neworder = (NewOrder*) malloc(sizeof(NewOrder));
    neworder->no_w_id = w_id;
    neworder->no_d_id = d_id;
    neworder->no_o_id = o_id;

    int64_t key = makeNewOrderKey(neworder->no_w_id, neworder->no_d_id, neworder->no_o_id);
    neworders_.insert(key, neworder);
    return neworder;
}*/

__attribute__((transaction_safe)) NewOrder* TPCCTables::insertNewOrder(int64_t w_id, int64_t d_id, int64_t o_id) {
    NewOrder* neworder = (NewOrder*) malloc(sizeof(NewOrder));
    neworder->no_w_id = w_id;
    neworder->no_d_id = d_id;
    neworder->no_o_id = o_id;

    int64_t key = makeNewOrderKey(neworder->no_w_id, neworder->no_d_id, neworder->no_o_id);
    neworders_.insert_tm(key, neworder);
    return neworder;
}

NewOrder* TPCCTables::findNewOrder(int64_t w_id, int64_t d_id, int64_t o_id) {
    NewOrder* output = NULL;
    if (neworders_.find_tm(makeNewOrderKey(w_id, d_id, o_id), &output)) {
        return output;
    }
    return NULL;
}
