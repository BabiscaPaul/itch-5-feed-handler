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
    # Hand out a fresh cursor per request so concurrent handlers don't
    # serialize on the single connection's internal mutex (which can wedge
    # under request bursts from the frontend).
    return request.app.state.db.cursor()
