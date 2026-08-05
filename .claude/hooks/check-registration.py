#!/usr/bin/env python3
"""PostToolUse hook: warns when a new file is not registered where the build
expects it. Catches "wrote the file, forgot to register it, build still green
because nothing references it yet."

  src/*.cpp            -> CF_SRCS in CMakeLists.txt
  include/cute_*.h     -> #include in include/cute.h (with whitelist)
  test/test_*.cpp      -> CF_TEST_SRCS in test/CMakeLists.txt
                          AND TEST_SUITE/RUN_TRACED in test/main.cpp
  samples/*.c|*.cpp    -> add_sample(...) in samples/CMakeLists.txt

Warn-only (exit 2 so the message reaches Claude). Runs from the repo root.
"""
import json
import os
import sys

# Headers deliberately not in the cute.h umbrella.
UMBRELLA_WHITELIST = {
    "cute_result.h", "cute_shader_bytecode.h", "cute_user_config.h",
    "cute_priority_queue.h", "cute_debug_printf.h", "cute_c_runtime.h",
    "cute_defines.h",
}


def read(path):
    try:
        with open(path) as f:
            return f.read()
    except OSError:
        return ""


data = json.load(sys.stdin)
file_path = data.get("tool_input", {}).get("file_path", "")
if not file_path:
    sys.exit(0)

rel = os.path.relpath(os.path.abspath(file_path), os.getcwd())
rel = rel.replace(os.sep, "/")
name = os.path.basename(rel)
problems = []

if rel.startswith("src/") and rel.endswith(".cpp") and rel.count("/") == 1:
    if name not in read("CMakeLists.txt"):
        problems.append(f"{name} is not listed in CF_SRCS in CMakeLists.txt.")

elif (rel.startswith("include/") and rel.endswith(".h")
      and rel.count("/") == 1 and name.startswith("cute_")
      and not name.endswith("_shd.h") and name not in UMBRELLA_WHITELIST):
    if name not in read("include/cute.h"):
        problems.append(f"{name} is not included by the umbrella header include/cute.h.")

elif (rel.startswith("test/") and rel.endswith(".cpp")
      and rel.count("/") == 1 and name.startswith("test_")):
    stem = name[:-len(".cpp")]
    if name not in read("test/CMakeLists.txt"):
        problems.append(f"{name} is not listed in CF_TEST_SRCS in test/CMakeLists.txt.")
    main_cpp = read("test/main.cpp")
    if stem not in main_cpp:
        problems.append(
            f"suite '{stem}' is not registered in test/main.cpp "
            f"(needs TEST_SUITE({stem}); and RUN_TRACED({stem});)."
        )

elif (rel.startswith("samples/") and rel.count("/") == 1
      and (rel.endswith(".c") or rel.endswith(".cpp"))):
    if name not in read("samples/CMakeLists.txt"):
        problems.append(
            f"{name} has no add_sample(...) entry in samples/CMakeLists.txt."
        )

if problems:
    for p in problems:
        print(f"WARNING: {p}", file=sys.stderr)
    sys.exit(2)
