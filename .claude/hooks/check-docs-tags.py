#!/usr/bin/env python3
"""PostToolUse hook: warns when an include/ header contains an @tag the docs
parser does not recognize.

tools/docs_parser.c tokenizes ENTIRE headers (not just doc blocks) and panics
on any unknown @word — even inside a plain // comment. That kills the docs
build in CI. This hook catches it at edit time. Warn-only (exit 2 so the
message reaches Claude).
"""
import json
import os
import re
import sys

ALLOWED = {
    "function", "struct", "enum", "category", "brief", "param", "return",
    "remarks", "example", "related", "member", "entry", "end",
}

data = json.load(sys.stdin)
file_path = data.get("tool_input", {}).get("file_path", "")

norm = os.path.normpath(file_path)
parts = norm.split(os.sep)
if "include" not in parts or not file_path.endswith(".h"):
    sys.exit(0)

try:
    with open(file_path) as f:
        lines = f.readlines()
except OSError:
    sys.exit(0)

bad = []
for lineno, line in enumerate(lines, 1):
    for m in re.finditer(r"(?:^|\s)@([A-Za-z_]\w*)", line):
        if m.group(1) not in ALLOWED:
            bad.append((lineno, "@" + m.group(1)))

if bad:
    name = os.path.basename(file_path)
    print(
        f"WARNING: {name} contains @tags the docs parser rejects — "
        "these will PANIC the docs build in CI:",
        file=sys.stderr,
    )
    for lineno, tag in bad:
        print(f"  line {lineno}: {tag}", file=sys.stderr)
    print(
        "Allowed tags: " + " ".join(sorted("@" + t for t in ALLOWED)) + ". "
        "Mark deprecations in prose inside @brief/@remarks instead of @deprecated.",
        file=sys.stderr,
    )
    sys.exit(2)
