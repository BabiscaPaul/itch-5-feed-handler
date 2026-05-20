from typing import Annotated

from fastapi import Path

RUN_ID_PATTERN = r"^\d{4}-\d{2}-\d{2}$"
SYMBOL_PATTERN = r"^[A-Z][A-Z0-9.\-]{0,7}$"

RunIdPath = Annotated[
    str,
    Path(pattern=RUN_ID_PATTERN, description="Trading day in YYYY-MM-DD format", examples=["2019-01-30"]),
]

SymbolPath = Annotated[
    str,
    Path(pattern=SYMBOL_PATTERN, description="Stock ticker (uppercase, up to 8 chars)", examples=["AAPL"]),
]
