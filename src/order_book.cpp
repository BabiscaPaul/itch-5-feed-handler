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
            auto it = m_asks.find(price);
            if (it == m_asks.end()) break;
            if (shares >= it->second) m_asks.erase(it);
            else                      it->second -= shares;
            break;
        }

        case Side::Buy : {
            auto it = m_bids.find(price);
            if (it == m_bids.end()) break;
            if (shares >= it->second) m_bids.erase(it);
            else                      it->second -= shares;
            break;
        }

        default: break;
    }
}