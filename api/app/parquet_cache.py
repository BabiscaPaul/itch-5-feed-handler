import threading
from pathlib import Path
from typing import Literal

import duckdb
from fastapi import HTTPException

from app.settings import get_settings

Kind = Literal["bbo", "trades"]

_locks: dict[tuple[str, Kind], threading.Lock] = {}
_locks_guard = threading.Lock()


def _lock_for(run_id: str, kind: Kind) -> threading.Lock:
    key = (run_id, kind)
    with _locks_guard:
        lock = _locks.get(key)
        if lock is None:
            lock = threading.Lock()
            _locks[key] = lock
        return lock


def run_dir(run_id: str) -> Path:
    return get_settings().data_dir / run_id


def ensure_run_exists(run_id: str) -> Path:
    d = run_dir(run_id)
    if not d.is_dir() or not (d / "bbo.csv").exists():
        raise HTTPException(status_code=404, detail=f"Run not found: {run_id}")
    return d


def ensure_parquet(conn: duckdb.DuckDBPyConnection, run_id: str, kind: Kind) -> Path:
    d = ensure_run_exists(run_id)
    csv_path = d / f"{kind}.csv"
    pq_path = d / f"{kind}.parquet"

    if not csv_path.exists():
        raise HTTPException(status_code=404, detail=f"{kind}.csv missing for run {run_id}")

    if pq_path.exists() and pq_path.stat().st_mtime >= csv_path.stat().st_mtime:
        return pq_path

    with _lock_for(run_id, kind):
        if pq_path.exists() and pq_path.stat().st_mtime >= csv_path.stat().st_mtime:
            return pq_path

        tmp_path = pq_path.with_suffix(".parquet.tmp")
        conn.execute(
            f"COPY (SELECT * FROM read_csv(?, header=true, AUTO_DETECT=true)) "
            f"TO '{tmp_path}' (FORMAT PARQUET, COMPRESSION ZSTD)",
            [str(csv_path)],
        )
        tmp_path.replace(pq_path)
        return pq_path
