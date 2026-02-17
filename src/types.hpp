#pragma once

#include <cstdint>
#include <string>

namespace lob {

enum class Side { Buy, Sell };

inline std::string side_to_string(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

struct Order {
    std::uint64_t id = 0;
    Side side = Side::Buy;
    std::int64_t price = 0; // price in ticks (cents)
    std::int64_t qty = 0;
    std::uint64_t ts_ns = 0;
};

struct Trade {
    std::uint64_t taker_id = 0;
    std::uint64_t maker_id = 0;
    std::int64_t price = 0;
    std::int64_t qty = 0;
};

} // namespace lob
