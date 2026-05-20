import csv
import io
from collections.abc import Iterator
from typing import Literal

import duckdb
from fastapi import APIRouter, Depends, HTTPException, Query
from fastapi.responses import FileResponse, StreamingResponse

from app.db import get_db
from app.parquet_cache import ensure_parquet, ensure_run_exists
from app.validators import RunIdPath, SYMBOL_PATTERN

router = APIRouter(prefix="/runs", tags=["downloads"])

Kind = Literal["bbo", "trades"]

CSV_CHUNK_SIZE = 10_000


def _csv_stream(cursor: duckdb.DuckDBPyConnection) -> Iterator[bytes]:
    buf = io.StringIO()
    writer = csv.writer(buf)

    columns = [d[0] for d in cursor.description]
    writer.writerow(columns)
    yield buf.getvalue().encode("utf-8")
    buf.seek(0)
    buf.truncate()

    while True:
        rows = cursor.fetchmany(CSV_CHUNK_SIZE)
        if not rows:
            break
        writer.writerows(rows)
        yield buf.getvalue().encode("utf-8")
        buf.seek(0)
        buf.truncate()


@router.get("/{run_id}/files/{kind}")
def download_file(
    run_id: RunIdPath,
    kind: Kind,
    symbol: str | None = Query(
        default=None,
        pattern=SYMBOL_PATTERN,
        description="If present, return only rows for this ticker; otherwise the full file.",
        examples=["AAPL"],
    ),
    db: duckdb.DuckDBPyConnection = Depends(get_db),
):
    run_path = ensure_run_exists(run_id)
    csv_path = run_path / f"{kind}.csv"
    if not csv_path.is_file():
        raise HTTPException(status_code=404, detail=f"{kind}.csv not found for run {run_id}")

    if symbol is None:
        return FileResponse(
            csv_path,
            media_type="text/csv",
            filename=f"{run_id}_{kind}.csv",
        )

    pq_path = ensure_parquet(db, run_id, kind)
    cursor = db.execute(
        f"SELECT * FROM '{pq_path}' WHERE symbol = ? ORDER BY timestamp",
        [symbol],
    )
    return StreamingResponse(
        _csv_stream(cursor),
        media_type="text/csv",
        headers={
            "Content-Disposition": f'attachment; filename="{run_id}_{kind}_{symbol}.csv"',
        },
    )
