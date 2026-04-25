"""Pydantic models for campaign plan YAML schema.

Owns field definitions, defaults, and type validation. Runtime environment
checks (binary exists, output dir writable) are in check_runtime().
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

import yaml
from pydantic import BaseModel, ConfigDict, Field, ValidationError, field_validator, model_validator


class Resources(BaseModel):
    """Slurm resource specification — ignored by LocalBackend."""
    model_config = ConfigDict(extra="ignore")

    cpus:           int = Field(1,    gt=0)
    mem_mb:         int = Field(1024, gt=0)
    time_limit_min: int = Field(10,   gt=0)


class RunEntry(BaseModel):
    """One entry in a binary's 'runs' list: optional test name plus seed source(s)."""
    model_config = ConfigDict(extra="forbid")

    test:           str | None  = None
    count:          int         = Field(0, ge=0)
    seeds:          list[int]   = Field(default_factory=list)
    max_time_ps:    int | None  = None  # None → inherits Plan.max_time_ps
    wall_timeout_s: int | None  = Field(None, gt=0)  # None → inherits Plan.wall_timeout_s
    extra_args:     list[str]   = Field(default_factory=list)

    @field_validator("seeds", mode="before")
    @classmethod
    def parse_seeds(cls, v: list) -> list[int]:
        """Accept decimal ints or hex strings (e.g. "0x1a2b")."""
        result = []
        for s in v:
            if isinstance(s, int):
                result.append(s)
            elif isinstance(s, str):
                try:
                    result.append(int(s, 0))
                except ValueError:
                    raise ValueError(f"seed {s!r} is not a valid integer")
            else:
                raise ValueError(f"seed {s!r} must be an integer or hex string")
        return result

    @model_validator(mode="after")
    def requires_seeds_or_count(self) -> RunEntry:
        if not self.seeds and self.count == 0:
            raise ValueError("must specify 'seeds', 'count', or both")
        return self


class BinaryEntry(BaseModel):
    """One binary with its own heartbeat setting and list of run entries."""
    model_config = ConfigDict(extra="forbid")

    binary:       Path
    heartbeat_ms: int | None     = None  # None → omit --progress.heartbeat-ms=
    runs:         list[RunEntry] = Field(min_length=1)


class Plan(BaseModel):
    """Top-level campaign plan schema."""
    model_config = ConfigDict(extra="forbid")

    schema_version: int       = 1
    batch_id:       str       = "batch"
    output_dir:     Path      = Path("runs")
    max_time_ps:    int       = Field(100_000_000_000, gt=0)
    wall_timeout_s: int | None = Field(None, gt=0)  # None = no wall-clock limit
    binaries:       list[BinaryEntry] = Field(min_length=1)
    resources:      Resources = Field(default_factory=Resources)


def load_plan(plan_path: Path) -> Plan:
    """Load and validate a campaign YAML file. Exits with a clear message on error."""
    with open(plan_path) as f:
        raw = yaml.safe_load(f)
    try:
        return Plan.model_validate(raw)
    except ValidationError as exc:
        lines = [f"Plan validation failed ({exc.error_count()} error(s)):"]
        for err in exc.errors():
            loc = " → ".join(str(x) for x in err["loc"]) if err["loc"] else "plan"
            lines.append(f"  [{loc}] {err['msg']}")
        print("\n".join(lines), file=sys.stderr)
        sys.exit(1)


def check_warnings(plan: Plan) -> list[str]:
    """Non-fatal advisories about the plan configuration."""
    warnings: list[str] = []
    if plan.wall_timeout_s is None and all(
        e.wall_timeout_s is None
        for b in plan.binaries
        for e in b.runs
    ):
        warnings.append(
            "wall_timeout_s not set — hung jobs will block the batch indefinitely"
        )
    return warnings


def check_runtime(plan: Plan) -> list[str]:
    """Check environment prerequisites. Returns a list of error strings (empty = ok).

    Separate from schema validation because these depend on the filesystem,
    not the YAML content. Binary paths are CWD-relative, matching existing behaviour.
    """
    errors: list[str] = []

    for b in plan.binaries:
        if not b.binary.is_file():
            errors.append(
                f"binary not found: {b.binary}\n"
                f"  Resolved: {b.binary.resolve()}"
            )
        elif not os.access(b.binary, os.X_OK):
            errors.append(f"binary not executable: {b.binary.resolve()}")

    output_parent = plan.output_dir if plan.output_dir.exists() else plan.output_dir.parent
    if output_parent.exists() and not os.access(output_parent, os.W_OK):
        errors.append(f"output directory not writable: {plan.output_dir.resolve()}")

    return errors
