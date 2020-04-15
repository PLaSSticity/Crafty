#include "tpccclient.h"

#include <cstdio>
#include <vector>

#include "tpcc_assert.h"
#include "clock.h"
#include "randomgenerator.h"
#include "tpccdb.h"

using std::vector;

// Non-integral constants must be defined in a .cc file. Needed for Mac OS X.
// http://www.research.att.com/~bs/bs_faq2.html#in-class
const float TPCCClient::MIN_PAYMENT_AMOUNT;
const float TPCCClient::MAX_PAYMENT_AMOUNT;

volatile int global_stock_level_txs_ratio;
volatile int global_delivery_txs_ratio;
volatile int global_order_status_txs_ratio;
volatile int global_payment_txs_ratio;
volatile int global_new_order_ratio;
volatile int global_num_warehouses;

TPCCClient::TPCCClient(Clock* clock, tpcc::RandomGenerator* generator, TPCCDB* db, int num_items,
        int num_warehouses, int districts_per_warehouse, int customers_per_district) :
        clock_(clock),
        generator_(generator),
        db_(db),
        num_items_(num_items),
        num_warehouses_(num_warehouses),
        districts_per_warehouse_(districts_per_warehouse),
        customers_per_district_(customers_per_district),
        remote_item_milli_p_(OrderLine::REMOTE_PROBABILITY_MILLIS),
        bound_warehouse_(0),
        bound_district_(0),
        executed_stock_level_txs_(0),
        executed_delivery_txs_(0),
        executed_order_status_txs_(0),
        executed_payment_txs_(0),
        executed_new_order_txs_(0) {
    ASSERT(clock_ != NULL);
    ASSERT(generator_ != NULL);
    ASSERT(db_ != NULL);
    ASSERT(1 <= num_items_ && num_items_ <= Item::NUM_ITEMS);
    ASSERT(1 <= num_warehouses_ && num_warehouses_ <= Warehouse::MAX_WAREHOUSE_ID);
    ASSERT(1 <= districts_per_warehouse_ &&
            districts_per_warehouse_ <= District::NUM_PER_WAREHOUSE);
    ASSERT(1 <= customers_per_district_ && customers_per_district_ <= Customer::NUM_PER_DISTRICT);
}

TPCCClient::~TPCCClient() {
}

void TPCCClient::doStockLevel(TM_ARGDECL_ALONE) {
    int32_t threshold = generator_->number(MIN_STOCK_LEVEL_THRESHOLD, MAX_STOCK_LEVEL_THRESHOLD);
    int result = db_->stockLevel(TM_ARG generateWarehouse(), generateDistrict(), threshold);
    ASSERT(result >= 0);
}

void TPCCClient::doOrderStatus(TM_ARGDECL_ALONE) {
    OrderStatusOutput output;
    int y = generator_->number(1, 100);
    if (y < 0) {    // FIXME: we are not supporting searches by Customer Name
        // 60%: order status by last name
        char c_last[Customer::MAX_LAST+1];
        generator_->lastName(c_last, customers_per_district_);
        db_->orderStatus(TM_ARG generateWarehouse(), generateDistrict(), c_last, &output);
    } else {
        // 40%: order status by id
        db_->orderStatus(TM_ARG generateWarehouse(), generateDistrict(), generateCID(), &output);
    }
}

void TPCCClient::doDelivery(TM_ARGDECL_ALONE) {
    int carrier = generator_->number(Order::MIN_CARRIER_ID, Order::MAX_CARRIER_ID);
    char now[Clock::DATETIME_SIZE+1];
    clock_->getDateTimestamp(now);

    db_->delivery(TM_ARG generateWarehouse(), carrier, now, NULL, NULL);
}

void TPCCClient::doPayment(TM_ARGDECL_ALONE) {
    PaymentOutput output;
    int x = generator_->number(1, 100);
    int y = generator_->number(1, 100);

    int32_t w_id = generateWarehouse();
    int32_t d_id = generateDistrict();

    int32_t c_w_id;
    int32_t c_d_id;
    if (global_num_warehouses == 1 || x <= 85) {
        // 85%: paying through own warehouse (or there is only 1 warehouse)
        c_w_id = w_id;
        c_d_id = d_id;
    } else {
        // 15%: paying through another warehouse:
        // select in range [1, num_warehouses] excluding w_id
        c_w_id = generator_->numberExcluding(1, global_num_warehouses, w_id);
        ASSERT(c_w_id != w_id);
        c_d_id = generateDistrict();
    }
    float h_amount = generator_->fixedPoint(2, MIN_PAYMENT_AMOUNT, MAX_PAYMENT_AMOUNT);

    char now[Clock::DATETIME_SIZE+1];
    clock_->getDateTimestamp(now);
    if (y < 0) { // FIXME: we are not supporting searches by Last Name of customer
        // 60%: payment by last name
        char c_last[Customer::MAX_LAST+1];
        generator_->lastName(c_last, customers_per_district_);
        db_->payment(TM_ARG w_id, d_id, c_w_id, c_d_id, c_last, h_amount, now, &output, NULL);
    } else {
        // 40%: payment by id
        db_->payment(TM_ARG w_id, d_id, c_w_id, c_d_id, generateCID(), h_amount, now, &output, NULL);
    }
}

bool TPCCClient::doNewOrder(TM_ARGDECL_ALONE) {
    int32_t w_id = generateWarehouse();
    int ol_cnt = generator_->number(Order::MIN_OL_CNT, Order::MAX_OL_CNT);

    // 1% of transactions roll back
    bool rollback = generator_->number(1, 100) == 1;

    vector<NewOrderItem> items(ol_cnt);
    for (int i = 0; i < ol_cnt; ++i) {
        if (rollback && i+1 == ol_cnt) {
            items[i].i_id = Item::NUM_ITEMS + 1;
        } else {
            items[i].i_id = generateItemID();
        }

        // TPC-C suggests generating a number in range (1, 100) and selecting remote on 1
        // This provides more variation, and lets us tune the fraction of "remote" transactions.
        bool remote = generator_->number(1, 1000) <= remote_item_milli_p_;
        if (global_num_warehouses > 1 && remote) {
            items[i].ol_supply_w_id = generator_->numberExcluding(1, global_num_warehouses, w_id);
        } else {
            items[i].ol_supply_w_id = w_id;
        }
        items[i].ol_quantity = generator_->number(1, MAX_OL_QUANTITY);
    }

    char now[Clock::DATETIME_SIZE+1];
    clock_->getDateTimestamp(now);
    NewOrderOutput output;
    bool result = db_->newOrder(TM_ARG
            w_id, generateDistrict(), generateCID(), items, now, &output, NULL);
    assert(result == !rollback);
    return result;
}

void TPCCClient::doOne(TM_ARGDECL_ALONE) {
    // This is not strictly accurate: The requirement is for certain *minimum* percentages to be
    // maintained. This is close to the right thing, but not precisely correct.
    // See TPC-C 5.2.4 (page 68).
    int x = generator_->number(1, 100);
    if (x <= global_stock_level_txs_ratio) {
        doStockLevel(TM_ARG_ALONE);
        executed_stock_level_txs_++;
    } else if (x <= global_stock_level_txs_ratio + global_delivery_txs_ratio) {
        doDelivery(TM_ARG_ALONE);
        executed_delivery_txs_++;
    } else if (x <= global_stock_level_txs_ratio + global_delivery_txs_ratio + global_order_status_txs_ratio) {
        doOrderStatus(TM_ARG_ALONE);
        executed_order_status_txs_++;
    } else if (x <= global_stock_level_txs_ratio + global_delivery_txs_ratio + global_order_status_txs_ratio + global_payment_txs_ratio) {
        doPayment(TM_ARG_ALONE);
        executed_payment_txs_++;
    } else {
        doNewOrder(TM_ARG_ALONE);
        executed_new_order_txs_++;
    }
}

void TPCCClient::remote_item_milli_p(int remote_item_milli_p) {
    assert(0 <= remote_item_milli_p && remote_item_milli_p <= 1000);
    remote_item_milli_p_ = remote_item_milli_p;
}

void TPCCClient::bindWarehouseDistrict(int warehouse_id, int district_id) {
    assert(0 <= warehouse_id && warehouse_id <= global_num_warehouses);
    assert(0 <= district_id && district_id <= districts_per_warehouse_);
    bound_warehouse_ = warehouse_id;
    bound_district_ = district_id;
}

int32_t TPCCClient::generateWarehouse() {
    if (bound_warehouse_ == 0) {
        return generator_->number(1, global_num_warehouses);
    } else {
        return bound_warehouse_;
    }
}

int32_t TPCCClient::generateDistrict() {
    if (bound_district_ == 0) {
        return generator_->number(1, districts_per_warehouse_);
    } else {
        return bound_district_;
    }
}

int32_t TPCCClient::generateCID() {
    return generator_->NURand(1023, 1, customers_per_district_);
}

int32_t TPCCClient::generateItemID() {
    return generator_->NURand(8191, 1, num_items_);
}
