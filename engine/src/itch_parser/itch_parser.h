#pragma once

#include <iostream>
#include <array>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <format>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "messages.h"
#include "book_manager/book_manager.h"
#include "order_book/order_book.h"
#include "csv/csv_writer.h"

inline void parse_and_build(const uint8_t* data, size_t size,
                            BookManager& mgr,
                            CsvWriter& writer,
                            const std::unordered_set<std::string>& tracked_symbols) {
    std::array<uint64_t, 256> count{};
    std::unordered_map<StockLocate, std::string> tracked_locates{};
    std::unordered_map<StockLocate, BBO> last_bbo{};

    auto write_bbo_if_changed = [&](StockLocate locate, Timestamp ts) {
        auto sym_it = tracked_locates.find(locate);
        if (sym_it == tracked_locates.end()) return;

        const OrderBook* book = mgr.book_for(locate);
        if (!book) return;

        auto current = book->get_bbo();
        if (!current) return;

        auto& last = last_bbo[locate];
        if (*current != last) {
            writer.write_bbo(sym_it->second, ts, *current);
            last = *current;
        }
    };

    size_t offset{0};
    auto start = std::chrono::steady_clock::now();

    while (offset < size) {
        uint16_t len = read_u16_be(data + offset);
        const uint8_t* body = data + offset + 2;
        unsigned char type = body[0];
        count[type]++;

        switch (type) {
            case 'R': {
                StockDirectory m{body};
                mgr.on_stock_directory(m.stock_locate(), m.stock());
                std::string sym{m.stock()};
                if (tracked_symbols.contains(sym)) {
                    tracked_locates[m.stock_locate()] = std::move(sym);
                }
                break;
            }
            case 'A': {
                AddOrder m{body};
                mgr.on_add(m.order_ref(), static_cast<Side>(m.side()),
                        m.shares(), m.price(), m.stock_locate());
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'F': {
                AddOrderMPID m{body};
                mgr.on_add(m.order_ref(), static_cast<Side>(m.side()),
                        m.shares(), m.price(), m.stock_locate());
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'E': {
                OrderExecuted m{body};
                auto sym_it = tracked_locates.find(m.stock_locate());
                std::optional<Price> trade_price;
                if (sym_it != tracked_locates.end()) {
                    trade_price = mgr.price_for(m.order_ref());
                }
                mgr.on_executed(m.order_ref(), m.executed_shares());

                if (sym_it != tracked_locates.end() && trade_price) {
                    writer.write_trade(sym_it->second, m.timestamp(),
                                    TradePrint{*trade_price, m.executed_shares()});
                }
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'C': {
                OrderExecutedWithPrice m{body};
                mgr.on_executed_with_price(m.order_ref(), m.executed_shares());

                auto sym_it = tracked_locates.find(m.stock_locate());
                if (sym_it != tracked_locates.end()) {
                    writer.write_trade(sym_it->second, m.timestamp(),
                                    TradePrint{m.price(), m.executed_shares()});
                }
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'X': {
                OrderCancel m{body};
                mgr.on_cancel(m.order_ref(), m.cancelled_shares());
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'D': {
                OrderDelete m{body};
                mgr.on_delete(m.order_ref());
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            case 'U': {
                OrderReplace m{body};
                mgr.on_replace(m.old_order_ref(), m.new_order_ref(),
                            m.shares(), m.price());
                write_bbo_if_changed(m.stock_locate(), m.timestamp());
                break;
            }
            default: break;
        }

        offset += 2 + len;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    uint64_t total = 0;
    for (uint64_t c : count) total += c;

    for (int i = 0; i < 256; ++i) {
        if (count[i]) std::cout << static_cast<char>(i) << ": " << count[i] << '\n';
    }

    std::cout << std::format("\nTotal: {}\n", total);
    std::cout << std::format("Time: {:.2f}s ({:.1f}M msgs/sec)\n",
                            elapsed.count(),
                            total / elapsed.count() / 1e6);
}
