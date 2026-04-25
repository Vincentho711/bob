"""Standalone result aggregator for a completed campaign batch.

Reads manifest.jsonl, locates each run's progress.jsonl inside the batch
directory, and produces the same rich summary.json as the live campaign runner.
All paths in the output are absolute so they are usable from Jenkins, a
browser-based NFS viewer, or any terminal regardless of CWD.

Usage:
    python -m campaign.summarise --batch-dir runs/dev_regression_20260418T003251Z
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

from campaign.job_spec import JobSpec, JobStatus
from campaign.run_record import build_run_record


def _read_manifest(batch_dir: Path) -> list[dict]:
    path = batch_dir / "manifest.jsonl"
    if not path.exists():
        print(f"No manifest found at {path}", file=sys.stderr)
        return []
    entries = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    entries.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return entries


def _manifest_to_spec(entry: dict) -> JobSpec:
    """Reconstruct a minimal JobSpec from a manifest entry (enough for build_run_record)."""
    return JobSpec(
        job_id=entry["job_id"],
        binary=Path(entry.get("binary", "")),
        args=[],
        output_dir=Path(entry["run_dir"]),
        test_name=entry["test"],
        seed=entry["seed"],
    )


def _print_table(records: list[dict]) -> None:
    header = f"\n{'BINARY':<20} {'TEST':<16} {'SEED':<18} {'STATUS':<10} {'WALL(s)':>8}  {'SIM(ps)':>12}  {'SEQ':>5}  MSG"
    print(header)
    print("-" * max(len(header), 80))
    for r in records:
        binary = Path(r.get("binary", "")).stem if r.get("binary") else ""
        test   = r.get("test") or ""
        seed   = r.get("seed", "-")
        status = r.get("status", "unknown")
        wall   = f"{r['duration_s']:.2f}" if r.get("duration_s") is not None else "-"
        sim_ps = str(r.get("sim_time_ps") or "-")
        seq    = str(r.get("seq_count") or "-")
        msg    = (r.get("error_msg") or "")[:60]
        print(f"{binary:<20} {test:<16} {seed:<18} {status:<10} {wall:>8}  {sim_ps:>12}  {seq:>5}  {msg}")


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


def summarise(batch_dir: Path) -> int:
    batch_dir = batch_dir.resolve()
    entries   = _read_manifest(batch_dir)
    if not entries:
        return 1

    # Re-read existing summary.json for batch metadata if available
    existing: dict = {}
    existing_path = batch_dir / "summary.json"
    if existing_path.exists():
        try:
            with open(existing_path) as f:
                existing = json.load(f)
        except (OSError, json.JSONDecodeError):
            pass

    specs   = [_manifest_to_spec(e) for e in entries]
    records = [build_run_record(spec, _infer_status(spec)) for spec in specs]

    counts  = Counter(r["status"] for r in records)
    total   = len(records)
    passed  = counts.get("passed", 0)
    failures = [r for r in records if r["status"] != "passed"]

    now = datetime.now(timezone.utc).isoformat()
    summary = {
        "batch_id":    existing.get("batch_id", batch_dir.name),
        "plan":        existing.get("plan", None),
        "binaries":    existing.get("binaries", [existing["binary"]] if existing.get("binary") else []),
        "started_at":  existing.get("started_at", None),
        "finished_at": existing.get("finished_at", now),
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

    summary_path = batch_dir / "summary.json"
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)

    _print_table(records)
    _print_failures(failures)
    print(f"\n{passed}/{total} passed  |  summary: {summary_path.resolve()}")
    return 0 if counts.get("failed", 0) == 0 and counts.get("error", 0) == 0 and counts.get("timeout", 0) == 0 else 1


def _infer_status(spec: JobSpec) -> JobStatus:
    """Derive status from progress.jsonl without a live backend."""
    import json

    progress_path = spec.output_dir / "progress.jsonl"
    if not progress_path.exists():
        return JobStatus.ERROR

    run_end = None
    try:
        with open(progress_path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    ev = json.loads(line)
                    if ev.get("t") == "run_end":
                        run_end = ev
                except json.JSONDecodeError:
                    pass
    except OSError:
        return JobStatus.ERROR

    if run_end is None:
        return JobStatus.ERROR
    s = run_end.get("status", "")
    if s == "completed":
        return JobStatus.PASSED
    elif s == "max_time":
        return JobStatus.TIMEOUT
    elif s == "wall_timeout":
        return JobStatus.TIMEOUT
    return JobStatus.FAILED


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Summarise a completed campaign batch")
    parser.add_argument("--batch-dir", required=True, type=Path,
                        help="Path to the batch directory (contains manifest.jsonl)")
    args = parser.parse_args(argv)
    return summarise(args.batch_dir)


if __name__ == "__main__":
    sys.exit(main())
