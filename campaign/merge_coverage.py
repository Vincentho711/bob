"""Coverage merger for campaign batch directories.

Reads coverage.json from each run in a batch, merges them per binary,
and writes a self-describing merged JSON file for downstream analysis.

Usage:
    # Single batch — writes coverage_merged_<binary>.json to batch_dir
    python -m campaign.merge_coverage --batch-dir <path>

    # Cross-batch (--output-dir required)
    python -m campaign.merge_coverage --batch-dir d1 --batch-dir d2 --output-dir <out>

    # Accumulate into a running cumulative directory
    python -m campaign.merge_coverage --batch-dir <path> --accumulate-from <dir>

    # Reset baseline on destructive schema change
    python -m campaign.merge_coverage --batch-dir <path> --accumulate-from <dir> --reset-baseline
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path


def _utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _utc_now_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def _load_json(path: Path) -> dict | None:
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        print(f"  [WARN] Failed to read {path}: {e}", file=sys.stderr)
        return None


# ---------------------------------------------------------------------------
# Schema evolution detection
# ---------------------------------------------------------------------------

def _extract_structure(cov: dict) -> dict:
    """
    Returns {cg_name: {"coverpoints": {cp_name: {bin_name: (type, ranges_key)}},
                        "crosses": {cx_name: [constituents]}}}
    for structural comparison across runs/cumulatives.
    """
    struct: dict = {}
    for cg in cov.get("covergroups", []):
        cg_name = cg["name"]
        cps: dict[str, dict] = {}
        for cp in cg.get("coverpoints", []):
            bins: dict[str, tuple] = {}
            for b in cp.get("bins", []):
                ranges_key = tuple(sorted((r["lo"], r["hi"]) for r in b.get("ranges", [])))
                bins[b["name"]] = (b["type"], ranges_key)
            cps[cp["name"]] = bins
        cxs: dict[str, list] = {}
        for cx in cg.get("cross_coverpoints", []):
            cxs[cx["name"]] = list(cx.get("constituents", []))
        struct[cg_name] = {"coverpoints": cps, "crosses": cxs}
    return struct


def _classify_change(old: dict, new: dict) -> tuple[str, str]:
    """Returns ("match"|"additive"|"destructive", diff_description)."""
    removed: list[str] = []
    added: list[str] = []

    for cg_name, old_cg in old.items():
        if cg_name not in new:
            removed.append(f"covergroup '{cg_name}'")
            continue
        new_cg = new[cg_name]
        old_cps = old_cg["coverpoints"]
        new_cps = new_cg["coverpoints"]
        for cp_name, old_bins in old_cps.items():
            if cp_name not in new_cps:
                removed.append(f"  coverpoint '{cg_name}/{cp_name}'")
                continue
            for bin_name, old_props in old_bins.items():
                if bin_name not in new_cps[cp_name]:
                    removed.append(f"  bin '{cg_name}/{cp_name}/{bin_name}'")
                elif new_cps[cp_name][bin_name] != old_props:
                    removed.append(f"  bin '{cg_name}/{cp_name}/{bin_name}' (type/ranges changed)")
            for bin_name in new_cps[cp_name]:
                if bin_name not in old_bins:
                    added.append(f"  bin '{cg_name}/{cp_name}/{bin_name}'")
        for cp_name in new_cps:
            if cp_name not in old_cps:
                added.append(f"  coverpoint '{cg_name}/{cp_name}' (new)")
        old_cxs, new_cxs = old_cg["crosses"], new_cg["crosses"]
        for cx_name, old_const in old_cxs.items():
            if cx_name not in new_cxs:
                removed.append(f"  cross '{cg_name}/{cx_name}'")
            elif new_cxs[cx_name] != old_const:
                removed.append(f"  cross '{cg_name}/{cx_name}' (constituents changed)")
        for cx_name in new_cxs:
            if cx_name not in old_cxs:
                added.append(f"  cross '{cg_name}/{cx_name}' (new)")
    for cg_name in new:
        if cg_name not in old:
            added.append(f"covergroup '{cg_name}' (new)")

    if not removed and not added:
        return "match", ""
    diff_lines = [f"  - {r}" for r in removed] + [f"  + {a}" for a in added]
    if not removed:
        return "additive", "\n".join(diff_lines)
    return "destructive", "\n".join(diff_lines)


# ---------------------------------------------------------------------------
# Per-test coverage helper
# ---------------------------------------------------------------------------

def _test_group_coverage(test_covs: list[dict]) -> float:
    """Compute merged coverage for a group of coverage.json dicts (same test).
    Counts a bin as hit if any run in the group hit it at least once.
    """
    if not test_covs:
        return 0.0
    bins_defined = 0
    hit_bins: set[tuple] = set()
    cx_expected: dict[tuple, int] = {}   # (cg, cx) -> expected_bins
    hit_combos: set[tuple] = set()       # (cg, cx, key)

    ref = test_covs[0]
    for cg in ref.get("covergroups", []):
        cg_name = cg["name"]
        for cp in cg.get("coverpoints", []):
            for b in cp.get("bins", []):
                if b.get("type") == "regular":
                    bins_defined += 1
        for cx in cg.get("cross_coverpoints", []):
            expected = cx.get("expected_bins", 0)
            cx_expected[(cg_name, cx["name"])] = expected
            bins_defined += expected

    for cov in test_covs:
        for cg in cov.get("covergroups", []):
            cg_name = cg["name"]
            for cp in cg.get("coverpoints", []):
                cp_name = cp["name"]
                for b in cp.get("bins", []):
                    if b.get("type") == "regular" and b.get("hits", 0) > 0:
                        hit_bins.add((cg_name, cp_name, b["name"]))
            for cx in cg.get("cross_coverpoints", []):
                cx_name = cx["name"]
                for key in cx.get("hits", {}):
                    hit_combos.add((cg_name, cx_name, key))

    hits_total = len(hit_bins) + len(hit_combos)
    return hits_total / bins_defined if bins_defined > 0 else 0.0


# ---------------------------------------------------------------------------
# Run collection
# ---------------------------------------------------------------------------

def _collect_runs(
    batch_dirs: list[Path],
) -> dict[str, list[tuple[str, Path, str | None]]]:
    """
    Returns {binary_stem: [(batch_id, run_dir, test_name), ...]}
    for binaries with coverage enabled whose run_dirs contain coverage.json.
    """
    result: dict[str, list[tuple[str, Path, str | None]]] = defaultdict(list)

    for batch_dir in batch_dirs:
        batch_dir = batch_dir.resolve()

        summary_path = batch_dir / "summary.json"
        if not summary_path.exists():
            print(f"[WARN] No summary.json in {batch_dir} — skipping", file=sys.stderr)
            continue
        summary = _load_json(summary_path)
        if summary is None:
            continue

        coverage_map: dict[str, bool] = summary.get("coverage", {})
        batch_id = summary.get("batch_id", batch_dir.name)

        if not any(coverage_map.values()):
            print(f"[INFO] {batch_dir.name}: no coverage enabled — skipping")
            continue

        manifest_path = batch_dir / "manifest.jsonl"
        if not manifest_path.exists():
            print(f"[WARN] No manifest.jsonl in {batch_dir} — skipping", file=sys.stderr)
            continue

        with open(manifest_path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    entry = json.loads(line)
                except json.JSONDecodeError:
                    continue
                binary_stem = Path(entry.get("binary", "")).stem
                if not coverage_map.get(binary_stem, False):
                    continue
                run_dir = Path(entry["run_dir"])
                if not (run_dir / "coverage.json").exists():
                    continue
                result[binary_stem].append((batch_id, run_dir, entry.get("test")))

    return result


# ---------------------------------------------------------------------------
# Core merge
# ---------------------------------------------------------------------------

def _do_merge(
    binary_stem: str,
    batch_ids: list[str],
    cov_runs: list[tuple[dict, str | None]],  # (parsed coverage.json, test_name)
    runs_total: int,
    accum: dict | None,
) -> dict:
    """Merge individual run coverage.json files (plus optional accumulated base) into one dict."""
    ref = cov_runs[0][0]
    fingerprint = ref.get("model_fingerprint", "")

    # Initialize accumulators — use reference structure for ordering
    cg_order: list[str] = []
    cp_order: dict[str, list[str]] = {}
    cx_order: dict[str, list[str]] = {}
    # {cg -> {cp -> {bin -> {hits, seeds, type, ranges}}}}
    cp_bins: dict[str, dict[str, dict[str, dict]]] = {}
    cp_samples: dict[str, dict[str, int]] = {}
    # {cg -> {cx -> {constituents, expected_bins, total_samples, hits: {key: int}, seeds: {key: int}}}}
    cx_agg: dict[str, dict[str, dict]] = {}

    def _init_cg(cg: dict) -> None:
        cg_name = cg["name"]
        if cg_name in cp_bins:
            return
        cg_order.append(cg_name)
        cp_bins[cg_name] = {}
        cp_samples[cg_name] = {}
        cx_agg[cg_name] = {}
        cp_order[cg_name] = [cp["name"] for cp in cg.get("coverpoints", [])]
        cx_order[cg_name] = [cx["name"] for cx in cg.get("cross_coverpoints", [])]
        for cp in cg.get("coverpoints", []):
            cp_name = cp["name"]
            cp_bins[cg_name][cp_name] = {}
            cp_samples[cg_name][cp_name] = 0
            for b in cp.get("bins", []):
                cp_bins[cg_name][cp_name][b["name"]] = {
                    "hits": 0, "seeds": 0,
                    "type": b["type"], "ranges": b.get("ranges", []),
                }
        for cx in cg.get("cross_coverpoints", []):
            cx_name = cx["name"]
            cx_agg[cg_name][cx_name] = {
                "constituents": cx.get("constituents", []),
                "expected_bins": cx.get("expected_bins", 0),
                "total_samples": 0,
                "hits": {},
                "seeds": {},
            }

    for cg in ref.get("covergroups", []):
        _init_cg(cg)

    # Add accumulated base (treated as a pre-merged synthetic "run")
    if accum is not None:
        for cg in accum.get("covergroups", []):
            cg_name = cg["name"]
            if cg_name not in cp_bins:
                continue
            for cp in cg.get("coverpoints", []):
                cp_name = cp["name"]
                if cp_name not in cp_bins[cg_name]:
                    continue
                cp_samples[cg_name][cp_name] += cp.get("total_samples", 0)
                for b in cp.get("bins", []):
                    bname = b["name"]
                    if bname not in cp_bins[cg_name][cp_name]:
                        continue
                    cp_bins[cg_name][cp_name][bname]["hits"]  += b.get("hits", 0)
                    cp_bins[cg_name][cp_name][bname]["seeds"] += b.get("seeds_contributing", 0)
            for cx in cg.get("cross_coverpoints", []):
                cx_name = cx["name"]
                if cx_name not in cx_agg[cg_name]:
                    continue
                cx_agg[cg_name][cx_name]["total_samples"] += cx.get("total_samples", 0)
                for key, cnt in cx.get("hits", {}).items():
                    cx_agg[cg_name][cx_name]["hits"][key] = (
                        cx_agg[cg_name][cx_name]["hits"].get(key, 0) + cnt
                    )

    # Merge individual runs
    for cov, _ in cov_runs:
        for cg in cov.get("covergroups", []):
            cg_name = cg["name"]
            if cg_name not in cp_bins:
                _init_cg(cg)

            for cp in cg.get("coverpoints", []):
                cp_name = cp["name"]
                if cp_name not in cp_bins[cg_name]:
                    cp_bins[cg_name][cp_name] = {}
                    cp_samples[cg_name][cp_name] = 0
                    if cp_name not in cp_order.get(cg_name, []):
                        cp_order.setdefault(cg_name, []).append(cp_name)
                cp_samples[cg_name][cp_name] += cp.get("total_samples", 0)
                for b in cp.get("bins", []):
                    bname = b["name"]
                    if bname not in cp_bins[cg_name][cp_name]:
                        cp_bins[cg_name][cp_name][bname] = {
                            "hits": 0, "seeds": 0,
                            "type": b["type"], "ranges": b.get("ranges", []),
                        }
                    h = b.get("hits", 0)
                    cp_bins[cg_name][cp_name][bname]["hits"] += h
                    if h > 0:
                        cp_bins[cg_name][cp_name][bname]["seeds"] += 1

            for cx in cg.get("cross_coverpoints", []):
                cx_name = cx["name"]
                if cx_name not in cx_agg[cg_name]:
                    cx_agg[cg_name][cx_name] = {
                        "constituents": cx.get("constituents", []),
                        "expected_bins": cx.get("expected_bins", 0),
                        "total_samples": 0, "hits": {}, "seeds": {},
                    }
                    if cx_name not in cx_order.get(cg_name, []):
                        cx_order.setdefault(cg_name, []).append(cx_name)
                cx_agg[cg_name][cx_name]["total_samples"] += cx.get("total_samples", 0)
                for key, cnt in cx.get("hits", {}).items():
                    cx_agg[cg_name][cx_name]["hits"][key] = (
                        cx_agg[cg_name][cx_name]["hits"].get(key, 0) + cnt
                    )
                    if cnt > 0:
                        cx_agg[cg_name][cx_name]["seeds"][key] = (
                            cx_agg[cg_name][cx_name]["seeds"].get(key, 0) + 1
                        )

    # Build output structure
    out_covergroups = []
    total_defined = 0
    total_hit = 0
    unhit_bins: list[dict] = []

    for cg_name in cg_order:
        cg_defined = 0
        cg_hit = 0
        out_coverpoints = []

        for cp_name in cp_order.get(cg_name, []):
            if cp_name not in cp_bins[cg_name]:
                continue
            bins_data = cp_bins[cg_name][cp_name]
            cp_num = 0
            cp_hit = 0
            out_bins = []

            # Regular bins first (metrics + unhit tracking)
            for bname, bd in bins_data.items():
                if bd["type"] != "regular":
                    continue
                cp_num += 1
                if bd["hits"] > 0:
                    cp_hit += 1
                else:
                    unhit_bins.append({"covergroup": cg_name, "coverpoint": cp_name, "bin": bname})
                out_bins.append({
                    "name": bname, "type": bd["type"],
                    "ranges": bd["ranges"], "hits": bd["hits"],
                    "seeds_contributing": bd["seeds"],
                })
            # Non-regular bins (illegal/ignore) — include in output, excluded from metrics
            for bname, bd in bins_data.items():
                if bd["type"] == "regular":
                    continue
                out_bins.append({
                    "name": bname, "type": bd["type"],
                    "ranges": bd["ranges"], "hits": bd["hits"],
                    "seeds_contributing": bd["seeds"],
                })

            cg_defined += cp_num
            cg_hit += cp_hit
            cp_cov = cp_hit / cp_num if cp_num > 0 else 0.0
            out_coverpoints.append({
                "name": cp_name,
                "num_bins": cp_num,
                "bins_hit": cp_hit,
                "total_samples": cp_samples[cg_name].get(cp_name, 0),
                "coverage": cp_cov,
                "bins": out_bins,
            })

        out_crosses = []
        for cx_name in cx_order.get(cg_name, []):
            if cx_name not in cx_agg[cg_name]:
                continue
            cxd = cx_agg[cg_name][cx_name]
            expected = cxd["expected_bins"]
            seen = len(cxd["hits"])
            cg_defined += expected
            cg_hit += seen
            cx_cov = seen / expected if expected > 0 else 0.0
            out_crosses.append({
                "name": cx_name,
                "constituents": cxd["constituents"],
                "expected_bins": expected,
                "combinations_seen": seen,
                "total_samples": cxd["total_samples"],
                "coverage": cx_cov,
                "hits": dict(sorted(cxd["hits"].items())),
            })

        cg_cov = cg_hit / cg_defined if cg_defined > 0 else 0.0
        total_defined += cg_defined
        total_hit += cg_hit
        out_covergroups.append({
            "name": cg_name,
            "coverage": cg_cov,
            "coverpoints": out_coverpoints,
            "cross_coverpoints": out_crosses,
        })

    total_coverage = total_hit / total_defined if total_defined > 0 else 0.0

    # Per-test coverage breakdown (current batch only — excludes accumulated base)
    test_groups: dict[str, list[dict]] = defaultdict(list)
    for cov, test_name in cov_runs:
        key = test_name if test_name is not None else "default"
        test_groups[key].append(cov)
    coverage_by_test = {
        key: {"runs": len(covs), "total_coverage": _test_group_coverage(covs)}
        for key, covs in test_groups.items()
    }

    # Final batch_ids and run counts (merge with cumulative if accumulating)
    merged_batch_ids = list(batch_ids)
    merged_runs_total = runs_total
    merged_runs_merged = len(cov_runs)
    if accum is not None:
        accum_bids = [b for b in accum.get("batch_ids", []) if b not in set(batch_ids)]
        merged_batch_ids = accum_bids + merged_batch_ids
        merged_runs_total += accum.get("runs_total", 0)
        merged_runs_merged += accum.get("runs_merged", 0)

    return {
        "schema_version": 1,
        "model_fingerprint": fingerprint,
        "binary": binary_stem,
        "batch_ids": merged_batch_ids,
        "finished_at": _utc_now_iso(),
        "runs_total": merged_runs_total,
        "runs_merged": merged_runs_merged,
        "total_coverage": total_coverage,
        "coverage_by_test": coverage_by_test,
        "unhit_bins": unhit_bins,
        "covergroups": out_covergroups,
    }


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def merge_coverage(
    batch_dirs: list[Path],
    accumulate_from_dir: Path | None = None,
    output_dir: Path | None = None,
    reset_baseline: bool = False,
    force_union: bool = False,
    strict: bool = False,
) -> int:
    runs_by_binary = _collect_runs(batch_dirs)

    if not runs_by_binary:
        print("No coverage data found in specified batch directories.")
        return 0

    any_output = False

    for binary_stem, runs in runs_by_binary.items():
        print(f"\nMerging '{binary_stem}' ({len(runs)} runs with coverage.json) ...")

        # Deduplicate and preserve batch_id order
        batch_ids: list[str] = []
        seen_bids: set[str] = set()
        for bid, _, _ in runs:
            if bid not in seen_bids:
                batch_ids.append(bid)
                seen_bids.add(bid)

        # Parse coverage.json files; validate schema_version
        cov_runs: list[tuple[dict, str | None]] = []
        fingerprints: set[str] = set()
        for _, run_dir, test_name in runs:
            cov = _load_json(run_dir / "coverage.json")
            if cov is None:
                continue
            if cov.get("schema_version") != 1:
                print(f"  [WARN] {run_dir}/coverage.json: unknown schema_version — skipping",
                      file=sys.stderr)
                continue
            fingerprints.add(cov.get("model_fingerprint", ""))
            cov_runs.append((cov, test_name))

        if not cov_runs:
            print(f"  [WARN] {binary_stem}: no parseable coverage.json files — skipping")
            continue

        if len(fingerprints) > 1:
            print(f"  [WARN] {binary_stem}: {len(fingerprints)} distinct fingerprints in batch",
                  file=sys.stderr)
        batch_fp = next(iter(fingerprints))

        # Accumulation: load existing cumulative and apply schema evolution policy
        accum: dict | None = None
        accum_path: Path | None = None

        if accumulate_from_dir is not None:
            accum_path = accumulate_from_dir / f"{binary_stem}.json"
            if accum_path.exists():
                accum = _load_json(accum_path)
                if accum is not None:
                    accum_fp = accum.get("model_fingerprint", "")
                    if accum_fp != batch_fp:
                        change_type, diff_text = _classify_change(
                            _extract_structure(accum),
                            _extract_structure(cov_runs[0][0]),
                        )
                        if change_type == "additive":
                            if strict:
                                print(f"ERROR: {binary_stem}: additive schema change — refusing (--strict)",
                                      file=sys.stderr)
                                print(diff_text, file=sys.stderr)
                                return 1
                            print(f"  [INFO] {binary_stem}: additive schema change — auto union-merging")
                            print(diff_text)
                        elif change_type == "destructive":
                            if reset_baseline:
                                stamp = _utc_now_stamp()
                                archive = accum_path.parent / f"{binary_stem}_until_{stamp}.json"
                                accum_path.rename(archive)
                                print(f"  [INFO] {binary_stem}: archived old cumulative → {archive}")
                                accum = None
                                accum_path = accumulate_from_dir / f"{binary_stem}.json"
                            elif force_union:
                                print(f"  [WARN] {binary_stem}: destructive schema change — force-union merging",
                                      file=sys.stderr)
                                print(diff_text, file=sys.stderr)
                            else:
                                print(f"ERROR: {binary_stem}: destructive schema change:",
                                      file=sys.stderr)
                                print(diff_text, file=sys.stderr)
                                print(
                                    "  Re-run with --reset-baseline to archive and start fresh, "
                                    "or --force-union to merge anyway",
                                    file=sys.stderr,
                                )
                                return 1
            # File absent: create fresh silently

        # Perform merge
        merged = _do_merge(binary_stem, batch_ids, cov_runs, len(runs), accum)

        # Determine output path
        if output_dir is not None:
            out_path = output_dir / f"coverage_merged_{binary_stem}.json"
        elif accum_path is not None:
            out_path = accum_path
        elif len(batch_dirs) == 1:
            out_path = batch_dirs[0].resolve() / f"coverage_merged_{binary_stem}.json"
        else:
            print("ERROR: --output-dir required for multi-batch merge without --accumulate-from",
                  file=sys.stderr)
            return 1

        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, "w") as f:
            json.dump(merged, f, indent=2)

        pct = f"{merged['total_coverage'] * 100:.1f}%"
        unhit = len(merged["unhit_bins"])
        merged_n = merged["runs_merged"]
        total_n = merged["runs_total"]
        print(f"  Merged {merged_n}/{total_n} runs | coverage={pct} | unhit_bins={unhit} → {out_path}")
        any_output = True

    if not any_output:
        print("No coverage data in this batch.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Merge per-run coverage.json files from campaign batch directories"
    )
    parser.add_argument(
        "--batch-dir", dest="batch_dirs", action="append",
        type=Path, required=True, metavar="PATH",
        help="Batch directory (repeatable for cross-batch merge)",
    )
    parser.add_argument(
        "--accumulate-from", dest="accumulate_from", type=Path, metavar="DIR",
        help="Directory containing per-binary cumulative .json files (created on first run)",
    )
    parser.add_argument(
        "--output-dir", dest="output_dir", type=Path, metavar="PATH",
        help="Output directory for merged files (required for multi-batch without --accumulate-from)",
    )
    parser.add_argument(
        "--reset-baseline", action="store_true",
        help="On destructive schema change: archive old cumulative and start fresh",
    )
    parser.add_argument(
        "--force-union", action="store_true",
        help="On destructive schema change: merge anyway with warnings",
    )
    parser.add_argument(
        "--strict", action="store_true",
        help="Refuse even pure-additive schema changes",
    )
    args = parser.parse_args(argv)

    return merge_coverage(
        batch_dirs=args.batch_dirs,
        accumulate_from_dir=args.accumulate_from,
        output_dir=args.output_dir,
        reset_baseline=args.reset_baseline,
        force_union=args.force_union,
        strict=args.strict,
    )


if __name__ == "__main__":
    sys.exit(main())
