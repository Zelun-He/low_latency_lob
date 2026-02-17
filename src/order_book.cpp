#include "order_book.hpp"

#include <algorithm>
#include <iomanip>

namespace lob {

void OrderBook::add(const Order& order) {
    add(Order{order});
}

void OrderBook::add(Order&& order) {
    auto insert = [](auto& book, Order&& o) {
        auto& level = book[o.price];
        level.total_qty += o.qty;
        level.orders.push_back(std::move(o));
    };
    if (order.side == Side::Buy) {
        insert(bids_, std::move(order));
    } else {
        insert(asks_, std::move(order));
    }
}

void OrderBook::match(Order& incoming, std::vector<Trade>& trades) {
    if (incoming.qty <= 0) {
        return;
    }

    if (incoming.side == Side::Buy) {
        while (incoming.qty > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            if (incoming.price < it->first) {
                break;
            }

            auto& level = it->second;
            while (incoming.qty > 0 && !level.orders.empty()) {
                auto& maker = level.orders.front();
                const auto exec_qty = std::min(incoming.qty, maker.qty);

                incoming.qty -= exec_qty;
                maker.qty -= exec_qty;
                level.total_qty -= exec_qty;

                trades.push_back({incoming.id, maker.id, it->first, exec_qty});

                if (maker.qty == 0) {
                    level.orders.pop_front();
                }
            }

            if (level.orders.empty()) {
                asks_.erase(it);
            }
        }
    } else {
        while (incoming.qty > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            if (incoming.price > it->first) {
                break;
            }

            auto& level = it->second;
            while (incoming.qty > 0 && !level.orders.empty()) {
                auto& maker = level.orders.front();
                const auto exec_qty = std::min(incoming.qty, maker.qty);

                incoming.qty -= exec_qty;
                maker.qty -= exec_qty;
                level.total_qty -= exec_qty;

                trades.push_back({incoming.id, maker.id, it->first, exec_qty});

                if (maker.qty == 0) {
                    level.orders.pop_front();
                }
            }

            if (level.orders.empty()) {
                bids_.erase(it);
            }
        }
    }
}

std::int64_t OrderBook::best_bid() const {
    if (bids_.empty()) {
        return 0;
    }
    return bids_.begin()->first;
}

std::int64_t OrderBook::best_ask() const {
    if (asks_.empty()) {
        return 0;
    }
    return asks_.begin()->first;
}

void OrderBook::dump(std::ostream& os, std::size_t depth) const {
    os << "BIDS (price/qty)\n";
    std::size_t count = 0;
    for (const auto& [price, level] : bids_) {
        os << "  " << price << " / " << level.total_qty << "\n";
        if (++count >= depth) {
            break;
        }
    }

    os << "ASKS (price/qty)\n";
    count = 0;
    for (const auto& [price, level] : asks_) {
        os << "  " << price << " / " << level.total_qty << "\n";
        if (++count >= depth) {
            break;
        }
    }
}

void OrderBook::dump_csv(std::ostream& os) const {
    os << "side,price,total_qty\n";
    for (const auto& [price, level] : bids_) {
        os << "BID," << price << "," << level.total_qty << "\n";
    }
    for (const auto& [price, level] : asks_) {
        os << "ASK," << price << "," << level.total_qty << "\n";
    }
}

} // namespace lob
