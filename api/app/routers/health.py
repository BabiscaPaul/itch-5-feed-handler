from fastapi import APIRouter

from app.models import HealthResponse
from app.runs_index import all_runs_dir, list_run_ids

router = APIRouter(tags=["health"])


@router.get("/health", response_model=HealthResponse)
def health() -> HealthResponse:
    return HealthResponse(
        status="ok",
        data_dir_exists=all_runs_dir().is_dir(),
        runs_count=len(list_run_ids()),
    )
