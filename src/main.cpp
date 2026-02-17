#include "matching_engine.hpp"
#include "sim.hpp"
#include "types.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

struct Args {
    std::size_t simulate = 100000;
    bool use_stdin = false;
    bool keep_trades = false;
    bool print_book = false;
    std::size_t book_depth = 10;
    std::int64_t base_price = 10000; // 100.00
    std::int64_t price_range = 50;   // 0.50
    std::int64_t max_qty = 100;
    std::uint64_t seed = 1;
    double buy_ratio = 0.5;
    std::string dump_data_dir;
};

void print_usage() {
    std::cout << "Low-Latency Limit Order Book & Matching Engine\n"
              << "Usage:\n"
              << "  lob_engine --simulate N [options]\n"
              << "  lob_engine --stdin [options]\n\n"
              << "Options:\n"
              << "  --simulate N         Number of simulated orders (default 100000)\n"
              << "  --stdin              Read orders from stdin: SIDE PRICE QTY\n"
              << "  --base PRICE         Base price (default 100.00)\n"
              << "  --range PRICE        Max price delta (default 0.50)\n"
              << "  --max-qty N           Max quantity per order (default 100)\n"
              << "  --buy-ratio R         Buy ratio 0-1 (default 0.5)\n"
              << "  --seed N              RNG seed (default 1)\n"
              << "  --keep-trades         Retain all trades in memory\n"
              << "  --print-book          Print top of book after run\n"
              << "  --book-depth N        Depth for book print (default 10)\n"
              << "  --dump-data DIR       Dump CSV data to DIR for visualization\n"
              << "  --help                Show this help\n";
}

std::int64_t parse_price_ticks(const std::string& text) {
    const double price = std::stod(text);
    return static_cast<std::int64_t>(std::llround(price * 100.0));
}

bool parse_args(int argc, char** argv, Args& args) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help") {
            print_usage();
            return false;
        }
        if (arg == "--simulate" && i + 1 < argc) {
            args.simulate = static_cast<std::size_t>(std::stoull(argv[++i]));
            continue;
        }
        if (arg == "--stdin") {
            args.use_stdin = true;
            continue;
        }
        if (arg == "--base" && i + 1 < argc) {
            args.base_price = parse_price_ticks(argv[++i]);
            continue;
        }
        if (arg == "--range" && i + 1 < argc) {
            args.price_range = parse_price_ticks(argv[++i]);
            continue;
        }
        if (arg == "--max-qty" && i + 1 < argc) {
            args.max_qty = static_cast<std::int64_t>(std::stoll(argv[++i]));
            continue;
        }
        if (arg == "--buy-ratio" && i + 1 < argc) {
            args.buy_ratio = std::stod(argv[++i]);
            continue;
        }
        if (arg == "--seed" && i + 1 < argc) {
            args.seed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
            continue;
        }
        if (arg == "--keep-trades") {
            args.keep_trades = true;
            continue;
        }
        if (arg == "--print-book") {
            args.print_book = true;
            continue;
        }
        if (arg == "--book-depth" && i + 1 < argc) {
            args.book_depth = static_cast<std::size_t>(std::stoull(argv[++i]));
            continue;
        }
        if (arg == "--dump-data" && i + 1 < argc) {
            args.dump_data_dir = argv[++i];
            args.keep_trades = true; // need trades for CSV
            continue;
        }

        std::cerr << "Unknown argument: " << arg << "\n";
        print_usage();
        return false;
    }

    return true;
}

bool parse_order_line(const std::string& line, lob::Order& order) {
    std::istringstream iss(line);
    std::string side_text;
    std::string price_text;
    std::int64_t qty = 0;
    if (!(iss >> side_text >> price_text >> qty)) {
        return false;
    }

    if (side_text == "B" || side_text == "BUY" || side_text == "Buy" || side_text == "buy") {
        order.side = lob::Side::Buy;
    } else if (side_text == "S" || side_text == "SELL" || side_text == "Sell" || side_text == "sell") {
        order.side = lob::Side::Sell;
    } else {
        return false;
    }

    order.price = parse_price_ticks(price_text);
    order.qty = qty;
    order.ts_ns = lob::now_ns();
    return true;
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) {
        return 1;
    }

    lob::LatencyStats latency;
    if (!args.use_stdin) {
        latency.reserve(args.simulate);
    }

    lob::MatchingEngine engine(latency);
    std::vector<lob::Trade> trades;
    trades.reserve(64);

    std::size_t processed = 0;
    const auto start = std::chrono::steady_clock::now();

    if (args.use_stdin) {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }

            lob::Order order;
            order.id = static_cast<std::uint64_t>(processed + 1);
            if (!parse_order_line(line, order)) {
                std::cerr << "Invalid order line: " << line << "\n";
                return 1;
            }

            engine.process(order, trades);
            ++processed;
            if (!args.keep_trades) {
                trades.clear();
            }
        }
    } else {
        lob::SimConfig cfg;
        cfg.count = args.simulate;
        cfg.base_price = args.base_price;
        cfg.price_range = args.price_range;
        cfg.max_qty = args.max_qty;
        cfg.seed = args.seed;
        cfg.buy_ratio = args.buy_ratio;

        lob::run_simulation(cfg, [&](const lob::Order& order) {
            engine.process(order, trades);
            ++processed;
            if (!args.keep_trades) {
                trades.clear();
            }
        });
    }

    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    const auto secs = elapsed.count();
    const auto msg_per_sec = secs > 0.0 ? static_cast<double>(processed) / secs : 0.0;

    std::cout << "Processed " << processed << " orders in " << secs << "s ("
              << static_cast<std::uint64_t>(msg_per_sec) << " msg/s)\n";

    latency.report(std::cout);

    if (args.print_book) {
        engine.book().dump(std::cout, args.book_depth);
    }

    if (!args.dump_data_dir.empty()) {
        const auto& dir = args.dump_data_dir;

        // Write trades CSV
        {
            std::ofstream f(dir + "/trades.csv");
            f << "trade_idx,taker_id,maker_id,price,qty\n";
            for (std::size_t i = 0; i < trades.size(); ++i) {
                const auto& t = trades[i];
                f << i << "," << t.taker_id << "," << t.maker_id
                  << "," << t.price << "," << t.qty << "\n";
            }
        }

        // Write latency CSV
        {
            std::ofstream f(dir + "/latency.csv");
            latency.dump_csv(f);
        }

        // Write order book CSV
        {
            std::ofstream f(dir + "/book.csv");
            engine.book().dump_csv(f);
        }

        std::cout << "Data dumped to " << dir << "/\n";
    }

    return 0;
}
