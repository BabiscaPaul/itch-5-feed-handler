#pragma once

#include "messages.h"
#include "order_book.h"
#include <fstream>
#include <string>
#include <string_view>

struct TradePrint {
    Price  price{};
    Shares shares{};
};

class CsvWriter {
    std::ofstream m_bbo_file;
    std::ofstream m_trades_file;

    public:
        CsvWriter(const std::string& bbo_path, const std::string& trades_path);

        void write_bbo(std::string_view symbol, Timestamp ts, const BBO& bbo);
        void write_trade(std::string_view symbol, Timestamp ts, const TradePrint& trade);
};
