"""
Low-Latency LOB Visualizer
Reads CSV data produced by lob_engine --dump-data and displays 3 charts:
  1. Order Book Depth Chart
  2. Latency Distribution Histogram
  3. Trade Price Over Time
"""

import csv
import os
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np


def read_csv(filepath):
    """Read a CSV file and return a list of dicts."""
    with open(filepath, newline="") as f:
        return list(csv.DictReader(f))


def load_data(data_dir):
    """Load all CSV files from the data directory."""
    data_dir = Path(data_dir)
    book = read_csv(data_dir / "book.csv")
    latency = read_csv(data_dir / "latency.csv")
    trades = read_csv(data_dir / "trades.csv")
    return book, latency, trades


def plot_depth_chart(ax, book_rows):
    """Plot the order book depth chart (cumulative quantity at each price level)."""
    bids = [(int(r["price"]), int(r["total_qty"])) for r in book_rows if r["side"] == "BID"]
    asks = [(int(r["price"]), int(r["total_qty"])) for r in book_rows if r["side"] == "ASK"]

    # Sort bids descending by price, asks ascending
    bids.sort(key=lambda x: -x[0])
    asks.sort(key=lambda x: x[0])

    if bids:
        bid_prices = [p / 100.0 for p, _ in bids]
        bid_cum = list(np.cumsum([q for _, q in bids]))
        ax.fill_between(bid_prices, bid_cum, alpha=0.4, color="#22c55e", step="pre")
        ax.step(bid_prices, bid_cum, where="pre", color="#16a34a", linewidth=1.5, label="Bids")

    if asks:
        ask_prices = [p / 100.0 for p, _ in asks]
        ask_cum = list(np.cumsum([q for _, q in asks]))
        ax.fill_between(ask_prices, ask_cum, alpha=0.4, color="#ef4444", step="pre")
        ax.step(ask_prices, ask_cum, where="pre", color="#dc2626", linewidth=1.5, label="Asks")

    ax.set_title("Order Book Depth", fontsize=14, fontweight="bold")
    ax.set_xlabel("Price ($)")
    ax.set_ylabel("Cumulative Quantity")
    ax.legend(loc="upper center", framealpha=0.9)
    ax.grid(True, alpha=0.3)
    ax.xaxis.set_major_formatter(ticker.FormatStrFormatter("%.2f"))

    # Zoom to the interesting range around the spread
    all_prices = [p / 100.0 for p, _ in bids + asks]
    if all_prices:
        mid = (min(all_prices) + max(all_prices)) / 2
        span = max(all_prices) - min(all_prices)
        margin = max(span * 0.05, 0.10)
        ax.set_xlim(min(all_prices) - margin, max(all_prices) + margin)


def plot_latency_histogram(ax, latency_rows):
    """Plot the latency distribution histogram."""
    samples = np.array([int(r["sample_ns"]) for r in latency_rows], dtype=np.float64)

    if len(samples) == 0:
        ax.text(0.5, 0.5, "No latency data", ha="center", va="center", transform=ax.transAxes)
        return

    # Clip extreme outliers for better visualization (show up to p99.5)
    clip_val = np.percentile(samples, 99.5)
    display = samples[samples <= clip_val]

    p50 = np.percentile(samples, 50)
    p90 = np.percentile(samples, 90)
    p99 = np.percentile(samples, 99)
    avg = np.mean(samples)

    ax.hist(display, bins=80, color="#6366f1", alpha=0.75, edgecolor="#4338ca", linewidth=0.3)

    # Draw percentile lines
    for val, label, color in [
        (p50, f"p50={int(p50)}ns", "#f59e0b"),
        (p90, f"p90={int(p90)}ns", "#f97316"),
        (p99, f"p99={int(p99)}ns", "#ef4444"),
    ]:
        ax.axvline(val, color=color, linestyle="--", linewidth=1.5, label=label)

    ax.set_title("Latency Distribution", fontsize=14, fontweight="bold")
    ax.set_xlabel("Latency (ns)")
    ax.set_ylabel("Count")
    ax.legend(loc="upper right", framealpha=0.9)
    ax.grid(True, alpha=0.3, axis="y")

    # Add summary text
    summary = f"n={len(samples):,}  avg={int(avg)}ns  min={int(samples.min())}ns  max={int(samples.max())}ns"
    ax.text(
        0.02, 0.95, summary,
        transform=ax.transAxes, fontsize=8,
        verticalalignment="top",
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.8),
    )


def plot_trade_prices(ax, trade_rows):
    """Plot trade execution prices over time."""
    if not trade_rows:
        ax.text(0.5, 0.5, "No trade data", ha="center", va="center", transform=ax.transAxes)
        return

    indices = [int(r["trade_idx"]) for r in trade_rows]
    prices = [int(r["price"]) / 100.0 for r in trade_rows]
    qtys = [int(r["qty"]) for r in trade_rows]

    # Downsample if too many trades for scatter
    n = len(prices)
    if n > 5000:
        # Use a moving average line instead of scatter for large datasets
        prices_arr = np.array(prices)
        window = max(n // 500, 10)
        kernel = np.ones(window) / window
        smoothed = np.convolve(prices_arr, kernel, mode="valid")
        smooth_x = np.arange(len(smoothed)) + window // 2

        ax.plot(smooth_x, smoothed, color="#6366f1", linewidth=1.0, label=f"MA({window})")

        # Also show raw as faint scatter
        ax.scatter(
            indices, prices,
            s=0.3, alpha=0.08, c="#94a3b8", rasterized=True,
        )
        ax.legend(loc="upper right", framealpha=0.9)
    else:
        # Color by quantity
        sc = ax.scatter(
            indices, prices,
            s=np.clip(np.array(qtys) * 0.5, 2, 30),
            c=qtys, cmap="YlOrRd", alpha=0.6, edgecolors="none",
        )
        plt.colorbar(sc, ax=ax, label="Quantity", pad=0.02, shrink=0.8)

    ax.set_title("Trade Prices Over Time", fontsize=14, fontweight="bold")
    ax.set_xlabel("Trade Index")
    ax.set_ylabel("Price ($)")
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.2f"))
    ax.grid(True, alpha=0.3)

    # Show spread info
    if prices:
        ax.text(
            0.02, 0.95,
            f"{len(prices):,} trades  |  price range: ${min(prices):.2f} – ${max(prices):.2f}",
            transform=ax.transAxes, fontsize=8,
            verticalalignment="top",
            bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.8),
        )


def main():
    data_dir = sys.argv[1] if len(sys.argv) > 1 else "data"

    if not os.path.isdir(data_dir):
        print(f"Error: data directory '{data_dir}' not found.")
        print("Run:  lob_engine --simulate 100000 --dump-data data")
        sys.exit(1)

    print(f"Loading data from {data_dir}/...")
    book, latency, trades = load_data(data_dir)
    print(f"  Book levels: {len(book)}")
    print(f"  Latency samples: {len(latency)}")
    print(f"  Trades: {len(trades)}")

    # Set up the figure
    plt.style.use("seaborn-v0_8-whitegrid")
    fig, axes = plt.subplots(1, 3, figsize=(20, 6))
    fig.suptitle("Low-Latency Limit Order Book — Dashboard", fontsize=16, fontweight="bold", y=1.02)

    plot_depth_chart(axes[0], book)
    plot_latency_histogram(axes[1], latency)
    plot_trade_prices(axes[2], trades)

    plt.tight_layout()
    plt.savefig(os.path.join(data_dir, "dashboard.png"), dpi=150, bbox_inches="tight")
    print(f"\nSaved dashboard to {data_dir}/dashboard.png")
    plt.show()


if __name__ == "__main__":
    main()
