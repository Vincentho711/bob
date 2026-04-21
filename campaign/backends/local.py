from __future__ import annotations

import subprocess
import threading
from concurrent.futures import Future, ThreadPoolExecutor, wait as futures_wait, FIRST_COMPLETED
from pathlib import Path

from campaign.job_spec import JobSpec, JobStatus
from campaign.run_record import _finalise as finalise_run


class LocalBackend:
    """Runs jobs via a ThreadPoolExecutor — each thread blocks on proc.wait().

    submit() is non-blocking and respects max_workers via the executor's
    internal queue. poll() harvests completed futures. wait_for_change()
    blocks until the first running job finishes (or timeout), making the
    campaign runner event-driven rather than fixed-interval polling.

    This eliminates the O(poll_interval) per-cycle overhead that dominates
    wall time for fast local jobs.
    """

    def __init__(self, max_workers: int | None = None) -> None:
        import os
        self._executor  = ThreadPoolExecutor(max_workers=max_workers or os.cpu_count() or 1)
        self._futures:  dict[str, Future]             = {}
        self._done:     dict[str, JobStatus]          = {}
        # _started is written by worker threads (GIL-safe set.add) and read by
        # poll() on the main thread to distinguish queued-but-not-yet-started
        # futures from jobs that have actually spawned a process.
        self._started:   set[str]                     = set()
        # _procs is written by worker threads and read by cancel() on the main
        # thread, so access is guarded by a lock.
        self._procs:     dict[str, subprocess.Popen] = {}
        self._procs_lock = threading.Lock()

    # ------------------------------------------------------------------
    # Protocol methods
    # ------------------------------------------------------------------

    def submit(self, spec: JobSpec) -> str:
        future = self._executor.submit(self._run_job, spec)
        self._futures[spec.job_id] = future
        return spec.job_id

    def poll(self, job_ids: list[str]) -> dict[str, JobStatus]:
        for jid, future in list(self._futures.items()):
            if future.done():
                try:
                    self._done[jid] = future.result()
                except Exception:
                    self._done[jid] = JobStatus.ERROR
                del self._futures[jid]

        result: dict[str, JobStatus] = {}
        for jid in job_ids:
            if jid in self._done:
                result[jid] = self._done[jid]
            elif jid in self._futures:
                result[jid] = JobStatus.RUNNING if jid in self._started else JobStatus.PENDING
            else:
                result[jid] = JobStatus.PENDING
        return result

    def cancel(self, job_ids: list[str]) -> None:
        for jid in job_ids:
            future = self._futures.pop(jid, None)
            if future:
                future.cancel()
            self._started.discard(jid)
            with self._procs_lock:
                proc = self._procs.pop(jid, None)
            if proc:
                proc.terminate()
            self._done[jid] = JobStatus.ERROR

    # ------------------------------------------------------------------
    # Event-driven wait — not part of the Protocol; used by the campaign
    # runner when available. Returns as soon as any running job finishes
    # or timeout expires, whichever comes first.
    # ------------------------------------------------------------------

    def wait_for_change(self, timeout: float = 5.0) -> None:
        pending = list(self._futures.values())
        if pending:
            futures_wait(pending, timeout=timeout, return_when=FIRST_COMPLETED)

    # ------------------------------------------------------------------
    # Worker (runs in executor thread)
    # ------------------------------------------------------------------

    def _run_job(self, spec: JobSpec) -> JobStatus:
        self._started.add(spec.job_id)
        # output_dir is created by the binary via --output-dir; we create the
        # parent (batch_dir) so the temp stderr file has somewhere to land.
        spec.output_dir.parent.mkdir(parents=True, exist_ok=True)
        err_path = spec.output_dir.parent / f".{spec.job_id}.err"
        with open(err_path, "w") as err_fh:
            proc = subprocess.Popen(spec.args, stdout=subprocess.DEVNULL, stderr=err_fh)
            with self._procs_lock:
                self._procs[spec.job_id] = proc
            proc.wait()
        with self._procs_lock:
            self._procs.pop(spec.job_id, None)
        return finalise_run(spec, err_path)
