#!/usr/bin/env python3
"""PostToolUse hook: warns when an include/ header is missing its expected CF_*_H guard
or copyright block. Warnings exit with code 2 so they reach Claude.

Two escape hatches for headers that don't follow the derived-from-filename
convention: LEGACY_GUARDS maps a header to its real (pre-convention) guard,
and NO_GUARD lists headers deliberately written without an include guard
(e.g. repeat-inclusion headers)."""
import sys
import json
import os
import re

# Headers whose real guard predates the CF_<NAME>_H convention.
LEGACY_GUARDS = {
    "cute_time.h": "CF_TIMER_H",
    "cute_doubly_list.h": "CF_DOUBLY_LINKED_LIST_H",
}
# Headers deliberately without an include guard (repeat-inclusion headers).
NO_GUARD = {"cute_debug_printf.h"}

data = json.load(sys.stdin)
tool_input = data.get("tool_input", {})
file_path = tool_input.get("file_path", "")

norm = os.path.normpath(file_path)
parts = norm.split(os.sep)
problems = []

if "include" in parts and file_path.endswith(".h"):
    name = os.path.basename(file_path)
    base = name[:-2]

    if base.startswith("cute_"):
        rest = base[5:]
    elif base == "cute":
        rest = ""
    else:
        rest = base

    expected = LEGACY_GUARDS.get(name, f"CF_{rest.upper()}_H" if rest else "CF_H")

    try:
        with open(file_path) as f:
            content = f.read()
    except OSError:
        sys.exit(0)

    if name not in NO_GUARD and expected not in content:
        problems.append(f"{name} is missing expected include guard '{expected}'.")

    copyright_re = r"Copyright \(C\) 20\d\d Randy Gaul https://randygaul\.github\.io/"
    if not re.search(copyright_re, content):
        problems.append(
            f"{name} is missing the standard Cute Framework copyright block "
            "(see any header in include/ for the exact text)."
        )

if problems:
    for p in problems:
        print(f"WARNING: {p}", file=sys.stderr)
    sys.exit(2)
