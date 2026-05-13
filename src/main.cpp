#include "symbol_loader.h"
#include "mmap_reader.h"
#include "itch_parser.h"
#include "book_manager.h"
#include "csv/csv_writer.h"
#include <print>
#include <string>
#include <unordered_set>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::println("Usage: {} <itch-file> <symbols-file>\n", argv[0]);
        return 1;
    }

    const auto itch_path = argv[1];
    const auto symbols_path = argv[2];
    const auto tracked_symbols = load_symbols(symbols_path);

    if (tracked_symbols.empty()) {
        std::println("Error: symbols.txt is missing or empty.\n Create one at the repo root with one symbol per line, e.g.:\n AAPL\n  MSFT\n  SPY\n");
        return 1;
    }

    std::println("Tracking {} symbols from symbols.txt\n", tracked_symbols.size());

    MmapReader reader{itch_path};
    if (!reader.is_valid()) return 1;

    std::println("File size: {} bytes ({:.2f} GB)\n\n", reader.size(), reader.size() / 1e9);

    BookManager manager{};
    CsvWriter writer{"output/bbo_history.csv", "output/trades.csv"};

    parse_and_build(reader.data(), reader.size(), manager, writer, tracked_symbols);

    return 0;
}
