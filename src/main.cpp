#include "mmap_reader.h"
#include "itch_parser.h"
#include "book_manager.h"
#include "order_book.h"
#include "csv/csv_writer.h"
#include <format>
#include <iostream>

static void csv_writer_smoke_test() {
    std::cout << "--- CsvWriter smoke test ---\n";

    CsvWriter writer{"output/bbo_history.csv", "output/trades.csv"};

    // 1. Normal BBO row — AAPL, ~$165.25 area
    writer.write_bbo("AAPL", 36000000000000ULL,
                    BBO{Level{1652400, 500}, Level{1652600, 300}});

    // 2. BBO at a different timestamp, same symbol
    writer.write_bbo("AAPL", 36000000123456ULL,
                    BBO{Level{1652500, 700}, Level{1652700, 200}});

    // 3. Different symbol, wider spread
    writer.write_bbo("MSFT", 36000000999999ULL,
                    BBO{Level{1040000, 1000}, Level{1045000, 800}});

    // 4. Sub-penny price
    writer.write_bbo("SUB", 36000002000000ULL,
                    BBO{Level{1000050, 10}, Level{1000075, 10}});

    // 6. Normal trade
    writer.write_trade("AAPL", 36001234567890ULL,
                    TradePrint{1652500, 100});

    // 7. Big size trade
    writer.write_trade("SPY", 36005678901234ULL,
                    TradePrint{2800000, 100000});

    // 8. Tiny trade
    writer.write_trade("MSFT", 36009999999999ULL,
                    TradePrint{1042500, 1});

    std::cout << "Wrote to output/bbo_history.csv and output/trades.csv\n";
}

int main(int argc, char* argv[]) {
    csv_writer_smoke_test();

    if (argc < 2) {
        std::cout << std::format("\nNo ITCH file provided; smoke test only.\n");
        return 0;
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
