import duckdb
from fastapi import APIRouter, Depends, HTTPException

from app.db import get_db
from app.models import RunDetail, RunSummary, SymbolStat
from app.parquet_cache import ensure_parquet, ensure_run_exists
from app.runs_index import list_run_ids, run_files_with_sizes, total_size_bytes
from app.time_utils import ns_to_iso
from app.validators import RunIdPath

router = APIRouter(prefix="/runs", tags=["runs"])


def _run_counts(db: duckdb.DuckDBPyConnection, run_id: str) -> tuple[int, int, int]:
    bbo_pq = ensure_parquet(db, run_id, "bbo")
    trades_pq = ensure_parquet(db, run_id, "trades")
    row = db.execute(
        f"""
        SELECT
            (SELECT COUNT(*) FROM '{bbo_pq}')                  AS bbo_rows,
            (SELECT COUNT(*) FROM '{trades_pq}')               AS trades_rows,
            (SELECT COUNT(DISTINCT symbol) FROM '{trades_pq}') AS symbols
        """
    ).fetchone()
    return int(row[0]), int(row[1]), int(row[2])


@router.get("", response_model=list[RunSummary])
def list_runs(db: duckdb.DuckDBPyConnection = Depends(get_db)) -> list[RunSummary]:
    out: list[RunSummary] = []
    for run_id in list_run_ids():
        bbo_rows, trades_rows, symbols = _run_counts(db, run_id)
        out.append(
            RunSummary(
                run_id=run_id,
                bbo_rows=bbo_rows,
                trades_rows=trades_rows,
                symbols=symbols,
                size_bytes=total_size_bytes(run_id),
            )
        )
    return out


@router.get("/{run_id}", response_model=RunDetail)
def get_run(
    run_id: RunIdPath,
    db: duckdb.DuckDBPyConnection = Depends(get_db),
) -> RunDetail:
    ensure_run_exists(run_id)
    bbo_pq = ensure_parquet(db, run_id, "bbo")
    trades_pq = ensure_parquet(db, run_id, "trades")

    row = db.execute(
        f"""
        SELECT
            (SELECT COUNT(*) FROM '{bbo_pq}')                  AS bbo_rows,
            (SELECT COUNT(*) FROM '{trades_pq}')               AS trades_rows,
            (SELECT COUNT(DISTINCT symbol) FROM '{trades_pq}') AS symbols,
            LEAST(
                (SELECT MIN(timestamp) FROM '{bbo_pq}'),
                (SELECT MIN(timestamp) FROM '{trades_pq}')
            )                                                  AS first_ts,
            GREATEST(
                (SELECT MAX(timestamp) FROM '{bbo_pq}'),
                (SELECT MAX(timestamp) FROM '{trades_pq}')
            )                                                  AS last_ts
        """
    ).fetchone()

    bbo_rows, trades_rows, symbols, first_ts, last_ts = row
    return RunDetail(
        run_id=run_id,
        bbo_rows=int(bbo_rows),
        trades_rows=int(trades_rows),
        symbols=int(symbols),
        first_ts_ns=int(first_ts),
        first_ts=ns_to_iso(int(first_ts)),
        last_ts_ns=int(last_ts),
        last_ts=ns_to_iso(int(last_ts)),
        files=run_files_with_sizes(run_id),
    )


@router.get("/{run_id}/symbols", response_model=list[SymbolStat])
def list_symbols(
    run_id: RunIdPath,
    db: duckdb.DuckDBPyConnection = Depends(get_db),
) -> list[SymbolStat]:
    trades_pq = ensure_parquet(db, run_id, "trades")

    rows = db.execute(
        f"""
        SELECT
            symbol,
            COUNT(*)                                          AS trades,
            SUM(shares)                                       AS shares,
            SUM(CAST(price AS DOUBLE) * shares) / SUM(shares) AS vwap,
            FIRST(price ORDER BY timestamp)                   AS first_price,
            LAST(price ORDER BY timestamp)                    AS last_price,
            MIN(timestamp)                                    AS first_ts_ns,
            MAX(timestamp)                                    AS last_ts_ns
        FROM '{trades_pq}'
        GROUP BY symbol
        ORDER BY trades DESC
        """
    ).fetchall()

    if not rows:
        raise HTTPException(status_code=404, detail=f"No trades found for run {run_id}")

    return [
        SymbolStat(
            symbol=symbol,
            trades=int(trades),
            shares=int(shares),
            vwap=float(vwap),
            first_price=float(first_price),
            last_price=float(last_price),
            first_ts_ns=int(first_ts_ns),
            first_ts=ns_to_iso(int(first_ts_ns)),
            last_ts_ns=int(last_ts_ns),
            last_ts=ns_to_iso(int(last_ts_ns)),
        )
        for (symbol, trades, shares, vwap, first_price, last_price, first_ts_ns, last_ts_ns) in rows
    ]
