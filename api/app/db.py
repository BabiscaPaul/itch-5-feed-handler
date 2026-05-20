from collections.abc import AsyncIterator
from contextlib import asynccontextmanager

import duckdb
from fastapi import FastAPI, Request


@asynccontextmanager
async def db_lifespan(app: FastAPI) -> AsyncIterator[None]:
    conn = duckdb.connect(":memory:")
    app.state.db = conn
    try:
        yield
    finally:
        conn.close()


def get_db(request: Request) -> duckdb.DuckDBPyConnection:
    return request.app.state.db
