#include "csv_writer.h"
#include <print>

CsvWriter::CsvWriter(const std::string& bbo_path, const std::string& trades_path)
    : m_bbo_file{bbo_path}, m_trades_file{trades_path} {
    std::println(m_bbo_file,    "timestamp,symbol,bid_price,bid_size,ask_price,ask_size,spread");
    std::println(m_trades_file, "timestamp,symbol,price,shares");
}

void CsvWriter::write_bbo(std::string_view symbol, Timestamp ts, const BBO& bbo) {
    Price spread = bbo.ask.price - bbo.bid.price;

    std::println(m_bbo_file,
        "{},{},{}.{:04},{},{}.{:04},{},{}.{:04}",
        ts, symbol,
        bbo.bid.price / 10000, bbo.bid.price % 10000, bbo.bid.shares,
        bbo.ask.price / 10000, bbo.ask.price % 10000, bbo.ask.shares,
        spread / 10000, spread % 10000);
}

void CsvWriter::write_trade(std::string_view symbol, Timestamp ts, const TradePrint& trade) {
    std::println(m_trades_file,
        "{},{},{}.{:04},{}",
        ts, symbol,
        trade.price / 10000, trade.price % 10000,
        trade.shares);
}
