#!/usr/bin/env python3
"""PostToolUse hook: warns when an include/ header is missing its expected CF_*_H guard."""
import sys
import json
import os

data = json.load(sys.stdin)
tool_input = data.get("tool_input", {})
file_path = tool_input.get("file_path", "")

if "/include/" in file_path and file_path.endswith(".h"):
    name = os.path.basename(file_path)  # e.g. "cute_graphics.h"
    base = name[:-2]  # strip ".h"

    if base.startswith("cute_"):
        rest = base[5:]  # "cute_graphics" -> "graphics"
    elif base == "cute":
        rest = ""
    else:
        rest = base

    expected = f"CF_{rest.upper()}_H" if rest else "CF_H"

    try:
        with open(file_path) as f:
            content = f.read()
        if expected not in content:
            print(
                f"WARNING: {name} is missing expected include guard '{expected}'.",
                file=sys.stderr,
            )
    except OSError:
        pass
