#pragma once

#include <iostream>
#include <array>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <format>
#include "messages.h"
#include "book_manager.h"
#include "order_book.h"

inline void parse_and_build(const uint8_t* data, size_t size, BookManager& mgr) {
    std::array<uint64_t, 256> count{};
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
                break;
            }
            case 'A': {
                AddOrder m{body};
                mgr.on_add(m.order_ref(), static_cast<Side>(m.side()), m.shares(), m.price(), m.stock_locate());
                break;
            }
            case 'F': {
                AddOrderMPID m{body};
                mgr.on_add(m.order_ref(), static_cast<Side>(m.side()), m.shares(), m.price(), m.stock_locate());
                break;
            }
            case 'E': {
                OrderExecuted m{body};
                mgr.on_executed(m.order_ref(), m.executed_shares());
                break;
            }
            case 'C': {
                OrderExecutedWithPrice m{body};
                mgr.on_executed_with_price(m.order_ref(), m.executed_shares());
                break;
            }
            case 'X': {
                OrderCancel m{body};
                mgr.on_cancel(m.order_ref(), m.cancelled_shares());
                break;
            }
            case 'D': {
                OrderDelete m{body};
                mgr.on_delete(m.order_ref());
                break;
            }
            case 'U': {
                OrderReplace m{body};
                mgr.on_replace(m.old_order_ref(), m.new_order_ref(), m.shares(), m.price());
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
