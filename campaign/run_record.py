"""Build a per-run result record from a JobSpec and its on-disk artefacts.

Shared between campaign_runner.py (written at end of a live batch) and
summarise.py (re-derived from existing artefacts post-hoc).
"""
from __future__ import annotations

import json
import re
from pathlib import Path

from campaign.job_spec import JobSpec, JobStatus


def build_run_record(spec: JobSpec, status: JobStatus) -> dict:
    """Return a dict describing one run, suitable for embedding in summary.json.

    All filesystem paths are absolute (resolved) so they are immediately
    usable from Jenkins, a browser-based NFS viewer, or any terminal
    regardless of CWD.
    """
    binary_name = spec.binary.stem
    record: dict = {
        "job_id":      spec.job_id,
        "binary":      str(spec.binary.resolve()) if spec.binary != Path("") else None,
        "test":        spec.test_name,
        "seed":        f"0x{spec.seed_hex}",
        "status":      status.value,
        "run_dir":     None,
        "run_log":     None,
        "reproduce":   None,
        "duration_s":  None,
        "sim_time_ps": None,
        "seq_count":   None,
        "error_msg":   None,
    }

    if not status.is_terminal:
        return record

    run_dir = spec.output_dir.resolve()
    if not run_dir.exists():
        # Crashed before --output-dir was created; surface crash log if present
        crash_log = spec.output_dir.parent / f"{binary_name}_crash.log"
        record["run_log"] = str(crash_log) if crash_log.exists() else None
        return record

    record["run_dir"] = str(run_dir)

    log_path   = run_dir / f"{binary_name}.log"
    repro_path = run_dir / "reproduce.sh"
    record["run_log"]   = str(log_path)   if log_path.exists()   else None
    record["reproduce"] = str(repro_path) if repro_path.exists() else None

    run_end = _read_run_end(str(run_dir / "progress.jsonl"))
    if run_end:
        ts_wall_us = run_end.get("ts_wall_us")
        record["duration_s"]  = round(ts_wall_us / 1e6, 3) if ts_wall_us is not None else None
        record["sim_time_ps"] = run_end.get("ts_sim_ps")
        record["seq_count"]   = run_end.get("seq_count")
        record["error_msg"]   = run_end.get("msg") or None

    return record


def write_wall_timeout_event(
    progress_path: Path, wall_timeout_s: int | None, elapsed_us: int
) -> None:
    """Append a synthetic run_end event after a wall-clock kill.

    Written by LocalBackend so that build_run_record and summarise.py can
    surface the cause of failure via error_msg without any special-casing.
    Best-effort — OSError is silently swallowed so a write failure does not
    mask the original timeout.
    """
    msg = (
        f"killed: wall-clock timeout after {wall_timeout_s}s"
        if wall_timeout_s is not None
        else "killed: wall-clock timeout"
    )
    event = {
        "schema_version": 2,
        "t":          "run_end",
        "ts_wall_us": elapsed_us,
        "ts_sim_ps":  None,
        "status":     "wall_timeout",
        "msg":        msg,
        "seq_count":  None,
    }
    try:
        with open(progress_path, "a") as f:
            f.write(json.dumps(event) + "\n")
    except OSError:
        pass


def _write_exit_event(progress_path: Path, returncode: int) -> None:
    """Write a synthetic run_end for binaries that don't emit progress events.

    Mirrors write_wall_timeout_event so that summarise.py can derive status
    from progress.jsonl without special-casing no-progress binaries.
    Best-effort — OSError is silently swallowed.
    """
    event = {
        "schema_version": 2,
        "t":          "run_end",
        "ts_wall_us": None,
        "ts_sim_ps":  None,
        "status":     "completed" if returncode == 0 else "failed",
        "msg":        None if returncode == 0 else f"exited with code {returncode}",
        "seq_count":  None,
    }
    try:
        with open(progress_path, "w") as f:
            f.write(json.dumps(event) + "\n")
    except OSError:
        pass


def _finalise(spec: JobSpec, err_path: Path, *, returncode: int | None = None) -> JobStatus:
    """Handle post-process cleanup and write reproduce.sh.

    Called by LocalBackend from a worker thread after proc.wait() returns.
    err_path is the captured stderr temp file. Returns the terminal JobStatus
    derived from progress.jsonl, falling back to returncode for binaries that
    do not emit progress events.
    """
    binary_name = spec.binary.stem
    run_dir = spec.output_dir

    if run_dir.exists():
        # The binary redirects both stdout and stderr to its own log file via
        # freopen() in CoreArgumentGroup::post_parse_resolve(), so err_path is
        # empty for normal runs.  Just clean it up.
        try:
            err_path.unlink(missing_ok=True)
        except OSError:
            pass
        _write_reproduce_script(run_dir, spec)
        progress_path = run_dir / "progress.jsonl"
        run_end = _read_run_end(str(progress_path))
        if run_end is None:
            # No progress events — fall back to process exit code if available,
            # and write a synthetic run_end so post-hoc summarise works correctly.
            if returncode is not None:
                _write_exit_event(progress_path, returncode)
                return JobStatus.PASSED if returncode == 0 else JobStatus.FAILED
            return JobStatus.ERROR
        s = run_end.get("status", "")
        if s == "completed":
            return JobStatus.PASSED
        elif s == "max_time":
            return JobStatus.TIMEOUT
        elif s == "wall_timeout":
            return JobStatus.TIMEOUT
        return JobStatus.FAILED
    else:
        # Binary crashed before creating its output dir — rename stderr as crash log
        crash_log = spec.output_dir.parent / f"{binary_name}_crash.log"
        try:
            if err_path.stat().st_size > 0:
                err_path.rename(crash_log)
            else:
                err_path.unlink(missing_ok=True)
        except OSError:
            pass
        return JobStatus.ERROR


def _write_reproduce_script(run_dir: Path, spec: JobSpec) -> None:
    skip_prefixes = ("--output-dir=",)
    repro_args = [str(spec.binary.resolve())]
    for arg in spec.args[1:]:
        if not any(arg.startswith(p) for p in skip_prefixes):
            repro_args.append(arg)
    repro_args.append("--waves")
    repro_args.append("--output-dir=${output_dir}")

    git_header: list[str] = []
    git_guard:  list[str] = []
    if spec.git_sha:
        git_header = [
            f"# Git SHA: {spec.git_sha}",
            "# To reproduce exactly: git checkout <sha>, rebuild the binary, then run this script.",
            "",
        ]
        git_guard = [
            'if command -v git >/dev/null 2>&1; then',
            '  _current_sha=$(git rev-parse HEAD 2>/dev/null || true)',
            f'  if [ -n "$_current_sha" ] && [ "$_current_sha" != "{spec.git_sha}" ]; then',
            f'    echo "WARNING: current HEAD ($_current_sha) differs from recorded SHA ({spec.git_sha})" >&2',
            '    echo "         Rebuild from that commit for exact reproduction." >&2',
            '  fi',
            'fi',
            "",
        ]

    lines = [
        "#!/usr/bin/env bash",
        *git_header,
        f"# Reproduce: test={spec.test_name}  seed=0x{spec.seed_hex}",
        f"# Original batch job: {spec.job_id}",
        "set -euo pipefail",
        *git_guard,
        'script_dir="$(cd "$(dirname "$0")" && pwd)"',
        'output_dir="${script_dir}/repro"',
        "",
        "while [[ $# -gt 0 ]]; do",
        "    case \"$1\" in",
        "        --output-dir) output_dir=\"$2\"; shift 2 ;;",
        "        --output-dir=*) output_dir=\"${1#*=}\"; shift ;;",
        "        *) echo \"Unknown argument: $1\" >&2; exit 1 ;;",
        "    esac",
        "done",
        "",
        "mkdir -p \"${output_dir}\"",
        "",
        " \\\n    ".join(repro_args),
        "",
    ]
    script_path = run_dir / "reproduce.sh"
    script_path.write_text("\n".join(lines))
    script_path.chmod(0o755)


def _read_run_end(jsonl_path: str) -> dict | None:
    run_end = None
    try:
        with open(jsonl_path) as f:
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
        pass
    return run_end
