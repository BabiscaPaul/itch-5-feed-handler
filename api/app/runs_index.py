import re
from pathlib import Path

from app.settings import get_settings
from app.validators import RUN_ID_PATTERN

_run_id_re = re.compile(RUN_ID_PATTERN)


def list_run_ids() -> list[str]:
    data_dir = get_settings().data_dir
    if not data_dir.is_dir():
        return []
    out: list[str] = []
    for child in data_dir.iterdir():
        if not child.is_dir():
            continue
        if not _run_id_re.fullmatch(child.name):
            continue
        if not (child / "bbo.csv").exists():
            continue
        out.append(child.name)
    return sorted(out, reverse=True)


def run_files_with_sizes(run_id: str) -> dict[str, int]:
    d = get_settings().data_dir / run_id
    out: dict[str, int] = {}
    if not d.is_dir():
        return out
    for f in sorted(d.iterdir()):
        if f.is_file():
            out[f.name] = f.stat().st_size
    return out


def total_size_bytes(run_id: str) -> int:
    return sum(run_files_with_sizes(run_id).values())


def run_exists(run_id: str) -> bool:
    return run_id in list_run_ids()


def all_runs_dir() -> Path:
    return get_settings().data_dir
