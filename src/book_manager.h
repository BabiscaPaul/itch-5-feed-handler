#pragma once

#include "messages.h"
#include "order_book.h"
#include <string>
#include <string_view>
#include <unordered_map>

class BookManager {
    std::unordered_map<OrderRef, Order> m_orders{};
    std::unordered_map<StockLocate, OrderBook> m_books{};
    std::unordered_map<std::string, StockLocate> m_symbol_to_locate{};

    void reduce_order(OrderRef ref, Shares shares);

    public:
        void on_stock_directory(StockLocate locate, std::string_view symbol);

        void on_add(OrderRef ref, Side side, Shares shares, Price price, StockLocate locate);
        void on_executed(OrderRef ref, Shares shares);
        void on_executed_with_price(OrderRef ref, Shares shares);
        void on_cancel(OrderRef ref, Shares shares);
        void on_delete(OrderRef ref);
        void on_replace(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price);

        const OrderBook* book_for(StockLocate locate) const;
        const OrderBook* book_for_symbol(std::string_view symbol) const;
};
