#!/usr/bin/env python3
"""Stress / robustness probe: run xsdb over a corpus of schemas across every
output target and categorize the outcomes. This is a DISCOVERY tool — failures
are the point (it surfaces unsupported XSD constructs), so it always exits 0 and
prints a markdown report.

Usage: run_stress.py [xsdb-binary] [corpus-dir]
  defaults: build/xsdb  test/stress/corpus
"""
import glob
import os
import subprocess
import sys
from collections import Counter, defaultdict

# The 10 emit targets + the "test" parser-model dump (the purest "does it parse"
# check — if `test` fails the schema didn't parse at all).
TARGETS = [
    "test", "c-xml-expat", "c-xml-expat-dom", "c-json-jsonc",
    "cpp-xml-expat", "cpp-json-jsonc", "python-sax", "python-json",
    "java-json.org", "java-xml-stax", "ts-xml",
]

# xsdb exit codes (src/main.cpp)
EXIT = {0: "ok", 1: "general", 2: "usage", 3: "no-file",
        4: "parse(XMLException)", 5: "template(Lua)", 6: "resource"}

# stderr markers -> the XSD feature/exception responsible
MARKERS = ["InvallidChildXMLElement", "CyclicTypeDefinition", "UndefiniedXSDType",
           "UnsupportedXSDType", "MissingElement", "MissingChildXMLElement",
           "NamespaceMismatch", "SubstitutionGroup", "URINotValid",
           "ProtocolNotSupported"]


def marker_of(stderr):
    for m in MARKERS:
        if m in stderr:
            return m
    return ""


def main(argv):
    xsdb = argv[1] if len(argv) > 1 else "build/xsdb"
    corpus = argv[2] if len(argv) > 2 else "test/stress/corpus"
    schemas = sorted(glob.glob(os.path.join(corpus, "*.xsd")))
    if not schemas:
        print(f"no schemas in {corpus} (run fetch_schemas.sh first)")
        return 0

    exit_counts = Counter()
    marker_counts = Counter()
    per_schema = {}            # schema -> list[(target, rc, marker)]
    fully_ok = []

    for schema in schemas:
        rows = []
        ok_all = True
        for t in TARGETS:
            try:
                p = subprocess.run([xsdb, t, schema], capture_output=True,
                                   text=True, timeout=120)
                rc, marker = p.returncode, marker_of(p.stderr)
            except subprocess.TimeoutExpired:
                rc, marker = 124, "TIMEOUT"
            except Exception as e:                       # noqa: BLE001
                rc, marker = 999, type(e).__name__
            rows.append((t, rc, marker))
            exit_counts[rc] += 1
            if marker:
                marker_counts[marker] += 1
            if rc != 0:
                ok_all = False
        per_schema[schema] = rows
        if ok_all:
            fully_ok.append(schema)

    # ---- report (markdown) ----
    n = len(schemas)
    print(f"# Stress report — {n} schemas x {len(TARGETS)} targets\n")
    print(f"- binary: `{xsdb}`")
    print(f"- fully-clean schemas (all targets exit 0): "
          f"**{len(fully_ok)}/{n}**\n")

    print("## Outcomes by exit code\n")
    for rc in sorted(exit_counts):
        print(f"- `{rc}` {EXIT.get(rc, '?'):<22} {exit_counts[rc]}")
    print()

    print("## Failures by XSD construct / exception\n")
    if marker_counts:
        for m, c in marker_counts.most_common():
            print(f"- **{m}**: {c}")
    else:
        print("- (none)")
    print()

    print("## Per-schema (target: exit/marker for non-ok)\n")
    for schema in schemas:
        base = os.path.basename(schema)
        bad = [(t, rc, mk) for (t, rc, mk) in per_schema[schema] if rc != 0]
        if not bad:
            print(f"- `{base}` — all targets ok")
            continue
        # if `test` (parse) failed, the schema doesn't parse at all
        parse = next((rc for (t, rc, _) in per_schema[schema] if t == "test"),
                     None)
        tag = "DOES-NOT-PARSE" if parse not in (0, None) else "parses; gen gaps"
        detail = ", ".join(f"{t}:{rc}{('/'+mk) if mk else ''}"
                           for (t, rc, mk) in bad)
        print(f"- `{base}` — {tag} — {detail}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
