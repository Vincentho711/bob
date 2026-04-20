from __future__ import annotations

from typing import Protocol

from campaign.job_spec import JobSpec, JobStatus


class JobBackend(Protocol):
    def submit(self, spec: JobSpec) -> str:
        """Enqueue a job. Returns the job_id. Non-blocking."""
        ...

    def poll(self, job_ids: list[str]) -> dict[str, JobStatus]:
        """Return current status for each requested job_id.
        Also responsible for draining any internal pending queue into
        available execution slots."""
        ...

    def cancel(self, job_ids: list[str]) -> None:
        """Best-effort cancellation."""
        ...
