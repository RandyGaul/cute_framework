#!/usr/bin/env python3
"""PostToolUse hook: warns when an include/ header is missing its expected CF_*_H guard
or copyright block. Warnings exit with code 2 so they reach Claude."""
import sys
import json
import os
import re

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

    expected = f"CF_{rest.upper()}_H" if rest else "CF_H"

    try:
        with open(file_path) as f:
            content = f.read()
    except OSError:
        sys.exit(0)

    if expected not in content:
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
