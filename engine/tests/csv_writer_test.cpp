#include <gtest/gtest.h>
#include "csv/csv_writer.h"
#include "order_book/order_book.h"
#include "messages.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
    namespace fs = std::filesystem;

    class TempDir {
        fs::path m_path;
    public:
        TempDir() {
            static int counter = 0;
            m_path = fs::temp_directory_path()
                / ("itch_csv_writer_test_"
                + std::to_string(::getpid())
                + "_"
                + std::to_string(counter++));
            fs::create_directories(m_path);
        }
        ~TempDir() {
            std::error_code ec;
            fs::remove_all(m_path, ec);
        }

        fs::path bbo_path()   const { return m_path / "bbo.csv"; }
        fs::path trade_path() const { return m_path / "trades.csv"; }
    };

    std::vector<std::string> read_lines(const fs::path& p) {
        std::vector<std::string> lines;
        std::ifstream in{p};
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }
}

// ============================================================
// Construction — error handling
// ============================================================

TEST(CsvWriterTest, ConstructorThrowsWhenBboPathUnopenable) {
    EXPECT_THROW(
        CsvWriter("/no/such/dir/bbo.csv", "/tmp/trades.csv"),
        std::runtime_error);
}

TEST(CsvWriterTest, ConstructorThrowsWhenTradesPathUnopenable) {
    TempDir td;
    EXPECT_THROW(
        CsvWriter(td.bbo_path().string(), "/no/such/dir/trades.csv"),
        std::runtime_error);
}

TEST(CsvWriterTest, ConstructorThrowsWhenBothPathsUnopenable) {
    EXPECT_THROW(
        CsvWriter("/no/such/dir/bbo.csv", "/no/such/dir/trades.csv"),
        std::runtime_error);
}

// ============================================================
// Construction + headers
// ============================================================

TEST(CsvWriterTest, ConstructorWritesBboHeader) {
    TempDir td;
    { CsvWriter w{td.bbo_path().string(), td.trade_path().string()}; }

    auto lines = read_lines(td.bbo_path());
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines[0], "timestamp,symbol,bid_price,bid_size,ask_price,ask_size,spread");
}

TEST(CsvWriterTest, ConstructorWritesTradesHeader) {
    TempDir td;
    { CsvWriter w{td.bbo_path().string(), td.trade_path().string()}; }

    auto lines = read_lines(td.trade_path());
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines[0], "timestamp,symbol,price,shares");
}

TEST(CsvWriterTest, EmptyWriterProducesHeaderOnly) {
    TempDir td;
    { CsvWriter w{td.bbo_path().string(), td.trade_path().string()}; }

    EXPECT_EQ(read_lines(td.bbo_path()).size(),   1u);
    EXPECT_EQ(read_lines(td.trade_path()).size(), 1u);
}

// ============================================================
// write_bbo — formatting
// ============================================================

TEST(CsvWriterTest, BboRowBasic) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{1650000, 100}, {1651000, 50}};
        w.write_bbo("AAPL", 1234567890, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1234567890,AAPL,165.0000,100,165.1000,50,0.1000");
}

TEST(CsvWriterTest, BboFractionalCentsPaddedToFourDecimals) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{1650001, 1}, {1650023, 1}};
        w.write_bbo("AAPL", 0, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "0,AAPL,165.0001,1,165.0023,1,0.0022");
}

TEST(CsvWriterTest, BboZeroPriceAndShares) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{0, 0}, {0, 0}};
        w.write_bbo("X", 0, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "0,X,0.0000,0,0.0000,0,0.0000");
}

TEST(CsvWriterTest, BboLargePriceCrossesFiveDigitDollar) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{999999999, 1}, {1000000000, 1}};   // $99999.9999 -> $100000.0000
        w.write_bbo("BIG", 1, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,BIG,99999.9999,1,100000.0000,1,0.0001");
}

TEST(CsvWriterTest, BboLargeTimestamp) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{1650000, 100}, {1660000, 50}};
        w.write_bbo("AAPL", 57600000000000ULL, b);   // ~16:00 ET in ns since midnight
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "57600000000000,AAPL,165.0000,100,166.0000,50,1.0000");
}

TEST(CsvWriterTest, BboSpreadOneTick) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{1650000, 100}, {1650001, 50}};   // 1 tick spread
        w.write_bbo("AAPL", 1, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0000,100,165.0001,50,0.0001");
}

TEST(CsvWriterTest, BboLockedMarketZeroSpread) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        BBO b{{1650000, 100}, {1650000, 50}};   // bid == ask (locked)
        w.write_bbo("AAPL", 1, b);
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0000,100,165.0000,50,0.0000");
}

TEST(CsvWriterTest, BboMultipleRowsAppended) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_bbo("AAPL", 1, BBO{{1650000, 100}, {1660000, 50}});
        w.write_bbo("AAPL", 2, BBO{{1651000, 200}, {1660000, 50}});
        w.write_bbo("MSFT", 3, BBO{{1060000, 100}, {1063000, 50}});
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0000,100,166.0000,50,1.0000");
    EXPECT_EQ(lines[2], "2,AAPL,165.1000,200,166.0000,50,0.9000");
    EXPECT_EQ(lines[3], "3,MSFT,106.0000,100,106.3000,50,0.3000");
}

TEST(CsvWriterTest, BboHighRowCountStaysConsistent) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        for (uint32_t i = 0; i < 1000; ++i) {
            w.write_bbo("AAPL", i, BBO{{1650000u + i, 100}, {1660000u, 50}});
        }
    }
    EXPECT_EQ(read_lines(td.bbo_path()).size(), 1001u);
}

// ============================================================
// write_trade — formatting
// ============================================================

TEST(CsvWriterTest, TradeRowBasic) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_trade("AAPL", 100, TradePrint{1650000, 250});
    }
    auto lines = read_lines(td.trade_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "100,AAPL,165.0000,250");
}

TEST(CsvWriterTest, TradeFractionalCentsPaddedToFourDecimals) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_trade("AAPL", 1, TradePrint{1650007, 1});
    }
    auto lines = read_lines(td.trade_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0007,1");
}

TEST(CsvWriterTest, TradeZeroPriceAndShares) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_trade("X", 0, TradePrint{0, 0});
    }
    auto lines = read_lines(td.trade_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "0,X,0.0000,0");
}

TEST(CsvWriterTest, TradeLargePrice) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_trade("BIG", 1, TradePrint{999999999, 1});
    }
    auto lines = read_lines(td.trade_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,BIG,99999.9999,1");
}

TEST(CsvWriterTest, TradeMultipleRowsAppended) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_trade("AAPL", 1, {1650000, 100});
        w.write_trade("AAPL", 2, {1651000, 50});
        w.write_trade("MSFT", 3, {1060000, 200});
    }
    auto lines = read_lines(td.trade_path());
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0000,100");
    EXPECT_EQ(lines[2], "2,AAPL,165.1000,50");
    EXPECT_EQ(lines[3], "3,MSFT,106.0000,200");
}

// ============================================================
// Interleaved writes don't cross files
// ============================================================

TEST(CsvWriterTest, BboAndTradeWritesAreIndependent) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        w.write_bbo  ("AAPL", 1, BBO{{1650000, 100}, {1660000, 50}});
        w.write_trade("AAPL", 2, {1655000, 25});
        w.write_bbo  ("AAPL", 3, BBO{{1651000, 100}, {1660000, 50}});
        w.write_trade("AAPL", 4, {1656000, 75});
    }

    auto bbo_lines = read_lines(td.bbo_path());
    auto trd_lines = read_lines(td.trade_path());

    ASSERT_EQ(bbo_lines.size(), 3u);   // header + 2 rows
    ASSERT_EQ(trd_lines.size(), 3u);   // header + 2 rows

    EXPECT_EQ(bbo_lines[1], "1,AAPL,165.0000,100,166.0000,50,1.0000");
    EXPECT_EQ(bbo_lines[2], "3,AAPL,165.1000,100,166.0000,50,0.9000");
    EXPECT_EQ(trd_lines[1], "2,AAPL,165.5000,25");
    EXPECT_EQ(trd_lines[2], "4,AAPL,165.6000,75");
}

// ============================================================
// Symbol passed as string_view from various sources
// ============================================================

TEST(CsvWriterTest, BboSymbolFromStdString) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        std::string sym = "AAPL";
        w.write_bbo(sym, 1, BBO{{1650000, 100}, {1660000, 50}});
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,AAPL,165.0000,100,166.0000,50,1.0000");
}

TEST(CsvWriterTest, BboSymbolFromStringView) {
    TempDir td;
    {
        CsvWriter w{td.bbo_path().string(), td.trade_path().string()};
        const char* buf = "MSFTGARBAGE";
        std::string_view sym{buf, 4};
        w.write_bbo(sym, 1, BBO{{1060000, 100}, {1063000, 50}});
    }
    auto lines = read_lines(td.bbo_path());
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "1,MSFT,106.0000,100,106.3000,50,0.3000");
}
