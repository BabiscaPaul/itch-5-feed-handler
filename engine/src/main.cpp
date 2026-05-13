#include "symbol_loader.h"
#include "mmap_reader.h"
#include "itch_parser/itch_parser.h"
#include "book_manager/book_manager.h"
#include "csv/csv_writer.h"
#include <filesystem>
#include <print>
#include <string>
#include <unordered_set>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::println("Usage: {} <itch-file> <symbols-file> <output-dir>\n", argv[0]);
        return 1;
    }

    const auto itch_path = argv[1];
    const auto symbols_path = argv[2];
    const std::filesystem::path output_dir{argv[3]};

    const auto tracked_symbols = load_symbols(symbols_path);

    if (tracked_symbols.empty()) {
        std::println("Error: symbols file is missing or empty: {}\n One symbol per line, e.g.:\n AAPL\n  MSFT\n  SPY\n", symbols_path);
        return 1;
    }

    std::println("Tracking {} symbols from symbols.txt\n", tracked_symbols.size());

    MmapReader reader{itch_path};
    if (!reader.is_valid()) return 1;

    std::println("File size: {} bytes ({:.2f} GB)\n\n", reader.size(), reader.size() / 1e9);

    std::filesystem::create_directories(output_dir);

    BookManager manager{};
    CsvWriter writer{
        (output_dir / "bbo.csv").string(),
        (output_dir / "trades.csv").string()
    };

    parse_and_build(reader.data(), reader.size(), manager, writer, tracked_symbols);

    return 0;
}
