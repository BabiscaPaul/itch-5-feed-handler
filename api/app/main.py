from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.db import db_lifespan
from app.routers import bbo, health, runs, trades
from app.settings import get_settings

settings = get_settings()

app = FastAPI(
    title="ITCH Feed Handler API",
    description="Read-only REST API over per-run ITCH parser output (BBO + trades).",
    version="0.1.0",
    lifespan=db_lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origins,
    allow_methods=["GET"],
    allow_headers=["*"],
)

app.include_router(health.router)
app.include_router(runs.router)
app.include_router(bbo.router)
app.include_router(trades.router)
