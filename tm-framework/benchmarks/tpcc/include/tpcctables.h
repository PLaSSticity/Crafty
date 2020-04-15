#ifndef TPCCTABLES_H__
#define TPCCTABLES_H__

#include <map>
#include <set>
#include <vector>
#include "btree.h"
#include "clock.h"
#include "tpccdb.h"

#include "tm.h"

class CustomerByNameOrdering {
public:
    bool operator()(const Customer* a, const Customer* b);
};

// Stores all the tables in TPC-C
class TPCCTables : public TPCCDB {
public:
    virtual ~TPCCTables();

    virtual int64_t stockLevel(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t threshold);
    virtual void orderStatus(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id,
            OrderStatusOutput* output);
    virtual void orderStatus(TM_ARGDECL int64_t warehouse_id, int64_t district_id, const char* c_last,
            OrderStatusOutput* output);
    virtual bool newOrder(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id,
            const std::vector<NewOrderItem>& items, const char* now,
            NewOrderOutput* output, TPCCUndo** undo);
    __attribute__((transaction_safe)) virtual bool newOrderHome(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t customer_id,
            const std::vector<NewOrderItem>& items, const char* now,
            NewOrderOutput* output, TPCCUndo** undo, Item** item_tuples);
    __attribute__((transaction_safe)) virtual bool newOrderRemote(TM_ARGDECL int64_t home_warehouse, int64_t remote_warehouse,
            const std::vector<NewOrderItem>& items, std::vector<int64_t>* out_quantities,
            TPCCUndo** undo, Item** item_tuples);
    virtual void payment(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
            int64_t c_district_id, int64_t customer_id, float h_amount, const char* now,
            PaymentOutput* output, TPCCUndo** undo);
    virtual void payment(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
            int64_t c_district_id, const char* c_last, float h_amount, const char* now,
            PaymentOutput* output, TPCCUndo** undo);
    __attribute__((transaction_safe)) virtual void paymentHome(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
            int64_t c_district_id, int64_t c_id, float h_amount, const char* now,
            PaymentOutput* output, TPCCUndo** undo);
    __attribute__((transaction_safe)) virtual void paymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
            int64_t c_district_id, int64_t c_id, float h_amount, PaymentOutput* output,
            TPCCUndo** undo);
    virtual void paymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, int64_t c_warehouse_id,
            int64_t c_district_id, const char* c_last, float h_amount, PaymentOutput* output,
            TPCCUndo** undo);
    virtual void delivery(TM_ARGDECL int64_t warehouse_id, int64_t carrier_id, const char* now,
            std::vector<DeliveryOrderInfo>* orders, TPCCUndo** undo);
    virtual bool hasWarehouse(TM_ARGDECL int64_t warehouse_id) { return findWarehouse(TM_ARG warehouse_id) != NULL; }

    // Copies item into the item table.
    void insertItemSingleThread(Item* item);
    void insertItem(TM_ARGDECL Item* item);
    __attribute__((transaction_safe)) Item* findItem(TM_ARGDECL int64_t i_id);

    void insertWarehouse(TM_ARGDECL const Warehouse& warehouse);
    void insertWarehouseSingleThread(const Warehouse& warehouse);
    __attribute__((transaction_safe)) Warehouse* findWarehouse(TM_ARGDECL int64_t w_id);

    void insertStock(TM_ARGDECL const Stock& stock);
    void insertStockSingleThread(const Stock& stock);
    __attribute__((transaction_safe)) Stock* findStock(TM_ARGDECL int64_t w_id, int64_t s_id);

    void insertDistrict(TM_ARGDECL const District& district);
    void insertDistrictSingleThread(const District& district);
    __attribute__((transaction_safe)) District* findDistrict(TM_ARGDECL int64_t w_id, int64_t d_id);

    void insertCustomer(TM_ARGDECL const Customer& customer);
    void insertCustomerSingleThread(const Customer& customer);
    __attribute__((transaction_safe)) Customer* findCustomer(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t c_id);
    // Finds all customers that match (w_id, d_id, *, c_last), taking the n/2th one (rounded up).
    Customer* findCustomerByName(int64_t w_id, int64_t d_id, const char* c_last);

    // Stores order in the database. Returns a pointer to the database's tuple.
    __attribute__((transaction_safe)) Order* insertOrder(TM_ARGDECL const Order& order);
    Order* insertOrderSingleThread(const Order& order);
    __attribute__((transaction_safe)) Order* findOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id);
    __attribute__((transaction_safe)) Order* findLastOrderByCustomer(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t c_id);

    // Stores orderline in the database. Returns a pointer to the database's tuple.
    OrderLine* insertOrderLine(TM_ARGDECL const OrderLine& orderline);
    OrderLine* insertOrderLineSingleThread(const OrderLine& orderline);
    __attribute__((transaction_safe)) OrderLine* findOrderLine(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id, int64_t number);

    // Creates a new order in the database. Returns a pointer to the database's tuple.
    __attribute__((transaction_safe)) NewOrder* insertNewOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id);
    NewOrder* insertNewOrderSingleThread(int64_t w_id, int64_t d_id, int64_t o_id);
    NewOrder* findNewOrder(TM_ARGDECL int64_t w_id, int64_t d_id, int64_t o_id);

    static const int KEYS_PER_INTERNAL = 8;
    static const int KEYS_PER_LEAF = 8;

private:
    static const int STOCK_LEVEL_ORDERS = 100;

    // Loads each item from the items table. Returns true if they are all found.
    __attribute__((transaction_safe)) bool findAndValidateItems(TM_ARGDECL const std::vector<NewOrderItem>& items,
            Item** item_tuples);

    // Implements order status transaction after the customer tuple has been located.
    __attribute__((transaction_safe)) void internalOrderStatus(TM_ARGDECL Customer* customer, OrderStatusOutput* output);

    // Implements payment transaction after the customer tuple has been located.
    __attribute__((transaction_safe)) void internalPaymentRemote(TM_ARGDECL int64_t warehouse_id, int64_t district_id, Customer* c,
            float h_amount, PaymentOutput* output, TPCCUndo** undo);

    // Erases order from the database. NOTE: This is only for undoing transactions.
    void eraseOrder(const Order* order);
    // Erases order_line from the database. NOTE: This is only for undoing transactions.
    void eraseOrderLine(const OrderLine* order_line);
    // Erases new_order from the database. NOTE: This is only for undoing transactions.
    void eraseNewOrder(const NewOrder* new_order);


    BPlusTree<int64_t, Item*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> items_;
    BPlusTree<int64_t, Warehouse*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> warehouses_;
    BPlusTree<int64_t, Stock*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> stock_;
    BPlusTree<int64_t, District*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> districts_;
    BPlusTree<int64_t, Customer*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> customers_;
    typedef std::set<Customer*, CustomerByNameOrdering> CustomerByNameSet;
    CustomerByNameSet customers_by_name_;
    BPlusTree<int64_t, Order*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> orders_;
    // TODO: Tune the size of this tree for the bigger keys?
    BPlusTree<int64_t, Order*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> orders_by_customer_;
    BPlusTree<int64_t, OrderLine*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> orderlines_;
    BPlusTree<int64_t, NewOrder*, KEYS_PER_INTERNAL, KEYS_PER_LEAF> neworders_;
};

#endif
