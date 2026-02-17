#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ostream>
#include <vector>

namespace lob {

class LatencyStats {
public:
    void reserve(std::size_t count) {
        samples_.reserve(count);
    }

    void add(std::uint64_t ns) {
        samples_.push_back(ns);
        if (ns < min_) {
            min_ = ns;
        }
        if (ns > max_) {
            max_ = ns;
        }
        sum_ += static_cast<long double>(ns);
    }

    std::size_t count() const {
        return samples_.size();
    }

    void dump_csv(std::ostream& os) const {
        os << "sample_ns\n";
        for (const auto s : samples_) {
            os << s << "\n";
        }
    }

    void report(std::ostream& os) const {
        if (samples_.empty()) {
            os << "Latency: no samples\n";
            return;
        }

        auto sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        const auto p50 = percentile(sorted, 0.50);
        const auto p90 = percentile(sorted, 0.90);
        const auto p99 = percentile(sorted, 0.99);
        const auto avg = static_cast<long double>(sum_) / static_cast<long double>(samples_.size());

        os << "Latency (ns): min=" << min_
           << " avg=" << static_cast<std::uint64_t>(avg)
           << " p50=" << p50
           << " p90=" << p90
           << " p99=" << p99
           << " max=" << max_ << "\n";
    }

private:
    static std::uint64_t percentile(const std::vector<std::uint64_t>& sorted, double pct) {
        if (sorted.empty()) {
            return 0;
        }
        const auto idx = static_cast<std::size_t>(pct * static_cast<double>(sorted.size() - 1));
        return sorted[idx];
    }

    std::vector<std::uint64_t> samples_;
    std::uint64_t min_ = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t max_ = 0;
    long double sum_ = 0.0L;
};

} // namespace lob
