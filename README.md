# itch-feed-handler

> Bachelor thesis project — TU Cluj, 2026. Engine, API, and web are all wired up end-to-end; engine benchmarking is the remaining work.

An end-to-end pipeline for Nasdaq ITCH 5.0 binary market data: a C++23 feed handler that parses ~10 GB of raw messages into per-symbol order books, a FastAPI + DuckDB service that exposes the output as a typed JSON API, and a React SPA for browsing it.

```
ITCH file → engine (C++23) → data/runs/<day>/ → api (FastAPI + DuckDB) → web (React + Vite)
                              bbo.csv, trades.csv         localhost:8000           localhost:5173
```

The three apps are decoupled: the engine writes CSV to a shared `data/` directory, the API reads it via DuckDB (caching to Parquet on first query), the web frontend talks to the API over HTTP. No app calls another's code directly.

## Project Layout

```
itch-feed-handler/
├── engine/             # C++23 feed handler — batch CLI
│   ├── src/            # parser, order book, book manager, CSV writer
│   ├── tests/          # GoogleTest suites
│   ├── feeds/          # raw ITCH input (gitignored)
│   └── symbols.txt     # tickers to emit CSV for
├── api/                # FastAPI + DuckDB read-only service
│   └── app/            # routers: runs, bbo, trades, downloads, health
├── web/                # React 19 + Vite SPA
│   └── src/            # pages: Overview, BBO Explorer, Trades Tape, Benchmarks
├── data/               # engine output, keyed by run-id (gitignored)
│   └── runs/<YYYY-MM-DD>/
│       ├── bbo.csv
│       ├── trades.csv
│       └── *.parquet   # generated lazily by the API
└── docker-compose.yml  # brings up api + web for local dev
```

## Status

| Phase | Status |
|-------|--------|
| Engine — parser, order book, CSV output | Done |
| API — FastAPI + DuckDB, BBO / trades / downloads | Done |
| Web — Overview, BBO Explorer, Trades Tape | Done |
| Engine — benchmarking (SPSC queue, latency, `meta.json`) | In progress |
| Web — Benchmarks page (consumes engine `meta.json`) | Placeholder |

## Quickstart

The engine runs locally (it needs a native build and the raw ITCH file on disk); the API and web run via Docker.

### 1. Produce a run with the engine

Requires CMake 3.20+ and a C++23 compiler.

```bash
cd engine/
cmake -S . -B build
cmake --build build

# Filename → run-id is auto-derived. Output goes to ../data/runs/<YYYY-MM-DD>/.
./run.sh feeds/01302019.NASDAQ_ITCH50

# Tests:
ctest --test-dir build --output-on-failure
```

Edit `engine/symbols.txt` to control which tickers get written to `bbo.csv` / `trades.csv`. The engine builds books for *all* symbols regardless — only CSV output is filtered.

### 2. Browse it via the API + web

```bash
docker compose up --build
```

- API: <http://localhost:8000> (OpenAPI docs at `/docs`)
- Web: <http://localhost:5173>

Both services bind-mount their source for hot reload, and the API bind-mounts `./data` so it sees whatever runs the engine has produced.

### Running api / web without Docker

```bash
# API (needs uv: https://docs.astral.sh/uv/)
cd api/
uv sync
uv run uvicorn app.main:app --reload

# Web
cd web/
npm install
npm run dev
```

The web app reads `VITE_API_URL` (default `http://localhost:8000`, see `web/.env.development`). Regenerate the typed API client after the schema changes:

```bash
cd web/
npm run gen:types   # writes src/api/schema.ts from the live OpenAPI doc
```

## API Surface

Read-only REST. Every endpoint is scoped by `run_id` (the trading day).

| Method | Path | Returns |
|--------|------|---------|
| GET | `/health` | service + data-dir status |
| GET | `/runs` | all runs on disk with row counts and size |
| GET | `/runs/{run_id}` | per-run detail + file sizes + time range |
| GET | `/runs/{run_id}/symbols` | per-symbol stats (trades, shares, VWAP, first/last price) |
| GET | `/runs/{run_id}/symbols/{symbol}/bbo` | bucketed BBO time series (`100ms` / `1s` / `1m` / `5m`) |
| GET | `/runs/{run_id}/symbols/{symbol}/trades` | paginated trade prints |
| GET | `/runs/{run_id}/files/{kind}` | raw CSV download (`bbo` or `trades`), optionally filtered by symbol |

Time-series queries hit DuckDB over a Parquet snapshot of the run's CSV. The Parquet file is generated the first time it's needed and reused until the CSV changes.

## Per-Run Data Model

One ITCH file → one run, identified by its trading day (`2019-01-30`). Every endpoint takes this as `run_id`. All timestamps are nanoseconds since midnight ET (the engine's native unit); the API also returns ISO-formatted versions for display.

## Tech Stack

| Layer | Tech |
|-------|------|
| Engine | C++23, `mmap`, GoogleTest |
| API | Python 3.12, FastAPI, DuckDB, Pydantic, uv |
| Web | TypeScript, React 19, Vite, Mantine, TanStack Query, Zustand, Lightweight Charts |
| Storage | CSV on disk (engine output) + Parquet cache (API, generated lazily). No DB server. |
| Orchestration | Docker Compose (api + web) |
