#include <gtest/gtest.h>
#include "symbol_loader.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {
    namespace fs = std::filesystem;

    class TempDir {
        fs::path m_path;
    public:
        TempDir() {
            static int counter = 0;
            m_path = fs::temp_directory_path()
                / ("itch_symbol_loader_test_"
                + std::to_string(::getpid())
                + "_"
                + std::to_string(counter++));
            fs::create_directories(m_path);
        }
        ~TempDir() {
            std::error_code ec;
            fs::remove_all(m_path, ec);
        }

        fs::path symbols_path() const { return m_path / "symbols.txt"; }
    };

    void write_file(const fs::path& p, std::string_view content) {
        std::ofstream out{p, std::ios::binary};
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
}

// ============================================================
// File presence / emptiness
// ============================================================

TEST(SymbolLoaderTest, MissingFileReturnsEmpty) {
    TempDir td;
    auto syms = load_symbols(td.symbols_path());   // never created
    EXPECT_TRUE(syms.empty());
}

TEST(SymbolLoaderTest, EmptyFileReturnsEmpty) {
    TempDir td;
    write_file(td.symbols_path(), "");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_TRUE(syms.empty());
}

TEST(SymbolLoaderTest, OnlyBlankLinesReturnsEmpty) {
    TempDir td;
    write_file(td.symbols_path(), "\n\n\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_TRUE(syms.empty());
}

// ============================================================
// Basic parsing
// ============================================================

TEST(SymbolLoaderTest, SingleSymbol) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 1u);
    EXPECT_TRUE(syms.contains("AAPL"));
}

TEST(SymbolLoaderTest, MultipleSymbols) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\nMSFT\nGOOGL\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 3u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
    EXPECT_TRUE(syms.contains("GOOGL"));
}

TEST(SymbolLoaderTest, MissingFinalNewlineStillParsesLast) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\nMSFT");   // no trailing \n
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("MSFT"));
}

// ============================================================
// Whitespace and line-ending handling
// ============================================================

TEST(SymbolLoaderTest, BlankLinesInterspersedAreSkipped) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\n\n\nMSFT\n\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

TEST(SymbolLoaderTest, TrailingCarriageReturnTrimmed) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\r\nMSFT\r\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

TEST(SymbolLoaderTest, TrailingSpacesTrimmed) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL   \nMSFT \nGOOG\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 3u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
    EXPECT_TRUE(syms.contains("GOOG"));
}

TEST(SymbolLoaderTest, MixedTrailingCarriageReturnAndSpace) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL \r\nMSFT  \r\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

TEST(SymbolLoaderTest, LineWithOnlySpacesIsSkipped) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\n   \nMSFT\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

TEST(SymbolLoaderTest, LineWithOnlyCarriageReturnIsSkipped) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\n\r\nMSFT\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

// ============================================================
// Set semantics
// ============================================================

TEST(SymbolLoaderTest, DuplicateSymbolsDeduplicated) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\nMSFT\nAAPL\nMSFT\nAAPL\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 2u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
}

TEST(SymbolLoaderTest, DuplicateAfterTrimDeduplicated) {
    // "AAPL" and "AAPL   " collapse to the same key after trimming.
    TempDir td;
    write_file(td.symbols_path(), "AAPL\nAAPL   \nAAPL\r\n");
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 1u);
    EXPECT_TRUE(syms.contains("AAPL"));
}

// ============================================================
// Lookup behavior we rely on from callers
// ============================================================

TEST(SymbolLoaderTest, LookupOnLoadedSetWorks) {
    TempDir td;
    write_file(td.symbols_path(), "AAPL\nMSFT\nGOOGL\n");
    auto syms = load_symbols(td.symbols_path());

    EXPECT_TRUE (syms.contains("AAPL"));
    EXPECT_TRUE (syms.contains("MSFT"));
    EXPECT_FALSE(syms.contains("AAPL "));   // not present — caller-side string must be exact
    EXPECT_FALSE(syms.contains("aapl"));    // case-sensitive
    EXPECT_FALSE(syms.contains("FB"));
}

// ============================================================
// Realistic-shape test
// ============================================================

TEST(SymbolLoaderTest, ProductionLikeFileMessy) {
    TempDir td;
    write_file(td.symbols_path(),
        "AAPL\r\n"
        "\n"
        "MSFT  \n"
        "GOOGL\r\n"
        "\n"
        "\n"
        "MSFT\n"          // duplicate
        "AAPL  \n"        // duplicate after trim
        "TSLA \r\n"
        "   \n"           // whitespace-only -> skipped
    );
    auto syms = load_symbols(td.symbols_path());
    EXPECT_EQ(syms.size(), 4u);
    EXPECT_TRUE(syms.contains("AAPL"));
    EXPECT_TRUE(syms.contains("MSFT"));
    EXPECT_TRUE(syms.contains("GOOGL"));
    EXPECT_TRUE(syms.contains("TSLA"));
}
