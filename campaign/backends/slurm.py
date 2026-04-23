from __future__ import annotations

from campaign.job_spec import JobSpec, JobStatus


class SlurmBackend:
    """Slurm backend — not yet implemented.

    When ready, implement:
      submit  -> sbatch --parsable <script>   (one job or --array for bulk)
      poll    -> squeue -j <ids> --format=%i,%T --noheader
      cancel  -> scancel <ids>

    Key notes for implementation:
    - output_dir must be on NFS/GPFS visible from compute nodes.
    - Use --array=0-N for large seed sweeps (far more efficient than N
      individual sbatch calls).
    - Map Slurm states: RUNNING/COMPLETING->RUNNING, COMPLETED->read JSONL,
      FAILED/TIMEOUT/OOM->FAILED/TIMEOUT, PENDING/->PENDING.
    - resources dict keys: cpus, mem_mb, time_limit_min map to
      --cpus-per-task, --mem, --time respectively.
    - wall_timeout_s on JobSpec should map to sbatch --time (in minutes,
      rounded up) so Slurm enforces the limit natively rather than requiring
      a polling-based kill loop.
    """

    def submit(self, spec: JobSpec) -> str:
        raise NotImplementedError("SlurmBackend is not yet implemented")

    def poll(self, job_ids: list[str]) -> dict[str, JobStatus]:
        raise NotImplementedError("SlurmBackend is not yet implemented")

    def cancel(self, job_ids: list[str]) -> None:
        raise NotImplementedError("SlurmBackend is not yet implemented")
