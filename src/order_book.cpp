#include "order_book.h"

std::optional<BBO> OrderBook::get_bbo() const {
    if (!has_asks() || !has_bids()) return std::nullopt; 

    return BBO {
        Level{m_bids.begin()->first, m_bids.begin()->second}, 
        Level{m_asks.begin()->first, m_asks.begin()->second}
    };
}

bool OrderBook::has_bids() const {
    return !m_bids.empty();
}

bool OrderBook::has_asks() const {
    return !m_asks.empty();
}

void OrderBook::add(Side side, Price price, Shares shares) {
    switch (side) {
        case Side::Sell : {
            m_asks[price] += shares;
            break;
        }

        case Side::Buy : {
            m_bids[price] += shares;
            break;
        }

        default: break;
    }
}

void OrderBook::reduce(Side side, Price price, Shares shares) {
    switch (side) {
        case Side::Sell : {
            m_asks[price] -= shares;
            if (m_asks[price] == 0) m_asks.erase(price);
            break;
        }

        case Side::Buy : {
            m_bids[price] -= shares;
            if (m_bids[price] == 0) m_bids.erase(price);
            break;
        }

        default: break;
    }
}