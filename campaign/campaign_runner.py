"""Campaign runner: load a plan YAML, generate job specs, dispatch, poll, summarise.

Usage:
    python -m campaign.campaign_runner --plan campaign.yaml [options]

Options:
    --backend  local|slurm     Execution backend (default: local)
    --workers  N               Max concurrent jobs for local backend (default: cpu_count)
    --poll-interval  SECS      Seconds between status polls (default: 5)
    --dry-run                  Print job specs without executing
    --validate                 Validate plan and check binary; do not run
"""
from __future__ import annotations

import argparse
import json
import secrets
import sys
import time
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

from campaign.backends.local import LocalBackend
from campaign.backends.slurm import SlurmBackend
from campaign.job_spec import JobSpec, JobStatus
from campaign.plan import Plan, check_runtime, check_warnings, load_plan
from campaign.run_record import build_run_record
from campaign.tui import CampaignTUI


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _utc_now_str() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


# ---------------------------------------------------------------------------
# Spec expansion
# ---------------------------------------------------------------------------

def _expand_specs(plan: Plan, batch_run_id: str, batch_dir: Path) -> list[JobSpec]:
    """Expand plan entries into one JobSpec per (test, seed) pair."""
    binary             = plan.binary
    max_time_ps_global = plan.max_time_ps
    heartbeat_ms       = plan.heartbeat_ms
    resources          = plan.resources.model_dump()

    specs: list[JobSpec] = []
    for entry in plan.runs:
        test     = entry.test
        max_time = entry.max_time_ps if entry.max_time_ps is not None else max_time_ps_global

        wall_timeout_s = (
            entry.wall_timeout_s
            if entry.wall_timeout_s is not None
            else plan.wall_timeout_s
        )
        seeds = list(entry.seeds)
        if entry.count > 0:
            seeds += [secrets.randbelow(2**64) for _ in range(entry.count)]

        for seed in seeds:
            seed_hex = f"{seed:016x}"
            job_id   = f"{batch_run_id}_{test}_{seed_hex}"
            run_dir  = batch_dir / job_id
            args = [
                str(binary),
                f"--tb.test={test}",
                f"--seed={seed}",
                f"--max-time={max_time}",
                f"--output-dir={run_dir}",
                f"--progress.heartbeat-ms={heartbeat_ms}",
            ]
            specs.append(JobSpec(
                job_id=job_id,
                binary=binary,
                args=args,
                output_dir=run_dir,
                test_name=test,
                seed=seed,
                resources=resources,
                wall_timeout_s=wall_timeout_s,
            ))

    return specs


# ---------------------------------------------------------------------------
# Manifest
# ---------------------------------------------------------------------------

def _write_manifest(batch_dir: Path, specs: list[JobSpec]) -> None:
    batch_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = batch_dir / "manifest.jsonl"
    with open(manifest_path, "w") as f:
        for spec in specs:
            f.write(spec.to_manifest_line())
    print(f"Manifest written: {manifest_path}  ({len(specs)} jobs)")


# ---------------------------------------------------------------------------
# Status display
# ---------------------------------------------------------------------------

_STATUS_ORDER = [JobStatus.RUNNING, JobStatus.PASSED, JobStatus.FAILED,
                 JobStatus.TIMEOUT, JobStatus.ERROR, JobStatus.PENDING]


def _status_line(statuses: dict[str, JobStatus], total: int, elapsed: float) -> str:
    counts = Counter(statuses.values())
    parts = "  ".join(f"{s.value}:{counts.get(s, 0)}" for s in _STATUS_ORDER)
    mins, secs = divmod(int(elapsed), 60)
    return f"[{sum(s.is_terminal for s in statuses.values())}/{total}]  {parts}  | {mins:02d}:{secs:02d} elapsed"


# ---------------------------------------------------------------------------
# Summary / table
# ---------------------------------------------------------------------------

def _print_table(specs: list[JobSpec], records: list[dict]) -> None:
    rec_by_id = {r["job_id"]: r for r in records}
    header = f"\n{'TEST':<20} {'SEED':<18} {'STATUS':<10} {'WALL(s)':>8}  {'SIM(ps)':>12}  {'SEQ':>5}  MSG"
    print(header)
    print("-" * max(len(header), 80))
    for spec in specs:
        r      = rec_by_id.get(spec.job_id, {})
        status = r.get("status", "unknown")
        wall   = f"{r['duration_s']:.2f}" if r.get("duration_s") is not None else "-"
        sim_ps = str(r.get("sim_time_ps") or "-")
        seq    = str(r.get("seq_count") or "-")
        msg    = (r.get("error_msg") or "")[:60]
        print(f"{spec.test_name:<20} {spec.seed_hex:<18} {status:<10} {wall:>8}  {sim_ps:>12}  {seq:>5}  {msg}")


def _print_failures(failures: list[dict]) -> None:
    if not failures:
        return
    print(f"\nFAILURES ({len(failures)}):")
    for r in failures:
        print(f"  {r['test']}  {r['seed']}  [{r['status']}]")
        if r.get("error_msg"):
            print(f"    Error: {r['error_msg']}")
        if r.get("run_log"):
            print(f"    Log:   {r['run_log']}")
        if r.get("reproduce"):
            print(f"    Repro: {r['reproduce']}")


def _write_summary(batch_dir: Path, specs: list[JobSpec],
                   statuses: dict[str, JobStatus],
                   batch_run_id: str, plan_path: Path, binary: Path,
                   started_at: str, finished_at: str) -> dict:
    records = [build_run_record(spec, statuses.get(spec.job_id, JobStatus.UNKNOWN))
               for spec in specs]

    counts: Counter = Counter(r["status"] for r in records)
    total  = len(records)
    passed = counts.get("passed", 0)

    failures = [r for r in records if r["status"] != "passed"]

    summary = {
        "batch_id":    batch_run_id,
        "plan":        str(plan_path.resolve()),
        "binary":      str(binary.resolve()),
        "started_at":  started_at,
        "finished_at": finished_at,
        "counts": {
            "total":   total,
            "passed":  passed,
            "failed":  counts.get("failed",  0),
            "error":   counts.get("error",   0),
            "timeout": counts.get("timeout", 0),
            "pending": counts.get("pending", 0),
        },
        "pass_rate": f"{passed}/{total} ({100 * passed / total:.1f}%)" if total else "0/0",
        "failures":  failures,
        "runs":      records,
    }

    summary_path = (batch_dir / "summary.json").resolve()
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)

    _print_table(specs, records)
    _print_failures(failures)
    print(f"\n{passed}/{total} passed  |  summary: {summary_path}")
    return summary


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Campaign runner for batch simulation regressions")
    parser.add_argument("--plan",          required=True, type=Path, help="Path to campaign.yaml")
    parser.add_argument("--backend",       default="local", choices=["local", "slurm"])
    parser.add_argument("--workers",       type=int, default=None, help="Max concurrent jobs (local backend)")
    parser.add_argument("--poll-interval", type=float, default=5.0, metavar="SECS")
    parser.add_argument("--dry-run",       action="store_true", help="Print job specs without executing")
    parser.add_argument("--validate",      action="store_true", help="Validate plan and check binary; do not run")
    parser.add_argument("--no-tui",        action="store_true", help="Disable live terminal UI (auto-disabled when not a TTY)")
    args = parser.parse_args(argv)

    plan = load_plan(args.plan)

    if args.validate:
        errors = check_runtime(plan)
        if errors:
            for e in errors:
                print(f"  FAIL  {e}", file=sys.stderr)
            return 1
        for w in check_warnings(plan):
            print(f"  WARN  {w}", file=sys.stderr)
        n_jobs = sum(len(e.seeds) + e.count for e in plan.runs)
        print(f"Plan OK — {n_jobs} jobs would be generated from {len(plan.runs)} run entries")
        return 0

    errors = check_runtime(plan)
    if errors:
        print("Pre-flight checks failed:", file=sys.stderr)
        for e in errors:
            print(f"  {e}", file=sys.stderr)
        return 1
    for w in check_warnings(plan):
        print(f"  WARN  {w}", file=sys.stderr)

    batch_id         = plan.batch_id
    output_dir       = plan.output_dir
    heartbeat_ms     = plan.heartbeat_ms
    batch_run_id     = f"{batch_id}_{_utc_now_str()}"
    batch_dir        = output_dir / batch_run_id
    hung_threshold_s = max(10.0, heartbeat_ms / 1000 * 10)

    specs = _expand_specs(plan, batch_run_id, batch_dir)

    if args.dry_run:
        print(f"Dry run — batch: {batch_run_id}  ({len(specs)} jobs):")
        for spec in specs:
            print(f"  {spec.job_id}")
            print(f"    {' '.join(spec.args)}")
        return 0

    started_at = datetime.now(timezone.utc).isoformat()
    print(f"Batch: {batch_run_id}")
    _write_manifest(batch_dir, specs)

    if args.backend == "slurm":
        backend = SlurmBackend()
    else:
        backend = LocalBackend(max_workers=args.workers)

    all_job_ids = [s.job_id for s in specs]

    print(f"Submitting {len(specs)} jobs (backend={args.backend}) ...")
    for spec in specs:
        backend.submit(spec)

    wall_start = time.monotonic()
    statuses: dict[str, JobStatus] = {jid: JobStatus.PENDING for jid in all_job_ids}

    tui: CampaignTUI | None = None
    try:
        with CampaignTUI(specs, batch_run_id, enabled=not args.no_tui,
                         hung_threshold_s=hung_threshold_s) as tui:
            while True:
                statuses = backend.poll(all_job_ids)
                tui.update(statuses)
                if not tui.enabled:
                    line = _status_line(statuses, len(specs), time.monotonic() - wall_start)
                    print(f"\r{line}", end="", flush=True)

                if all(s.is_terminal for s in statuses.values()):
                    break

                # LocalBackend: block until the next job finishes (event-driven).
                # SlurmBackend and others: fall back to fixed-interval polling.
                if hasattr(backend, "wait_for_change"):
                    backend.wait_for_change(timeout=args.poll_interval)
                else:
                    time.sleep(args.poll_interval)
    except KeyboardInterrupt:
        if tui is not None and not tui.enabled:
            print()  # newline after \r
        print("Interrupted — cancelling ...")
        backend.cancel(all_job_ids)
        print("Cancelled.")
        return 130

    if tui is not None and not tui.enabled:
        print()  # newline after \r progress line
    finished_at = datetime.now(timezone.utc).isoformat()

    summary = _write_summary(batch_dir, specs, statuses,
                             batch_run_id, args.plan, plan.binary,
                             started_at, finished_at)
    c = summary["counts"]
    return 0 if c["failed"] == 0 and c["error"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
