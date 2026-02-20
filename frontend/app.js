const { useMemo, useState } = React;

function emptyBook() {
  return { bids: new Map(), asks: new Map(), trades: [], nextId: 1 };
}

function cloneLevel(level) {
  return { totalQty: level.totalQty, queue: level.queue.map((x) => ({ ...x })) };
}

function processOrder(book, order) {
  const next = {
    bids: new Map(Array.from(book.bids.entries(), ([p, l]) => [p, cloneLevel(l)])),
    asks: new Map(Array.from(book.asks.entries(), ([p, l]) => [p, cloneLevel(l)])),
    trades: [...book.trades],
    nextId: book.nextId + 1,
  };

  const incoming = { ...order, id: book.nextId };
  const sideBook = incoming.side === "BUY" ? next.asks : next.bids;
  const prices = Array.from(sideBook.keys()).sort((a, b) => (incoming.side === "BUY" ? a - b : b - a));

  for (const price of prices) {
    if (incoming.qty <= 0) break;
    if (incoming.side === "BUY" && incoming.price < price) break;
    if (incoming.side === "SELL" && incoming.price > price) break;

    const level = sideBook.get(price);
    while (incoming.qty > 0 && level.queue.length) {
      const maker = level.queue[0];
      const execQty = Math.min(incoming.qty, maker.qty);
      incoming.qty -= execQty;
      maker.qty -= execQty;
      level.totalQty -= execQty;
      next.trades.unshift({ takerId: incoming.id, makerId: maker.id, price, qty: execQty });
      if (maker.qty === 0) level.queue.shift();
    }
    if (level.queue.length === 0) sideBook.delete(price);
  }

  if (incoming.qty > 0) {
    const targetBook = incoming.side === "BUY" ? next.bids : next.asks;
    const level = targetBook.get(incoming.price) ?? { totalQty: 0, queue: [] };
    level.totalQty += incoming.qty;
    level.queue.push({ id: incoming.id, qty: incoming.qty, ts: Date.now() });
    targetBook.set(incoming.price, level);
  }

  return next;
}

function levelsFrom(bookSide, desc = false) {
  return Array.from(bookSide.entries())
    .sort((a, b) => (desc ? b[0] - a[0] : a[0] - b[0]))
    .map(([price, level]) => ({ price, totalQty: level.totalQty, count: level.queue.length }));
}

function App() {
  const [book, setBook] = useState(emptyBook);
  const [side, setSide] = useState("BUY");
  const [price, setPrice] = useState(100.0);
  const [qty, setQty] = useState(10);

  const bids = useMemo(() => levelsFrom(book.bids, true), [book]);
  const asks = useMemo(() => levelsFrom(book.asks, false), [book]);

  const onSubmit = (e) => {
    e.preventDefault();
    if (!Number.isFinite(price) || price <= 0 || !Number.isFinite(qty) || qty <= 0) return;

    setBook((prev) =>
      processOrder(prev, {
        side,
        price: Math.round(price * 100),
        qty: Math.floor(qty),
      }),
    );
  };

  const loadScenario = () => {
    let next = emptyBook();
    [["BUY", 100.0, 20], ["BUY", 99.8, 15], ["SELL", 100.3, 30], ["SELL", 100.5, 25], ["BUY", 100.4, 40]].forEach(
      ([s, p, q]) => {
        next = processOrder(next, { side: s, price: Math.round(p * 100), qty: q });
      },
    );
    setBook(next);
  };

  return (
    <div className="app">
      <h1>Interactive Limit Order Book Example</h1>
      <p className="muted">Submit BUY/SELL limit orders and immediately see matches and updated book levels.</p>
      <form className="card" onSubmit={onSubmit}>
        <div className="row">
          <label>Side
            <select value={side} onChange={(e) => setSide(e.target.value)}><option>BUY</option><option>SELL</option></select>
          </label>
          <label>Price
            <input type="number" step="0.01" value={price} onChange={(e) => setPrice(Number(e.target.value))} />
          </label>
          <label>Quantity
            <input type="number" step="1" value={qty} onChange={(e) => setQty(Number(e.target.value))} />
          </label>
          <label>&nbsp;
            <button type="submit">Submit Order</button>
          </label>
        </div>
        <div className="chips">
          <button type="button" onClick={loadScenario}>Load Sample Scenario</button>
          <button type="button" onClick={() => setBook(emptyBook())}>Reset Book</button>
          <span className="chip">Best Bid: {bids[0] ? (bids[0].price / 100).toFixed(2) : "-"}</span>
          <span className="chip">Best Ask: {asks[0] ? (asks[0].price / 100).toFixed(2) : "-"}</span>
        </div>
      </form>

      <div className="tables">
        <div className="card">
          <h3>Order Book</h3>
          <table>
            <thead><tr><th>Side</th><th>Price</th><th>Total Qty</th><th>Orders</th></tr></thead>
            <tbody>
              {bids.map((l) => <tr key={`b-${l.price}`}><td>BUY</td><td>{(l.price / 100).toFixed(2)}</td><td>{l.totalQty}</td><td>{l.count}</td></tr>)}
              {asks.map((l) => <tr key={`a-${l.price}`}><td>SELL</td><td>{(l.price / 100).toFixed(2)}</td><td>{l.totalQty}</td><td>{l.count}</td></tr>)}
            </tbody>
          </table>
        </div>
        <div className="card">
          <h3>Recent Trades</h3>
          <table>
            <thead><tr><th>Taker</th><th>Maker</th><th>Price</th><th>Qty</th></tr></thead>
            <tbody>
              {book.trades.slice(0, 12).map((t, i) => <tr key={`${t.takerId}-${t.makerId}-${i}`}><td>{t.takerId}</td><td>{t.makerId}</td><td>{(t.price / 100).toFixed(2)}</td><td>{t.qty}</td></tr>)}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
