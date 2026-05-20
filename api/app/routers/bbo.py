from typing import Literal

import duckdb
from fastapi import APIRouter, Depends, HTTPException, Query

from app.db import get_db
from app.models import BboBucket
from app.parquet_cache import ensure_parquet
from app.settings import get_settings
from app.time_utils import NS_PER_HOUR, ns_to_iso, regular_hours_window
from app.validators import RunIdPath, SymbolPath

router = APIRouter(prefix="/runs", tags=["bbo"])

Resolution = Literal["100ms", "1s", "1m", "5m"]

_RESOLUTION_NS: dict[Resolution, int] = {
    "100ms": 100_000_000,
    "1s": 1_000_000_000,
    "1m": 60_000_000_000,
    "5m": 300_000_000_000,
}

NS_PER_DAY = 24 * NS_PER_HOUR


@router.get("/{run_id}/symbols/{symbol}/bbo", response_model=list[BboBucket])
def get_bbo(
    run_id: RunIdPath,
    symbol: SymbolPath,
    db: duckdb.DuckDBPyConnection = Depends(get_db),
    from_ts_ns: int | None = Query(None, ge=0, description="Window start (ns since midnight ET). Defaults to start of day or regular open."),
    to_ts_ns: int | None = Query(None, ge=0, description="Window end (ns since midnight ET, exclusive). Defaults to end of day or regular close."),
    resolution: Resolution = Query("1s", description="Bucket size for downsampling"),
    regular_hours: bool = Query(False, description="If true, restrict to 09:30:00–16:00:00 ET regular session"),
) -> list[BboBucket]:
    
    open_ns, close_ns = regular_hours_window()
    if regular_hours:
        win_start = open_ns if from_ts_ns is None else max(from_ts_ns, open_ns)
        win_end = close_ns if to_ts_ns is None else min(to_ts_ns, close_ns)
    else:
        win_start = 0 if from_ts_ns is None else from_ts_ns
        win_end = NS_PER_DAY if to_ts_ns is None else to_ts_ns

    if win_end <= win_start:
        raise HTTPException(status_code=400, detail=f"to_ts_ns ({win_end}) must be greater than from_ts_ns ({win_start})")

    bucket_ns = _RESOLUTION_NS[resolution]
    expected_buckets = (win_end - win_start + bucket_ns - 1) // bucket_ns
    max_buckets = get_settings().max_bbo_buckets
    if expected_buckets > max_buckets:
        raise HTTPException(
            status_code=400,
            detail=(
                f"Requested {expected_buckets} buckets exceeds limit of {max_buckets}. "
                f"Narrow the time window or use a coarser resolution."
            ),
        )

    bbo_pq = ensure_parquet(db, run_id, "bbo")

    rows = db.execute(
        f"""
        WITH windowed AS (
            SELECT
                timestamp,
                (timestamp // {bucket_ns}) * {bucket_ns} AS bucket_ts_ns,
                bid_price, ask_price, spread
            FROM '{bbo_pq}'
            WHERE symbol = ?
            AND timestamp >= ?
            AND timestamp <  ?
        )
        SELECT
            bucket_ts_ns,
            FIRST(bid_price ORDER BY timestamp) AS bid_open,
            LAST(bid_price  ORDER BY timestamp) AS bid_close,
            FIRST(ask_price ORDER BY timestamp) AS ask_open,
            LAST(ask_price  ORDER BY timestamp) AS ask_close,
            AVG(spread) AS spread_avg,
            COUNT(*) AS samples
        FROM windowed
        GROUP BY bucket_ts_ns
        ORDER BY bucket_ts_ns
        """,
        [symbol, win_start, win_end],
    ).fetchall()

    return [
        BboBucket(
            bucket_ts_ns=int(bucket_ts_ns),
            ts=ns_to_iso(int(bucket_ts_ns)),
            bid_open=float(bid_open),
            bid_close=float(bid_close),
            ask_open=float(ask_open),
            ask_close=float(ask_close),
            spread_avg=float(spread_avg),
            samples=int(samples),
        )
        for (bucket_ts_ns, bid_open, bid_close, ask_open, ask_close, spread_avg, samples) in rows
    ]
