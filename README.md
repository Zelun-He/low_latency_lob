# Low-Latency Limit Order Book & Matching Engine

A C++ limit order book that accepts simulated or stdin orders, matches by price-time priority, and reports per-order latency stats.

## Features
- Limit order book with FIFO per price level
- Price-time priority matching
- Latency measurement per order
- Simulation mode for high-throughput benchmarking
- Simple stdin order ingestion

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Run

### Simulation

```bash
./lob_engine --simulate 1000000 --base 100.00 --range 0.50 --max-qty 200 --buy-ratio 0.55 --seed 42
```

### Stdin

Orders are formatted as:

```
SIDE PRICE QTY
```

Example:

```bash
echo "B 100.05 10" | ./lob_engine --stdin
```

### Book snapshot

```bash
./lob_engine --simulate 50000 --print-book --book-depth 5
```

## Notes
- Prices are parsed as decimal values and converted to integer ticks (cents).
- Use `--keep-trades` only if you want all trade records retained in memory.


## Interactive Frontend Example (React + JavaScript)

A browser-based visual example is included in `frontend/`.

- It provides an interactable input form for limit orders (side, price, qty).
- It shows resulting output immediately: updated order book and recent trades.
- Matching behavior mirrors the project logic (price-time priority, FIFO at each price level).

Run locally:

```bash
cd frontend
python3 -m http.server 4173
```

Then open `http://localhost:4173` in your browser.

