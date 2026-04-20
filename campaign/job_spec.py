from __future__ import annotations

import json
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path


class JobStatus(Enum):
    PENDING = "pending"
    RUNNING = "running"
    PASSED  = "passed"
    FAILED  = "failed"
    ERROR   = "error"    # crashed / no run_end
    TIMEOUT = "timeout"
    UNKNOWN = "unknown"

    @property
    def is_terminal(self) -> bool:
        return self in (JobStatus.PASSED, JobStatus.FAILED, JobStatus.ERROR, JobStatus.TIMEOUT)

    @property
    def is_success(self) -> bool:
        return self is JobStatus.PASSED


@dataclass
class JobSpec:
    job_id:     str           # "{batch_id}_{test}_{seed:016x}"
    binary:     Path
    args:       list[str]     # complete argv (binary + all flags)
    output_dir: Path          # full run directory path passed as --output-dir
    test_name:  str
    seed:       int
    resources:  dict = field(default_factory=dict)

    @property
    def seed_hex(self) -> str:
        return f"{self.seed:016x}"

    def to_manifest_line(self) -> str:
        return json.dumps({
            "job_id":   self.job_id,
            "test":     self.test_name,
            "seed":     self.seed,
            "seed_hex": self.seed_hex,
            "run_dir":  str(self.output_dir.resolve()),
        }) + "\n"
