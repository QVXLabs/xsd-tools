#!/usr/bin/env python3
"""Run the xsdb-test gtest suite in parallel via gtest sharding.

The round-trip tests dominate wall-clock: each shells out to mvn/cc/python
serially, so a full run is minutes. They are independent processes (each test
uses its own mkdtemp dir, ~/.m2 reads are concurrency-safe), so splitting the
suite across N shards run concurrently gives a near-linear speedup.

Usage:
  run_parallel.py [--jobs N] [--binary PATH] [-- <gtest args>]

Any args after `--` (or any unrecognized --gtest_* arg) pass through to each
shard, e.g. --gtest_filter='Java*Roundtrip.*'. Exit status is nonzero if any
test fails or any shard crashes.
"""
import argparse
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

_FAILED = re.compile(r"^\[\s*FAILED\s*\] (\S+)", re.M)
_RUN = re.compile(r"^\[ RUN", re.M)


def _default_binary():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    return os.path.join(root, "build", "test", "xsdb-test")


def _run_shard(binary, index, total, passthrough):
    env = dict(os.environ)
    env["GTEST_TOTAL_SHARDS"] = str(total)
    env["GTEST_SHARD_INDEX"] = str(index)
    env["GTEST_COLOR"] = "no"
    proc = subprocess.run(
        [binary, *passthrough],
        env=env,
        capture_output=True,
        text=True,
    )
    out = proc.stdout + proc.stderr
    return {
        "index": index,
        "returncode": proc.returncode,
        "ran": len(_RUN.findall(out)),
        "failed": _FAILED.findall(out),
        "output": out,
    }


def main(argv):
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("--jobs", "-j", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--binary", default=_default_binary())
    ap.add_argument("rest", nargs=argparse.REMAINDER)
    args = ap.parse_args(argv)

    # Drop a leading "--" separator if present.
    passthrough = args.rest[1:] if args.rest[:1] == ["--"] else args.rest

    binary = args.binary
    if not os.path.exists(binary) and os.path.exists(binary + ".exe"):
        binary += ".exe"
    if not os.path.exists(binary):
        print(f"test binary not found: {args.binary}", file=sys.stderr)
        return 2
    args.binary = binary

    jobs = max(1, args.jobs)
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(
            pool.map(
                lambda i: _run_shard(args.binary, i, jobs, passthrough),
                range(jobs),
            )
        )

    total_ran = sum(r["ran"] for r in results)
    failures = [t for r in results for t in r["failed"]]
    crashed = [r for r in results if r["returncode"] != 0 and not r["failed"]]

    # Echo full output of any shard that had failures or crashed.
    for r in results:
        if r["failed"] or r["returncode"] != 0:
            sys.stdout.write(r["output"])

    print(f"\n=== parallel summary ({jobs} shards) ===")
    print(f"tests run : {total_ran}")
    print(f"failed    : {len(failures)}")
    for name in failures:
        print(f"  FAILED {name}")
    for r in crashed:
        print(f"  SHARD {r['index']} crashed (exit {r['returncode']})")

    return 1 if (failures or crashed) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
