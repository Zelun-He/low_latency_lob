#pragma once

#include "time_utils.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstdint>
#include <random>

namespace lob {

struct SimConfig {
    std::size_t count = 100000;
    std::int64_t base_price = 10000; // 100.00
    std::int64_t price_range = 50;   // +/- 0.50
    std::int64_t max_qty = 100;
    std::uint64_t seed = 1;
    double buy_ratio = 0.5;
};

template <typename Fn>
void run_simulation(const SimConfig& cfg, Fn&& on_order) {
    std::mt19937_64 rng(cfg.seed);
    std::uniform_int_distribution<std::int64_t> price_delta(-cfg.price_range, cfg.price_range);
    std::uniform_int_distribution<std::int64_t> qty_dist(1, std::max<std::int64_t>(1, cfg.max_qty));
    std::bernoulli_distribution side_dist(cfg.buy_ratio);

    for (std::size_t i = 0; i < cfg.count; ++i) {
        const auto delta = price_delta(rng);
        const auto price = std::max<std::int64_t>(1, cfg.base_price + delta);
        Order order;
        order.id = static_cast<std::uint64_t>(i + 1);
        order.side = side_dist(rng) ? Side::Buy : Side::Sell;
        order.price = price;
        order.qty = qty_dist(rng);
        order.ts_ns = now_ns();
        on_order(order);
    }
}

} // namespace lob
