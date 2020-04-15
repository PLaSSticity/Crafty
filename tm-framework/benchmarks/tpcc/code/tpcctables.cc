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

int64_t TPCCTables::stockLevel(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t threshold) {
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
    //TM_THREAD_ENTER();
     int ro = 1;
    TM_BEGIN();
        District* d = findDistrict(TM_ARG warehouse_id, district_id);
	//printf("address of d is %p\n",d);
	int64_t o_id = d->d_next_o_id;
        // Iterate over [o_id-20, o_id)
        int64_t stock_quantity;
	Stock * stock;
        for (int64_t order_id = o_id - STOCK_LEVEL_ORDERS; order_id < o_id; ++order_id) {
            for (int line_number = 1; line_number <= Order::MAX_OL_CNT; ++line_number) {
                OrderLine* line = findOrderLine(TM_ARG warehouse_id, district_id, order_id, line_number);
                if (line == NULL) {
                    // We can break since we have reached the end of the lines for this order.
                    // TODO: A btree iterate in (w_id, d_id, o_id) order would be a clean way to do this
                    break;
                }
                // Check if s_quantity < threshold
                stock = findStock(TM_ARG warehouse_id, line->ol_i_id);
                if(local_exec_mode == 1 || local_exec_mode == 3){
		        stock_quantity = SLOW_PATH_SHARED_READ(stock->s_quantity);
		}
                else{
                	stock_quantity = FAST_PATH_SHARED_READ(stock->s_quantity);
		}
                if (stock_quantity < threshold) {
                    s_i_ids[counter_s] = line->ol_i_id;
                    counter_s++;
			//printf("%d\n",counter_s);
                }
            }
        }
	if(stock_quantity>0){
    if(local_exec_mode == 1 || local_exec_mode == 3){
	SLOW_PATH_SHARED_WRITE(stock->s_w_id,1);
    }
    else{
    	FAST_PATH_SHARED_WRITE(stock->s_w_id,1);
    }
  }
  TM_END();
  //TM_THREAD_EXIT();
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

void TPCCTables::orderStatus(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id, OrderStatusOutput* output) {
  //~ printf("order status %d %d %d\n", warehouse_id, district_id, customer_id);
    //__transaction_atomic {
    // FIXME(nmld): transaction block here
    //TM_THREAD_ENTER();
     int ro = 1;
    TM_BEGIN();
	Customer* customer = findCustomer(TM_ARG warehouse_id, district_id, customer_id);
        internalOrderStatus(TM_ARG customer, output);
    TM_END();
    //TM_THREAD_EXIT();
    //}
}

void TPCCTables::orderStatus(TM_ARGDECL int64_t warehouse_id, int64_t district_id, const char* c_last, OrderStatusOutput* output) {
    //~ printf("order status %d %d %s\n", warehouse_id, district_id, c_last);
    //TM_THREAD_ENTER();
     int ro = 1;
    TM_BEGIN();
    Customer* customer = findCustomerByName(warehouse_id, district_id, c_last);
    internalOrderStatus(TM_ARG customer, output);
    TM_END();
    //TM_THREAD_EXIT();
}

__attribute__((transaction_safe)) void TPCCTables::internalOrderStatus(TM_ARGDECL Customer* customer, OrderStatusOutput* output) {
    output->c_id = customer->c_id;
    if(customer->c_id > 2){
    // retrieve from customer: balance, first, middle, last
    if(local_exec_mode == 1 || local_exec_mode == 3){
      output->c_balance = SLOW_PATH_SHARED_READ_D(customer->c_balance);
    }
    else{
      output->c_balance = FAST_PATH_SHARED_READ_D(customer->c_balance);
    }

    // Find the row in the order table with largest o_id
    Order* order = findLastOrderByCustomer(TM_ARG customer->c_w_id, customer->c_d_id, customer->c_id);
    output->o_id = order->o_id;
    output->o_carrier_id = order->o_carrier_id;

    for (int64_t line_number = 1; line_number <= order->o_ol_cnt; ++line_number) {
        OrderLine* line = findOrderLine(TM_ARG customer->c_w_id, customer->c_d_id, order->o_id, line_number);
	if (line && line->ol_amount < 0.0) { customer->c_credit_lim = line->ol_amount; }
   }
   }
}

bool TPCCTables::newOrder(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id,
        const std::vector<NewOrderItem>& items, const char* now, NewOrderOutput* output,TPCCUndo** undo) {
    uint64_t items_size = items.size();
    int64_t* warehouse_set = (int64_t*) malloc(items_size * sizeof(int64_t));
    Item** item_tuples = (Item**) malloc(items_size * sizeof(Item*));

/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
        // perform the home part
        int ro = 0;
       //TM_THREAD_ENTER();
	bool result;
       TM_BEGIN();
        result = newOrderHome(TM_ARG warehouse_id, district_id, customer_id, items, now, output, undo, item_tuples);
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
            result = newOrderRemote(TM_ARG warehouse_id, warehouse_set[i], items, NULL, undo, item_tuples);
        }
      TM_END();
      //TM_THREAD_EXIT();
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

__attribute__((transaction_safe)) bool TPCCTables::newOrderHome(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id,
        const vector<NewOrderItem>& items, const char* now,
        NewOrderOutput* output, TPCCUndo** undo, Item** item_tuples) {
    //~ printf("new order %d %d %d %d %s\n", warehouse_id, district_id, customer_id, items.size(), now);
    // 2.4.3.4. requires that we display c_last, c_credit, and o_id for rolled back transactions:
    // read those values first
    District* d = findDistrict(TM_ARG warehouse_id, district_id);
    output->d_tax = d->d_tax;
    output->o_id = d->d_next_o_id;
    Customer* c = findCustomer(TM_ARG warehouse_id, district_id, customer_id);
    output->c_discount = c->c_discount;

    // CHEAT: Validate all items to see if we will need to abort
    uint64_t vec_size = getVectorSize(items);
    if (!findAndValidateItems(TM_ARG items, item_tuples)) {
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
    d->d_next_o_id = d->d_next_o_id + 1;

    Warehouse* w = findWarehouse(TM_ARG warehouse_id);
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
    Order* o = insertOrder(TM_ARG order);
    NewOrder* no = insertNewOrder(TM_ARG warehouse_id, district_id, output->o_id);

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
        Stock* stock = findStock(TM_ARG items[i].ol_supply_w_id, items[i].i_id);
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

        OrderLine* ol = insertOrderLine(TM_ARG line);
    }

    // Perform the "remote" part for this warehouse
    // TODO: It might be more efficient to merge this into the loop above, but this is simpler.
    bool result = newOrderRemote(TM_ARG warehouse_id, warehouse_id, items, NULL, undo, item_tuples);

    return true;
}

__attribute__((transaction_safe)) bool TPCCTables::newOrderRemote(TM_ARGDECL int64_t home_warehouse, int64_t remote_warehouse,
        const vector<NewOrderItem>& items, std::vector<int64_t>* out_quantities, TPCCUndo** undo, Item** item_tuples) {
    // Validate all the items: needed so that we don't need to undo in order to execute this
    // TODO: item_tuples is unused. Remove?
    uint64_t vec_size = getVectorSize(items);
    if (!findAndValidateItems(TM_ARG items, item_tuples)) {
        return false;
    }

    for (int i = 0; i < getVectorSize(items); ++i) {
        const NewOrderItem& item = getVectorItem(items, i);

        // Skip items that don't belong to remote warehouse
        if (item.ol_supply_w_id != remote_warehouse) {
            continue;
        }

        // update stock
        Stock* stock = findStock(TM_ARG item.ol_supply_w_id, item.i_id);
        if(local_exec_mode == 1 || local_exec_mode == 3){
          intptr_t temp_var = SLOW_PATH_SHARED_READ(stock->s_quantity);
          if (temp_var >= item.ol_quantity + 10) {
            SLOW_PATH_SHARED_WRITE(stock->s_quantity, temp_var-item.ol_quantity);
          } else {
  	        SLOW_PATH_SHARED_WRITE(stock->s_quantity, temp_var-item.ol_quantity+91);
          }
        	intptr_t temp_var_2 = SLOW_PATH_SHARED_READ(stock->s_ytd);
        	SLOW_PATH_SHARED_WRITE(stock->s_ytd, temp_var_2+item.ol_quantity);
        	intptr_t temp_var_3 = SLOW_PATH_SHARED_READ(stock->s_order_cnt);
        	SLOW_PATH_SHARED_WRITE(stock->s_order_cnt, temp_var_3+1);
          // newOrderHome calls newOrderRemote, so this is needed
          if (item.ol_supply_w_id != home_warehouse) {
              // remote order
  	          intptr_t temp_var_4 = SLOW_PATH_SHARED_READ(stock->s_remote_cnt);
  	          SLOW_PATH_SHARED_WRITE(stock->s_remote_cnt, temp_var_4+1);
          }
        } else{
          intptr_t temp_var = FAST_PATH_SHARED_READ(stock->s_quantity);
          if (temp_var >= item.ol_quantity + 10) {
            FAST_PATH_SHARED_WRITE(stock->s_quantity, temp_var-item.ol_quantity);
          } else {
  	         FAST_PATH_SHARED_WRITE(stock->s_quantity, temp_var-item.ol_quantity+91);
          }
        	intptr_t temp_var_2 = FAST_PATH_SHARED_READ(stock->s_ytd);
        	FAST_PATH_SHARED_WRITE(stock->s_ytd, temp_var_2+item.ol_quantity);
        	intptr_t temp_var_3 = FAST_PATH_SHARED_READ(stock->s_order_cnt);
        	FAST_PATH_SHARED_WRITE(stock->s_order_cnt, temp_var_3+1);
          // newOrderHome calls newOrderRemote, so this is needed
          if (item.ol_supply_w_id != home_warehouse) {
              // remote order
  	          intptr_t temp_var_4 = FAST_PATH_SHARED_READ(stock->s_remote_cnt);
  	          FAST_PATH_SHARED_WRITE(stock->s_remote_cnt, temp_var_4+1);
          }
        }
    }

    return true;
}

__attribute__((transaction_safe)) bool TPCCTables::findAndValidateItems(TM_ARGDECL const vector<NewOrderItem>& items,
        Item** item_tuples) {
    // CHEAT: Validate all items to see if we will need to abort
    for (int i = 0; i < getVectorSize(items); ++i) {
        Item* item = findItem(TM_ARG getVectorItem(items, i).i_id);
        item_tuples[i] = item;
        if (item == NULL) {
            return false;
        }
    }
    return true;
}


void TPCCTables::payment(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t customer_id, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
    //~ printf("payment %d %d %d %d %d %f %s\n", warehouse_id, district_id, c_warehouse_id, c_district_id, customer_id, h_amount, now);
/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
    //TM_THREAD_ENTER();
     int ro = 0;
    TM_BEGIN();
        Customer* customer = findCustomer(TM_ARG c_warehouse_id, c_district_id, customer_id);
        paymentHome(TM_ARG warehouse_id, district_id, c_warehouse_id, c_district_id, customer_id, h_amount,
                now, output, undo);
        internalPaymentRemote(TM_ARG warehouse_id, district_id, customer, h_amount, output, undo);
     TM_END();
     //TM_THREAD_EXIT();
/*    } */
}

void TPCCTables::payment(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, const char* c_last, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
          printf("WTF, why am I here?!!!\n" );
    //~ printf("payment %d %d %d %d %s %f %s\n", warehouse_id, district_id, c_warehouse_id, c_district_id, c_last, h_amount, now);
    /*Customer* customer = findCustomerByName(c_warehouse_id, c_district_id, c_last);
    paymentHome(TM_ARG warehouse_id, district_id, c_warehouse_id, c_district_id, customer->c_id, h_amount,
            now, output, undo);
    internalPaymentRemote(warehouse_id, district_id, customer, h_amount, output, undo);*/
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

void TPCCTables::paymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t c_id, float h_amount, PaymentOutput* output,
        TPCCUndo** undo) {
    Customer* customer = findCustomer(TM_ARG c_warehouse_id, c_district_id, c_id);
    internalPaymentRemote(TM_ARG warehouse_id, district_id, customer, h_amount, output, undo);
    zeroWarehouseDistrict(output);
}
void TPCCTables::paymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, const char* c_last, float h_amount, PaymentOutput* output,
        TPCCUndo** undo) {
    Customer* customer = findCustomerByName(c_warehouse_id, c_district_id, c_last);
    internalPaymentRemote(TM_ARG warehouse_id, district_id, customer, h_amount, output, undo);
    zeroWarehouseDistrict(output);
}

__attribute__((transaction_safe)) void TPCCTables::paymentHome(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
        int64_t c_district_id, int64_t customer_id, float h_amount, const char* now,
        PaymentOutput* output, TPCCUndo** undo) {
    Warehouse* w = findWarehouse(TM_ARG warehouse_id);
    if(local_exec_mode == 1 || local_exec_mode == 3){
      float temp_float = SLOW_PATH_SHARED_READ_D(w->w_ytd);
      TM_SHR_WR(w->w_ytd, temp_float+h_amount);
      District* d = findDistrict(TM_ARG warehouse_id, district_id);
      float temp_float_2 = SLOW_PATH_SHARED_READ_D(d->d_ytd);
      TM_SHR_WR(d->d_ytd, temp_float_2+h_amount);
    } else{
      float temp_float = FAST_PATH_SHARED_READ_D(w->w_ytd);
      TM_SHR_WR(w->w_ytd, temp_float+h_amount);

      District* d = findDistrict(TM_ARG warehouse_id, district_id);
      float temp_float_2 = FAST_PATH_SHARED_READ_D(d->d_ytd);
      TM_SHR_WR(d->d_ytd, temp_float_2+h_amount);
    }

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

__attribute__((transaction_safe)) void TPCCTables::internalPaymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, Customer* c,
        float h_amount, PaymentOutput* output, TPCCUndo** undo) {
    if(local_exec_mode == 1 || local_exec_mode == 3){
      float temp_float = SLOW_PATH_SHARED_READ_D(c->c_balance);
      TM_SHR_WR(c->c_balance, temp_float-h_amount);
      float temp_float_2 = SLOW_PATH_SHARED_READ_D(c->c_ytd_payment);
      TM_SHR_WR(c->c_ytd_payment, temp_float_2+h_amount);
      intptr_t temp_var = SLOW_PATH_SHARED_READ(c->c_payment_cnt);
      SLOW_PATH_SHARED_WRITE(c->c_payment_cnt, temp_var+1);

      output->c_credit_lim = SLOW_PATH_SHARED_READ_D(c->c_credit_lim);
      output->c_discount = c->c_discount;
      output->c_balance = SLOW_PATH_SHARED_READ_D(c->c_balance);
    } else{
      float temp_float = FAST_PATH_SHARED_READ_D(c->c_balance);
      TM_SHR_WR(c->c_balance, temp_float-h_amount);
      float temp_float_2 = FAST_PATH_SHARED_READ_D(c->c_ytd_payment);
      TM_SHR_WR(c->c_ytd_payment, temp_float_2+h_amount);
      intptr_t temp_var = FAST_PATH_SHARED_READ(c->c_payment_cnt);
      FAST_PATH_SHARED_WRITE(c->c_payment_cnt, temp_var+1);

      output->c_credit_lim = FAST_PATH_SHARED_READ_D(c->c_credit_lim);
      output->c_discount = c->c_discount;
      output->c_balance = FAST_PATH_SHARED_READ_D(c->c_balance);
    }
}

#undef ZERO_ADDRESS
#undef COPY_ADDRESS

// forward declaration for delivery
__attribute__((transaction_safe)) static int64_t makeNewOrderKey(int64_t w_id, int64_t d_id, int64_t o_id);

void TPCCTables::delivery(TM_ARGDECL int64_t warehouse_id, int64_t carrier_id, const char* now,
        std::vector<DeliveryOrderInfo>* orders, TPCCUndo** undo) {
    //~ printf("delivery %d %d %s\n", warehouse_id, carrier_id, now);

/*    __transaction_atomic { */
    // FIXME(nmld): transaction block here
    //TM_THREAD_ENTER();
     int ro = 0;
    TM_BEGIN();
        for (int64_t d_id = 1; d_id <= District::NUM_PER_WAREHOUSE; ++d_id) {
            // Find and remove the lowest numbered order for the district

            int64_t key = makeNewOrderKey(warehouse_id, d_id, 1);
            NewOrder* neworder;
            int64_t foundKey = -1;
	    if(local_exec_mode == 1 || local_exec_mode == 3){
	            if (!neworders_.slow_findLastLessThan(TM_ARG key, &neworder, &foundKey)) {
        	        neworder = NULL;
          	    }
	    } else{
                    if (!neworders_.fast_findLastLessThan(TM_ARG key, &neworder, &foundKey)) {
                        neworder = NULL;
                    }
	    }

            if (neworder == NULL || neworder->no_d_id != d_id || neworder->no_w_id != warehouse_id) {
                // No orders for this district
                // TODO: 2.7.4.2: If this occurs in max(1%, 1) of transactions, report it (???)
                continue;
            }
            int64_t o_id = neworder->no_o_id;
            if(local_exec_mode == 1 || local_exec_mode == 3)
              neworders_.slow_del(TM_ARG foundKey);
            else
              neworders_.fast_del(TM_ARG foundKey);
            // FIXME: delete neworder

            Order* o = findOrder(TM_ARG warehouse_id, d_id, o_id);
	    o->o_carrier_id = carrier_id;

            float total = 0;
            // TODO: Select based on (w_id, d_id, o_id) rather than using ol_number?
            for (int64_t i = 1; i <= o->o_ol_cnt; ++i) {
                OrderLine* line = findOrderLine(TM_ARG warehouse_id, d_id, o_id, i);
		char date_now[DATETIME_SIZE+1];
                tm_strncpy(date_now, now, Clock::DATETIME_SIZE);
                total += line->ol_amount;
            }

            Customer* c = findCustomer(TM_ARG warehouse_id, d_id, o->o_c_id);
            if(local_exec_mode == 1 || local_exec_mode == 3){
  	          float temp_float = SLOW_PATH_SHARED_READ_D(c->c_balance);
  	          TM_SHR_WR(c->c_balance, temp_float+total);
              intptr_t temp_var = SLOW_PATH_SHARED_READ(c->c_delivery_cnt);
  	          SLOW_PATH_SHARED_WRITE(c->c_delivery_cnt, temp_var+1);
            } else{
              float temp_float = FAST_PATH_SHARED_READ_D(c->c_balance);
  	          TM_SHR_WR(c->c_balance, temp_float+total);
              intptr_t temp_var = FAST_PATH_SHARED_READ(c->c_delivery_cnt);
  	          FAST_PATH_SHARED_WRITE(c->c_delivery_cnt, temp_var+1);
            }
    }
    TM_END();
    //TM_THREAD_EXIT();
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
__attribute__((transaction_safe)) static T* insert_tm(TM_ARGDECL BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree, int64_t key, const T& item) {
    T* copy = (T*) malloc(sizeof(T));
    tm_memcpy(copy, &item, sizeof(T));
    if(local_exec_mode == 1 || local_exec_mode == 3)
      tree->slow_insert(TM_ARG key, copy);
    else
      tree->fast_insert(TM_ARG key, copy);
    return copy;
}

template <typename T>
__attribute__((transaction_safe)) static T* insert(TM_ARGDECL BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>* tree, int64_t key, const T& item) {
    T* copy = (T*) malloc(sizeof(T));
    tm_memcpy(copy, &item, sizeof(T));
    tree->insert(key, copy);
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
__attribute__((transaction_safe))  static T* find_tm(TM_ARGDECL const BPlusTree<int64_t, T*, TPCCTables::KEYS_PER_INTERNAL, TPCCTables::KEYS_PER_LEAF>& tree, int64_t key) {
    T* output = NULL;
    bool out;
    if(local_exec_mode == 1 || local_exec_mode == 3)
      out = tree.slow_find(TM_ARG key, &output);
    else
      out = tree.fast_find(TM_ARG key, &output);
    if (out) {
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

void TPCCTables::insertItem(TM_ARGDECL Item* item) {
  if(local_exec_mode == 1 || local_exec_mode == 3) {
    items_.slow_insert(TM_ARG item->i_id, item);
  } else {
    items_.fast_insert(TM_ARG item->i_id, item);
  }
}

void TPCCTables::insertItemSingleThread(Item* item) {
  items_.insert(item->i_id, item);
}

__attribute__((transaction_safe)) Item* TPCCTables::findItem(TM_ARGDECL int64_t id) {

    return find_tm(TM_ARG items_, id);
}

void TPCCTables::insertWarehouse(TM_ARGDECL const Warehouse& w) {
    insert_tm(TM_ARG &warehouses_, w.w_id, w);
}

void TPCCTables::insertWarehouseSingleThread(TM_ARGDECL const Warehouse& w) {
    insert(&warehouses_, w.w_id, w);
}
__attribute__((transaction_safe)) Warehouse* TPCCTables::findWarehouse(TM_ARGDECL int64_t id) {
    return find_tm(TM_ARG warehouses_, id);
}

__attribute__((transaction_safe)) static int64_t makeStockKey(int64_t w_id, int64_t s_id) {
    return s_id + (w_id * Stock::NUM_STOCK_PER_WAREHOUSE);
}

void TPCCTables::insertStock(TM_ARGDECL const Stock& stock) {
    insert_tm(TM_ARG &stock_, makeStockKey(stock.s_w_id, stock.s_i_id), stock);
}

void TPCCTables::insertStockSingleThread(const Stock& stock) {
    insert(&stock_, makeStockKey(stock.s_w_id, stock.s_i_id), stock);
}

Stock* TPCCTables::findStock(TM_ARGDECL int64_t w_id, int64_t s_id) {
    return find_tm(TM_ARG stock_, makeStockKey(w_id, s_id));
}

__attribute__((transaction_safe))  static int64_t makeDistrictKey(int64_t w_id, int64_t d_id) {
    return d_id + (w_id * District::NUM_PER_WAREHOUSE);
}

void TPCCTables::insertDistrict(TM_ARGDECL const District& district) {
    insert_tm(TM_ARG &districts_, makeDistrictKey(district.d_w_id, district.d_id), district);
}

void TPCCTables::insertDistrictSingleThread(const District& district) {
    insert(&districts_, makeDistrictKey(district.d_w_id, district.d_id), district);
}

__attribute__((transaction_safe)) District* TPCCTables::findDistrict(TM_ARGDECL int64_t w_id, int64_t d_id) {
    return find_tm(TM_ARG districts_, makeDistrictKey(w_id, d_id));
}

__attribute__((transaction_safe)) static int64_t makeCustomerKey(int64_t w_id, int64_t d_id, int64_t c_id) {
    return (w_id * District::NUM_PER_WAREHOUSE + d_id)
            * Customer::NUM_PER_DISTRICT + c_id;
}

void TPCCTables::insertCustomer(TM_ARGDECL const Customer& customer) {
    Customer* c = insert_tm(TM_ARG &customers_, makeCustomerKey(customer.c_w_id, customer.c_d_id, customer.c_id), customer);
    assert(customers_by_name_.find(c) == customers_by_name_.end());
    customers_by_name_.insert(c);
}

void TPCCTables::insertCustomerSingleThread(const Customer& customer) {
    Customer* c = insert(TM_ARG &customers_, makeCustomerKey(customer.c_w_id, customer.c_d_id, customer.c_id), customer);
    assert(customers_by_name_.find(c) == customers_by_name_.end());
    customers_by_name_.insert(c);
}

__attribute__((transaction_safe)) Customer* TPCCTables::findCustomer(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t c_id) {
    return find_tm(TM_ARG customers_, makeCustomerKey(w_id, d_id, c_id));
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
  return (((int64_t) top_id) << 8) | o_id;
}

/*__attribute__((transaction_safe)) Order* TPCCTables::insertOrder(const Order& order) {
Order* tuple = insert(&orders_, makeOrderKey(order.o_w_id, order.o_d_id, order.o_id), order);
// Secondary index based on customer id
int64_t key = makeOrderByCustomerKey(order.o_w_id, order.o_d_id, order.o_c_id, order.o_id);
orders_by_customer_.insert(key, tuple);
return tuple;
}*/

__attribute__((transaction_safe)) Order* TPCCTables::insertOrder(TM_ARGDECL const Order& order) {
  Order* tuple = insert_tm(TM_ARG &orders_, makeOrderKey(order.o_w_id, order.o_d_id, order.o_id), order);
  // Secondary index based on customer id
  int64_t key = makeOrderByCustomerKey(order.o_w_id, order.o_d_id, order.o_c_id, order.o_id);
  if(local_exec_mode == 1 || local_exec_mode == 3)
    orders_by_customer_.slow_insert(TM_ARG key, tuple);
  else
    orders_by_customer_.fast_insert(TM_ARG key, tuple);
  return tuple;
}

Order* TPCCTables::insertOrderSingleThread(const Order& order) {
  Order* tuple = insert(&orders_, makeOrderKey(order.o_w_id, order.o_d_id, order.o_id), order);
  // Secondary index based on customer id
  int64_t key = makeOrderByCustomerKey(order.o_w_id, order.o_d_id, order.o_c_id, order.o_id);
  orders_by_customer_.insert(key, tuple);
  return tuple;
}


__attribute__((transaction_safe)) Order* TPCCTables::findOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id) {
  return find_tm(TM_ARG orders_, makeOrderKey(w_id, d_id, o_id));
}

__attribute__((transaction_safe)) Order* TPCCTables::findLastOrderByCustomer(TM_ARGDECL const int64_t w_id, const int64_t d_id, const int64_t c_id) {
  Order* order = NULL;

  // Increment the (w_id, d_id, c_id) tuple
  int64_t key = makeOrderByCustomerKey(w_id, d_id, c_id, 1);
  key += ((int64_t)1) << 8;
  bool found;
  if(local_exec_mode == 1 || local_exec_mode == 3)
  found = orders_by_customer_.slow_findLastLessThan(TM_ARG key, &order);
  else
  found = orders_by_customer_.fast_findLastLessThan(TM_ARG key, &order);
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

OrderLine* TPCCTables::insertOrderLine(TM_ARGDECL const OrderLine& orderline) {
  int64_t key = makeOrderLineKey(
    orderline.ol_w_id, orderline.ol_d_id, orderline.ol_o_id, orderline.ol_number
  );
  return insert_tm(TM_ARG &orderlines_, key, orderline);
}

OrderLine* TPCCTables::insertOrderLineSingleThread(const OrderLine& orderline) {
  int64_t key = makeOrderLineKey(
    orderline.ol_w_id, orderline.ol_d_id, orderline.ol_o_id, orderline.ol_number
  );
  return insert(TM_ARG &orderlines_, key, orderline);
}

__attribute__((transaction_safe)) OrderLine* TPCCTables::findOrderLine(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id, int64_t number) {
  return find_tm(TM_ARG orderlines_, makeOrderLineKey(w_id, d_id, o_id, number));
}

__attribute__((transaction_safe)) static int64_t makeNewOrderKey(int64_t w_id, int64_t d_id, int64_t o_id) {
  int64_t upper_id = w_id * Warehouse::MAX_WAREHOUSE_ID + d_id;
  int64_t id = static_cast<int64_t>(upper_id) << 8 | o_id;
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

__attribute__((transaction_safe)) NewOrder* TPCCTables::insertNewOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id) {
  NewOrder* neworder = (NewOrder*) malloc(sizeof(NewOrder));
  neworder->no_w_id = w_id;
  neworder->no_d_id = d_id;
  neworder->no_o_id = o_id;

  int64_t key = makeNewOrderKey(neworder->no_w_id, neworder->no_d_id, neworder->no_o_id);
  if(local_exec_mode == 1 || local_exec_mode == 3)
  neworders_.slow_insert(TM_ARG key, neworder);
  else
  neworders_.fast_insert(TM_ARG key, neworder);
  return neworder;
}

NewOrder* TPCCTables::insertNewOrderSingleThread(int64_t w_id, int64_t d_id, int64_t o_id) {
  NewOrder* neworder = (NewOrder*) malloc(sizeof(NewOrder));
  neworder->no_w_id = w_id;
  neworder->no_d_id = d_id;
  neworder->no_o_id = o_id;

  int64_t key = makeNewOrderKey(neworder->no_w_id, neworder->no_d_id, neworder->no_o_id);
  neworders_.insert(key, neworder);
  return neworder;
}

NewOrder* TPCCTables::findNewOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id) {
  NewOrder* output = NULL;
  bool out;
  if(local_exec_mode == 1 || local_exec_mode == 3)
  out = neworders_.slow_find(TM_ARG makeNewOrderKey(w_id, d_id, o_id), &output);
  else
  out = neworders_.fast_find(TM_ARG makeNewOrderKey(w_id, d_id, o_id), &output);
  if (out) {
    return output;
  }
  return NULL;
}
