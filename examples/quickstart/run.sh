#!/bin/sh
# Run the five-endpoint relay and show the message at every stage.
#
# Each endpoint reads the message on stdin and writes it (plus its own <hop>) to
# stdout; we capture each stage's output, print it, and pipe it to the next.
# ep5 prints the final message and asserts the relay succeeded (PASS on stderr,
# non-zero exit on failure).
#
# Usage: run.sh <gen-dir>
#   <gen-dir> holds the xsdb-generated, compiled artifacts:
#     message.py            (python-sax binding; added to PYTHONPATH)
#     ep2, ep5              (compiled C++ endpoints)
#     javac/                (compiled Java classes; interop.Ep3Relay)
# Override the toolchain with PYTHON3=/path and JAVA=/path if needed.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)   # source dir (ep1/ep4 .py)
GEN=${1:-${QS_GEN:-.}}                               # generated/compiled dir
PYTHON3=${PYTHON3:-python3}
JAVA=${JAVA:-java}
export PYTHONPATH="$GEN${PYTHONPATH:+:$PYTHONPATH}"

stage() { printf '\n──────── %s ────────\n' "$1"; }

stage "ep1 (python) — produce the message"
msg=$("$PYTHON3" "$HERE/ep1_producer.py")
printf '%s\n' "$msg"

stage "ep2 (c++) — relay"
msg=$(printf '%s' "$msg" | "$GEN/ep2")
printf '%s\n' "$msg"

stage "ep3 (java) — relay"
msg=$(printf '%s' "$msg" | "$JAVA" -cp "$GEN/javac" interop.Ep3Relay)
printf '%s\n' "$msg"

stage "ep4 (python) — relay"
msg=$(printf '%s' "$msg" | "$PYTHON3" "$HERE/ep4_relay.py")
printf '%s\n' "$msg"

stage "ep5 (c++) — sink + verify"
printf '%s' "$msg" | "$GEN/ep5"   # prints the final message; PASS/exit on stderr
