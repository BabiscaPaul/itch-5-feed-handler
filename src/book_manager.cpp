#include "book_manager.h"

void BookManager::on_stock_directory(StockLocate locate, std::string_view symbol) {
    m_symbol_to_locate[std::string{symbol}] = locate;
}

void BookManager::on_add(OrderRef ref, Side side, Shares shares, Price price, StockLocate locate) {
    m_orders[ref] = Order{ref, side, shares, price, locate};
    m_books[locate].add(side, price, shares);
}

void BookManager::reduce_order(OrderRef ref, Shares shares) {
    auto it = m_orders.find(ref);
    if (it == m_orders.end()) return;

    Order& order = it->second;
    m_books[order.locate].reduce(order.side, order.price, shares);

    if (shares >= order.shares) m_orders.erase(it);
    else order.shares -= shares;
}

void BookManager::on_executed(OrderRef ref, Shares shares) { reduce_order(ref, shares); }
void BookManager::on_executed_with_price(OrderRef ref, Shares shares) { reduce_order(ref, shares); }
void BookManager::on_cancel(OrderRef ref, Shares shares) { reduce_order(ref, shares); }

void BookManager::on_delete(OrderRef ref) {
    auto it = m_orders.find(ref);
    if (it == m_orders.end()) return;

    Order& order = it->second;
    m_books[order.locate].reduce(order.side, order.price, order.shares);
    m_orders.erase(it);
}

void BookManager::on_replace(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price) {
    auto it = m_orders.find(old_ref);
    if (it == m_orders.end()) return;

    Order old_order = it->second;
    m_books[old_order.locate].reduce(old_order.side, old_order.price, old_order.shares);
    m_orders.erase(it);

    m_orders[new_ref] = Order{new_ref, old_order.side, new_shares, new_price, old_order.locate};
    m_books[old_order.locate].add(old_order.side, new_price, new_shares);
}

const OrderBook* BookManager::book_for(StockLocate locate) const {
    auto it = m_books.find(locate);
    return it == m_books.end() ? nullptr : &it->second;
}

const OrderBook* BookManager::book_for_symbol(std::string_view symbol) const {
    auto it = m_symbol_to_locate.find(std::string{symbol});
    if (it == m_symbol_to_locate.end()) return nullptr;
    return book_for(it->second);
}
