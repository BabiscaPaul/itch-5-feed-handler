from pydantic import BaseModel, Field


class HealthResponse(BaseModel):
    status: str = Field(description="'ok' when the service is up")
    data_dir_exists: bool = Field(description="Whether the configured data directory is reachable")
    runs_count: int = Field(description="Number of run-id directories detected on disk")


class RunSummary(BaseModel):
    run_id: str = Field(description="Trading day in YYYY-MM-DD format", examples=["2019-01-30"])
    bbo_rows: int = Field(description="Number of BBO change rows")
    trades_rows: int = Field(description="Number of trade rows")
    symbols: int = Field(description="Distinct symbols present in trades.csv")
    size_bytes: int = Field(description="Total on-disk size of CSV + Parquet files for this run")


class RunDetail(BaseModel):
    run_id: str
    bbo_rows: int
    trades_rows: int
    symbols: int
    first_ts_ns: int = Field(description="Earliest timestamp in this run (ns since midnight ET)")
    first_ts: str = Field(description="Earliest timestamp formatted as HH:MM:SS.fffffffff")
    last_ts_ns: int = Field(description="Latest timestamp in this run (ns since midnight ET)")
    last_ts: str = Field(description="Latest timestamp formatted as HH:MM:SS.fffffffff")
    files: dict[str, int] = Field(description="Map of filename -> size in bytes for this run")


class SymbolStat(BaseModel):
    symbol: str
    trades: int = Field(description="Number of trade prints for this symbol")
    shares: int = Field(description="Total shares traded")
    vwap: float = Field(description="Volume-weighted average price across all trades")
    first_price: float
    last_price: float
    first_ts_ns: int
    first_ts: str
    last_ts_ns: int
    last_ts: str


class BboBucket(BaseModel):
    bucket_ts_ns: int = Field(description="Start of the time bucket (ns since midnight ET)")
    ts: str = Field(description="Bucket start formatted as HH:MM:SS.fffffffff")
    bid_open: float = Field(description="First bid price observed in this bucket")
    bid_close: float = Field(description="Last bid price observed in this bucket")
    ask_open: float
    ask_close: float
    spread_avg: float = Field(description="Mean of (ask - bid) across BBO updates in this bucket")
    samples: int = Field(description="Number of underlying BBO updates aggregated into this bucket")


class Trade(BaseModel):
    ts_ns: int
    ts: str
    price: float
    shares: int


class TradesPage(BaseModel):
    total: int = Field(description="Total trades matching the filter, before pagination")
    items: list[Trade]
