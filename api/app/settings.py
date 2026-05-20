from functools import lru_cache
from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

REPO_ROOT = Path(__file__).resolve().parent.parent.parent


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_prefix="ITCH_", env_file=".env", extra="ignore")

    data_dir: Path = REPO_ROOT / "data" / "runs"
    cors_origins: list[str] = [
        "http://localhost:3000",
        "http://localhost:5173",
    ]
    engine_tz: str = "America/New_York"

    max_trades_limit: int = 5000
    max_bbo_buckets: int = 10_000


@lru_cache
def get_settings() -> Settings:
    return Settings()
