#pragma once

#include "messages.h"
#include <functional>
#include <map>
#include <optional>

enum class Side : char {
    Buy = 'B', 
    Sell = 'S'
};

struct Order {
    OrderRef order_ref{};
    Side side{};
    Shares shares{};
    Price price{};
    StockLocate locate{};
};

struct Level {
    Price  price{};
    Shares shares{};
    bool operator==(const Level&) const = default;
};

struct BBO {
    Level bid{};
    Level ask{};
    bool operator==(const BBO&) const = default;
};

class OrderBook {
    std::map<Price, Shares, std::greater<>> m_bids{};
    std::map<Price, Shares> m_asks{};

    public:
        std::optional<BBO> get_bbo() const;
        bool has_bids() const;
        bool has_asks() const;
        void add(Side side, Price price, Shares shares);
        void reduce(Side side, Price price, Shares shares);
};