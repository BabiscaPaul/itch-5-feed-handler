# ITCH 5.0 — Message Reference

A quick reference for the messages we care about, how they're framed in the file, and what each one does to our book state.

For the full protocol spec, see `NQTVITCHSpecification.pdf` in this repo. This document only covers what's relevant to our parser.

---

## File framing

The ITCH file is a flat stream of length-prefixed messages, back to back:

```
┌─────────────┬────────────────────────┬─────────────┬────────────────────────┬─...
│  len (2 B)  │     message body       │  len (2 B)  │     message body       │
│ big-endian  │   (variable size)      │ big-endian  │                        │
└─────────────┴────────────────────────┴─────────────┴────────────────────────┘
```

- **Length prefix**: 2 bytes, big-endian, tells you how many bytes the *body* takes.
- **Body**: starts immediately after the prefix. First byte is the message type.

So to walk the file:
```
offset = 0
loop:
    len  = read 2 bytes big-endian at offset
    body = data + offset + 2
    type = body[0]
    ...dispatch on type...
    offset += 2 + len
```

All multi-byte integer fields in the body are **big-endian**. We use `read_big_endian` / `read_u16_be` / `read_timestamp` (in `src/messages.h`) to convert to host byte order.

---

## The common header

The first **11 bytes** of every message body share the same layout. This is why `read_timestamp(body + 5)` works for any message — every type has the timestamp at the same offset.

| Offset | Bytes | Field           | Meaning                                          |
|--------|-------|-----------------|--------------------------------------------------|
| 0      | 1     | type            | ASCII char identifying the message type (`'A'`, `'D'`, etc.) |
| 1–2    | 2     | stock_locate    | Numeric ID for the stock (assigned at start-of-day) |
| 3–4    | 2     | tracking_number | Internal Nasdaq tracking ID (we ignore)          |
| 5–10   | 6     | timestamp       | Nanoseconds since midnight ET (6 bytes, big-endian) |

After byte 10, the layout depends on the message type.

---

## Field types

Type aliases live in `src/messages.h`:

| Alias        | Size | Notes |
|--------------|------|-------|
| `OrderRef`   | 8 B  | Globally unique order ID for the day |
| `Timestamp`  | 8 B  | ns since midnight (stored as 6 bytes in messages, expanded to 8 in code) |
| `Price`      | 4 B  | Fixed-point: divide by 10000 for dollars (`1652500` → `$165.2500`) |
| `Shares`     | 4 B  | Share count |
| `StockLocate`| 2 B  | Symbol ID for the day (not stable across days) |
| `MatchNumber`| 8 B  | Unique ID per executed trade |

Stock symbols are stored as **8-byte ASCII fields**, right-padded with spaces (`"AAPL    "`). `read_stock` trims trailing spaces and returns a `std::string_view`.

---

## Messages we handle

### `'R'` — Stock Directory (39 bytes)

Sent at start-of-day for every listed stock. Maps a symbol string to its `stock_locate` for the session.

| Offset | Bytes | Field        |
|--------|-------|--------------|
| 0      | 1     | `'R'`        |
| 1–2    | 2     | stock_locate |
| 3–10   | -     | header       |
| 11–18  | 8     | symbol       |
| 19–38  | 20    | (other fields — financial status, ETP flag, etc. — we ignore) |

**Dispatched to:** `BookManager::on_stock_directory(locate, symbol)`
**What it does:** stash `"AAPL" → 7` in `m_symbol_to_locate` so we can look up books by symbol later.

Note: locates are NOT stable across days. Every run must rebuild this map from the file's `'R'` messages.

---

### `'A'` — Add Order (36 bytes)

A new resting limit order enters the book.

| Offset | Bytes | Field        |
|--------|-------|--------------|
| 0      | 1     | `'A'`        |
| 1–10   | -     | header       |
| 11–18  | 8     | order_ref    |
| 19     | 1     | side (`'B'` or `'S'`) |
| 20–23  | 4     | shares       |
| 24–31  | 8     | symbol       |
| 32–35  | 4     | price        |

**Dispatched to:** `BookManager::on_add(ref, side, shares, price, locate)`
**Volume:** Highest of any message type — ~40% of the file.

---

### `'F'` — Add Order with MPID (40 bytes)

Same as `'A'`, but also tags which market participant placed the order. We ignore the MPID — book reconstruction doesn't need it.

| Offset | Bytes | Field        |
|--------|-------|--------------|
| 0–35   | -     | same as `'A'`|
| 36–39  | 4     | MPID (ignored) |

**Dispatched to:** `BookManager::on_add(ref, side, shares, price, locate)` — same handler as `'A'`.

---

### `'E'` — Order Executed (31 bytes)

A resting order had some shares filled. The trade happened at the order's resting price (no price in the message).

| Offset | Bytes | Field           |
|--------|-------|-----------------|
| 0      | 1     | `'E'`           |
| 1–10   | -     | header          |
| 11–18  | 8     | order_ref       |
| 19–22  | 4     | executed_shares |
| 23–30  | 8     | match_number    |

**Dispatched to:** `BookManager::on_executed(ref, shares)`
**What it does:** reduce `shares` from the order's level, possibly erase the order if fully filled.

To get the trade price, we look up the order's price via `m_orders[ref].price` (used for `trades.csv` in Step 5).

---

### `'C'` — Order Executed with Price (36 bytes)

Like `'E'`, but the trade printed at a *different* price than the resting order's price. Used for cross trades, opening auctions, etc.

| Offset | Bytes | Field           |
|--------|-------|-----------------|
| 0      | 1     | `'C'`           |
| 1–10   | -     | header          |
| 11–18  | 8     | order_ref       |
| 19–22  | 4     | executed_shares |
| 23–30  | 8     | match_number    |
| 31     | 1     | printable (Y/N) |
| 32–35  | 4     | execution_price (the price the trade actually printed at) |

**Dispatched to:** `BookManager::on_executed_with_price(ref, shares)`
**Note:** Identical to `'E'` for book state. The execution price only matters for `trades.csv`.

---

### `'X'` — Order Cancel (23 bytes)

Owner reduced their order's size by some amount. The order stays, just smaller.

| Offset | Bytes | Field            |
|--------|-------|------------------|
| 0      | 1     | `'X'`            |
| 1–10   | -     | header           |
| 11–18  | 8     | order_ref        |
| 19–22  | 4     | cancelled_shares |

**Dispatched to:** `BookManager::on_cancel(ref, shares)`
**What it does:** identical book-state mechanics to `'E'`. Different *intent* (no trade), same effect on the level.

---

### `'D'` — Order Delete (19 bytes)

Owner cancelled the entire remaining order.

| Offset | Bytes | Field     |
|--------|-------|-----------|
| 0      | 1     | `'D'`     |
| 1–10   | -     | header    |
| 11–18  | 8     | order_ref |

**Dispatched to:** `BookManager::on_delete(ref)`
**What it does:** reduce the level by the order's *remaining* shares (after any prior partial executes), erase the order entirely.

**Important:** we use `order.shares` from `m_orders`, which always tracks remaining size — not the original size at add time.

---

### `'U'` — Order Replace (35 bytes)

Atomic delete-old + add-new. Same stock and same side as the original — only the ref/price/shares change. The new order loses time priority (which is why it gets a new ref).

| Offset | Bytes | Field          |
|--------|-------|----------------|
| 0      | 1     | `'U'`          |
| 1–10   | -     | header         |
| 11–18  | 8     | old_order_ref  |
| 19–26  | 8     | new_order_ref  |
| 27–30  | 4     | shares         |
| 31–34  | 4     | price          |

**Dispatched to:** `BookManager::on_replace(old_ref, new_ref, shares, price)`
**What it does:**
1. Look up old order, reduce its level by its *remaining* shares.
2. Erase old ref from `m_orders`.
3. Insert new ref with new shares/price, inheriting side and locate from the old order.
4. Add new shares to the new level.

Note: the message body has **no** side, locate, or symbol — we have to look those up from the old order. That's why we can't process a Replace if `old_ref` isn't already known.

---

## Messages we count but don't dispatch

These show up in the type-count summary, but our `parse_and_build` switch falls through to `default: break` for them. They don't affect visible-book state.

| Type | Name | Why we skip it |
|------|------|----------------|
| `'P'` | Trade (non-displayable) | A trade involving a hidden order. There's no Add for these, so nothing to reduce in the book. *Will* matter for `trades.csv` in Step 5. |
| `'Q'` | Cross Trade | Auction prints (opening/closing). Reported separately; the underlying order matches show up as `'E'`/`'C'`. |
| `'B'` | Broken Trade | Reverses a previously reported trade. Rare. |
| `'I'` | NOI (Net Order Imbalance) | Auction imbalance info. Not book state. |
| `'L'` | Market Participant Position | Per-firm position summaries. Not book state. |
| `'S'` | System Event | Market open/close announcements. |
| `'H'` | Stock Trading Action | Trading halts, resumes. |
| `'Y'` | Reg SHO Restriction | Short sale restriction state. |
| `'J'`, `'V'`, `'K'`, etc. | Various | Misc admin messages. |

If we need any of these later (e.g. for benchmarking comparisons or richer thesis output), we just add a case in the switch.

---

## Volume distribution (real numbers from Jan 30, 2019)

From our last full-file run (`Total: 368,366,634`):

| Type | Count       | % of file |
|------|-------------|-----------|
| `A`  | 162,970,455 | 44.2%     |
| `D`  | 158,273,361 | 43.0%     |
| `U`  | 27,222,746  | 7.4%      |
| `E`  | 8,096,995   | 2.2%      |
| `X`  | 4,669,874   | 1.3%      |
| `I`  | 3,684,511   | 1.0%      |
| `F`  | 1,725,898   | 0.5%      |
| `P`  | 1,326,184   | 0.4%      |
| ...  | (rest <1%)  |           |

Adds and Deletes alone are ~87% of the file. Most order activity is "place, then cancel" — a feature of modern algorithmic trading where firms post and pull liquidity constantly.

---

## Quick mental model

For book reconstruction, ITCH boils down to **7 verbs on a giant collection of orders**:

- `R`: "AAPL is locate 7 today" — *setup*
- `A` / `F`: "Add this order" — *create*
- `E` / `C`: "Fill these shares from this order" — *partial or full reduce*
- `X`: "Cancel these shares from this order" — *partial or full reduce*
- `D`: "Delete this order" — *full remove*
- `U`: "Replace this order with that one" — *delete + add atomically*

Everything else in the file is metadata or non-book trade reports — we count it, but it doesn't change the book.
