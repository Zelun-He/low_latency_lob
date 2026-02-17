#pragma once

#include "metrics.hpp"
#include "order_book.hpp"
#include "time_utils.hpp"

#include <vector>

namespace lob {

class MatchingEngine {
public:
    explicit MatchingEngine(LatencyStats& latency) : latency_(latency) {}

    void process(Order order, std::vector<Trade>& trades) {
        const auto start = now_ns();

        book_.match(order, trades);
        if (order.qty > 0) {
            book_.add(std::move(order));
        }

        const auto end = now_ns();
        latency_.add(end - start);
    }

    const OrderBook& book() const {
        return book_;
    }

private:
    OrderBook book_;
    LatencyStats& latency_;
};

} // namespace lob
