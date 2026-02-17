#pragma once

#include "types.hpp"

#include <deque>
#include <functional>
#include <map>
#include <ostream>
#include <vector>

namespace lob {

class OrderBook {
public:
    void add(const Order& order);
    void add(Order&& order);

    void match(Order& incoming, std::vector<Trade>& trades);

    std::int64_t best_bid() const;
    std::int64_t best_ask() const;

    void dump(std::ostream& os, std::size_t depth = 10) const;
    void dump_csv(std::ostream& os) const;

private:
    struct PriceLevel {
        std::deque<Order> orders;
        std::int64_t total_qty = 0;
    };

    std::map<std::int64_t, PriceLevel, std::greater<>> bids_;
    std::map<std::int64_t, PriceLevel, std::less<>> asks_;
};

} // namespace lob
