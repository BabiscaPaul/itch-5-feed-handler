# itch-feed-handler

> Bachelor thesis project — TU Cluj, 2026. Active development; structure and tech choices may evolve as the API and web phases come online. 

A C++23 feed handler for Nasdaq ITCH 5.0 binary market data, paired with a REST API and a web frontend for analytical exploration.

The engine parses ~10 GB of raw ITCH messages, reconstructs per-symbol order books, and writes time-series data to disk. A Python/FastAPI service layers SQL-style queries on top via DuckDB. A web frontend visualizes the results.

## Project Structure

```
itch-feed-handler/
├── engine/   # C++23 — the feed handler (current focus)
├── api/      # Python + FastAPI + DuckDB (next phase)
├── web/      # TypeScript SPA (final phase)
├── data/     # Engine output, organized by run-id (gitignored)
└── docs/     # System-wide documentation
```

## Status

| Phase | Status |
|-------|--------|
| Engine — parser, order book, CSV output | Done |
| Engine — SPSC queue, latency benchmarking | In progress |
| API (FastAPI + DuckDB) | Planned |
| Web frontend | Planned |

## Quickstart (engine)

Requires CMake 3.20+ and a C++23 compiler.

```bash
cd engine/
cmake -S . -B build
cmake --build build

# Run against an ITCH file (filename → run-id is auto-derived):
./run.sh feeds/01302019.NASDAQ_ITCH50
# → writes data/runs/2019-01-30/{bbo.csv,trades.csv}

# Run the test suite:
ctest --test-dir build --output-on-failure
```

## Tech Stack

| Layer | Tech |
|-------|------|
| Engine | C++23, GoogleTest |
| API | Python 3 + FastAPI + DuckDB |
| Web | TypeScript + React |
| Storage | CSV / Parquet on disk — no DB server |
