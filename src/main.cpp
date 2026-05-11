#include "mmap_reader.h"
#include "itch_parser.h"
#include "book_manager.h"
#include "order_book.h"
#include <format>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << std::format("Usage: {} <itch-file>\n", argv[0]);
        return 1;
    }

    MmapReader reader{argv[1]};
    if (!reader.is_valid()) return 1;

    std::cout << std::format("File size: {} bytes ({:.2f} GB)\n\n",
                            reader.size(), reader.size() / 1e9);

    BookManager manager{};
    parse_and_build(reader.data(), reader.size(), manager);

    for (auto sym : {"AAPL", "MSFT", "GOOGL", "AMZN", "SPY", "TSLA"}) {
        const OrderBook* book = manager.book_for_symbol(sym);
        if (!book) {
            std::cout << std::format("\n{}: not in stock directory\n", sym);
            continue;
        }
        std::cout << std::format("\n{}: has_bids={} has_asks={}\n",
                                sym, book->has_bids(), book->has_asks());
        if (auto bbo = book->get_bbo()) {
            std::cout << std::format("  Bid: ${:.4f} x {}\n",
                                    bbo->bid.price / 10000.0, bbo->bid.shares);
            std::cout << std::format("  Ask: ${:.4f} x {}\n",
                                    bbo->ask.price / 10000.0, bbo->ask.shares);
        }
    }

    return 0;
}