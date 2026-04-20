"""Live terminal UI for campaign progress using rich.

Auto-disables when stdout is not a TTY. Uses mtime-based hung detection:
a single stat() per running job per refresh cycle — no file tailing, no
background threads, no NFS-unfriendly sustained reads.
"""
from __future__ import annotations

import sys
import time
from pathlib import Path

from rich import box
from rich.console import Console
from rich.live import Live
from rich.table import Table
from rich.text import Text

from campaign.job_spec import JobSpec, JobStatus


_HUNG_THRESHOLD_S: float = 10.0

_STATUS_STYLE: dict[JobStatus, str] = {
    JobStatus.PENDING:  "dim",
    JobStatus.RUNNING:  "cyan",
    JobStatus.PASSED:   "green",
    JobStatus.FAILED:   "bold red",
    JobStatus.ERROR:    "bold red",
    JobStatus.TIMEOUT:  "yellow",
    JobStatus.UNKNOWN:  "dim",
}


def _progress_path(spec: JobSpec) -> Path:
    return spec.output_dir / "progress.jsonl"


def _mtime_age_s(path: Path) -> float | None:
    """Seconds since last modification, or None if the file does not yet exist."""
    try:
        return time.time() - path.stat().st_mtime
    except OSError:
        return None


class _Renderer:
    """Builds the rich Table rendered inside the Live context.

    All mutable state (job_start/end times) is owned by CampaignTUI and
    passed in on each build() call so this object is stateless and cheap
    to reason about.
    """

    def __init__(
        self,
        specs:            list[JobSpec],
        batch_run_id:     str,
        hung_threshold_s: float,
    ) -> None:
        self._specs            = specs
        self._batch_run_id     = batch_run_id
        self._hung_threshold_s = hung_threshold_s
        self._batch_start      = time.monotonic()
        # Pre-compute expected paths once so build() does no string allocation.
        self._progress_paths: dict[str, Path] = {
            s.job_id: _progress_path(s) for s in specs
        }

    def build(
        self,
        statuses:  dict[str, JobStatus],
        job_start: dict[str, float],
        job_end:   dict[str, float],
    ) -> Table:
        elapsed       = time.monotonic() - self._batch_start
        mins, secs    = divmod(int(elapsed), 60)
        done          = sum(1 for s in statuses.values() if s.is_terminal)
        total         = len(self._specs)

        table = Table(
            box=box.SIMPLE_HEAD,
            header_style="bold white",
            title=(
                f"[bold]{self._batch_run_id}[/bold]"
                f"  [dim]{done}/{total} done  {mins:02d}:{secs:02d}[/dim]"
            ),
            title_justify="left",
            show_footer=False,
            expand=False,
        )
        table.add_column("TEST",   no_wrap=True, min_width=18)
        table.add_column("SEED",   no_wrap=True, min_width=18, style="dim")
        table.add_column("STATUS", no_wrap=True, min_width=9)
        table.add_column("WALL",   no_wrap=True, justify="right", min_width=7)
        table.add_column("NOTE",   no_wrap=True)

        now = time.monotonic()
        for spec in self._specs:
            jid    = spec.job_id
            status = statuses.get(jid, JobStatus.PENDING)
            style  = _STATUS_STYLE.get(status, "")

            if jid in job_end:
                wall = f"{job_end[jid] - job_start.get(jid, job_end[jid]):.1f}s"
            elif jid in job_start:
                wall = f"{now - job_start[jid]:.1f}s"
            else:
                wall = "-"

            age  = _mtime_age_s(self._progress_paths[jid]) if status is JobStatus.RUNNING else None
            hung = age is not None and age > self._hung_threshold_s
            note = Text("HUNG", style="bold red") if hung else Text("")

            table.add_row(
                spec.test_name,
                spec.seed_hex,
                Text(status.value, style=style),
                wall,
                note,
            )

        return table


class CampaignTUI:
    """Live terminal UI for a running campaign batch.

    Auto-disables when stdout is not a TTY so Jenkins/headless invocations
    need no special flags. Pass enabled=False to force headless explicitly
    (e.g. --no-tui CLI flag).

    Usage::

        with CampaignTUI(specs, batch_run_id, enabled=not args.no_tui) as tui:
            while not done:
                statuses = backend.poll(job_ids)
                tui.update(statuses)
                if not tui.enabled:
                    # plain-text fallback
    """

    def __init__(
        self,
        specs:            list[JobSpec],
        batch_run_id:     str,
        *,
        enabled:          bool  = True,
        hung_threshold_s: float = _HUNG_THRESHOLD_S,
    ) -> None:
        # isatty() guards against CI pipes even when caller passes enabled=True.
        self._enabled: bool = enabled and sys.stdout.isatty()

        if not self._enabled:
            return

        self._renderer  = _Renderer(specs, batch_run_id, hung_threshold_s)
        self._console   = Console()
        # transient=True: the live table clears on exit; the detailed summary
        # printed by campaign_runner is the canonical final record.
        self._live      = Live(console=self._console, refresh_per_second=4, transient=True)
        self._job_start: dict[str, float]     = {}
        self._job_end:   dict[str, float]     = {}
        self._prev:      dict[str, JobStatus] = {}

    @property
    def enabled(self) -> bool:
        return self._enabled

    # ------------------------------------------------------------------
    # Context manager
    # ------------------------------------------------------------------

    def __enter__(self) -> CampaignTUI:
        if self._enabled:
            self._live.__enter__()
        return self

    def __exit__(self, *exc_info: object) -> None:
        if self._enabled:
            self._live.__exit__(*exc_info)

    # ------------------------------------------------------------------
    # Update — called once per poll cycle
    # ------------------------------------------------------------------

    def update(self, statuses: dict[str, JobStatus]) -> None:
        if not self._enabled:
            return

        now = time.monotonic()
        for jid, status in statuses.items():
            prev = self._prev.get(jid)
            if prev is not JobStatus.RUNNING and status is JobStatus.RUNNING:
                self._job_start[jid] = now
            if (
                prev is JobStatus.RUNNING
                and status.is_terminal
                and jid not in self._job_end
            ):
                self._job_end[jid] = now

        self._prev = dict(statuses)
        self._live.update(self._renderer.build(statuses, self._job_start, self._job_end))
