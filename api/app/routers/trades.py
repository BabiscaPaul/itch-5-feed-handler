import duckdb
from fastapi import APIRouter, Depends, Query

from app.db import get_db
from app.models import Trade, TradesPage
from app.parquet_cache import ensure_parquet
from app.settings import get_settings
from app.time_utils import NS_PER_HOUR, ns_to_iso
from app.validators import RunIdPath, SymbolPath

router = APIRouter(prefix="/runs", tags=["trades"])

NS_PER_DAY = 24 * NS_PER_HOUR


@router.get("/{run_id}/symbols/{symbol}/trades", response_model=TradesPage)
def get_trades(
    run_id: RunIdPath,
    symbol: SymbolPath,
    db: duckdb.DuckDBPyConnection = Depends(get_db),
    from_ts_ns: int = Query(0, ge=0, description="Window start (ns since midnight ET)"),
    to_ts_ns: int = Query(NS_PER_DAY, ge=0, description="Window end (ns since midnight ET, exclusive)"),
    limit: int = Query(1000, ge=1, le=get_settings().max_trades_limit),
    offset: int = Query(0, ge=0),
) -> TradesPage:
    trades_pq = ensure_parquet(db, run_id, "trades")

    total_row = db.execute(
        f"""
        SELECT COUNT(*)
        FROM '{trades_pq}'
        WHERE symbol = ?
            AND timestamp >= ?
            AND timestamp <  ?
        """,
        [symbol, from_ts_ns, to_ts_ns],
    ).fetchone()
    total = int(total_row[0])

    rows = db.execute(
        f"""
        SELECT timestamp, price, shares
        FROM '{trades_pq}'
        WHERE symbol = ?
            AND timestamp >= ?
            AND timestamp <  ?
        ORDER BY timestamp
        LIMIT ? OFFSET ?
        """,
        [symbol, from_ts_ns, to_ts_ns, limit, offset],
    ).fetchall()

    items = [
        Trade(
            ts_ns=int(ts),
            ts=ns_to_iso(int(ts)),
            price=float(price),
            shares=int(shares),
        )
        for (ts, price, shares) in rows
    ]
    return TradesPage(total=total, items=items)
