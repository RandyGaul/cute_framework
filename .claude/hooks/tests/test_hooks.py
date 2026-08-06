#!/usr/bin/env python3
"""Tests for the .claude/hooks scripts. Run from the repo root:

    python3 .claude/hooks/tests/test_hooks.py -v

Each hook reads Claude Code hook JSON on stdin and communicates through its
exit code: 0 = silent pass, 2 = block (PreToolUse) or warn-to-Claude
(PostToolUse), with the message on stderr.
"""
import json
import pathlib
import subprocess
import sys
import unittest

HOOKS_DIR = pathlib.Path(__file__).resolve().parents[1]
REPO_ROOT = HOOKS_DIR.parents[1]
FIXTURES = pathlib.Path(__file__).resolve().parent / "fixtures"


def run_hook(script_name, file_path):
    payload = json.dumps({"tool_input": {"file_path": str(file_path)}})
    return subprocess.run(
        [sys.executable, str(HOOKS_DIR / script_name)],
        input=payload,
        capture_output=True,
        text=True,
        cwd=REPO_ROOT,
    )


class TestBlockGeneratedFiles(unittest.TestCase):
    SCRIPT = "block-generated-files.py"

    def test_blocks_shd_header(self):
        r = run_hook(self.SCRIPT, "include/blit_shd.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("generated", r.stderr)

    def test_blocks_generated_version_header(self):
        r = run_hook(self.SCRIPT, "include/cute_version.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("cute_version.h.in", r.stderr)

    def test_blocks_generated_version_source(self):
        r = run_hook(self.SCRIPT, "src/cute_version.cpp")
        self.assertEqual(r.returncode, 2)
        self.assertIn("cute_version.cpp.in", r.stderr)

    def test_allows_version_templates(self):
        for p in ("include/cute_version.h.in", "src/cute_version.cpp.in"):
            r = run_hook(self.SCRIPT, p)
            self.assertEqual(r.returncode, 0, msg=p)

    def test_allows_normal_source(self):
        r = run_hook(self.SCRIPT, "src/cute_draw.cpp")
        self.assertEqual(r.returncode, 0)

    def test_allows_shader_bytecode_header(self):
        # cute_shader_bytecode.h is hand-written, not generated.
        r = run_hook(self.SCRIPT, "include/cute_shader_bytecode.h")
        self.assertEqual(r.returncode, 0)


class TestCheckIncludeGuard(unittest.TestCase):
    SCRIPT = "check-include-guard.py"

    def test_silent_on_clean_header(self):
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_goodfixture.h")
        self.assertEqual(r.returncode, 0)
        self.assertEqual(r.stderr, "")

    def test_warns_on_wrong_guard(self):
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_wrongguard.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("CF_WRONGGUARD_H", r.stderr)

    def test_warns_on_missing_copyright(self):
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_nocopyright.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("copyright", r.stderr.lower())

    def test_ignores_non_include_paths(self):
        r = run_hook(self.SCRIPT, "src/cute_draw.cpp")
        self.assertEqual(r.returncode, 0)

    def test_all_real_headers_are_clean(self):
        # Every real public header must pass silently, or the hook nags on
        # every edit. Catches legacy guards and deliberate no-guard headers.
        import glob
        headers = glob.glob(str(REPO_ROOT / "include" / "*.h"))
        self.assertGreater(len(headers), 40)
        for h in headers:
            r = run_hook(self.SCRIPT, h)
            self.assertEqual(r.returncode, 0, msg=f"{h}: {r.stderr}")


class TestCheckDocsTags(unittest.TestCase):
    SCRIPT = "check-docs-tags.py"

    def test_silent_on_allowed_tags(self):
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_goodfixture.h")
        self.assertEqual(r.returncode, 0)
        self.assertEqual(r.stderr, "")

    def test_warns_on_unknown_tags_with_line_numbers(self):
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_badtag.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("@deprecated", r.stderr)
        self.assertIn("@todo", r.stderr)
        self.assertIn("docs build", r.stderr)

    def test_ignores_non_include_files(self):
        r = run_hook(self.SCRIPT, "src/cute_draw.cpp")
        self.assertEqual(r.returncode, 0)

    def test_ignores_at_symbols_mid_token(self):
        # The real docs parser only reacts to whitespace-delimited @tokens,
        # so @ symbols in URLs or other mid-token positions should not trigger.
        r = run_hook(self.SCRIPT, FIXTURES / "include" / "cute_midtoken.h")
        self.assertEqual(r.returncode, 0)
        self.assertEqual(r.stderr, "")

    def test_all_real_headers_are_clean(self):
        # Regression guard: every current public header must pass, or the
        # hook would nag on every edit. (docs CI is green, so they must.)
        import glob
        for h in glob.glob(str(REPO_ROOT / "include" / "*.h")):
            if h.endswith("_shd.h"):
                continue  # generated shader headers, not doc-parsed prose
            r = run_hook(self.SCRIPT, h)
            self.assertEqual(r.returncode, 0, msg=f"{h}: {r.stderr}")


class TestCheckRegistration(unittest.TestCase):
    SCRIPT = "check-registration.py"

    def test_registered_source_is_silent(self):
        r = run_hook(self.SCRIPT, "src/cute_draw.cpp")
        self.assertEqual(r.returncode, 0)

    def test_unregistered_source_warns(self):
        r = run_hook(self.SCRIPT, "src/cute_notreal.cpp")
        self.assertEqual(r.returncode, 2)
        self.assertIn("CF_SRCS", r.stderr)

    def test_registered_header_is_silent(self):
        r = run_hook(self.SCRIPT, "include/cute_draw.h")
        self.assertEqual(r.returncode, 0)

    def test_unregistered_header_warns(self):
        r = run_hook(self.SCRIPT, "include/cute_notreal.h")
        self.assertEqual(r.returncode, 2)
        self.assertIn("cute.h", r.stderr)

    def test_whitelisted_header_is_silent(self):
        r = run_hook(self.SCRIPT, "include/cute_defines.h")
        self.assertEqual(r.returncode, 0)

    def test_registered_test_is_silent(self):
        r = run_hook(self.SCRIPT, "test/test_math.cpp")
        self.assertEqual(r.returncode, 0)

    def test_unregistered_test_warns_both_registries(self):
        r = run_hook(self.SCRIPT, "test/test_notreal.cpp")
        self.assertEqual(r.returncode, 2)
        self.assertIn("CF_TEST_SRCS", r.stderr)
        self.assertIn("main.cpp", r.stderr)

    def test_registered_sample_is_silent(self):
        r = run_hook(self.SCRIPT, "samples/easy_sprite.c")
        self.assertEqual(r.returncode, 0)

    def test_unregistered_sample_warns(self):
        r = run_hook(self.SCRIPT, "samples/notreal.cpp")
        self.assertEqual(r.returncode, 2)
        self.assertIn("add_sample", r.stderr)

    def test_non_registerable_paths_are_silent(self):
        for p in ("docs/foo.md", "test/test_harness.h", "include/cute_version.h.in",
                  ".claude/hooks/tests/fixtures/include/cute_goodfixture.h"):
            r = run_hook(self.SCRIPT, p)
            self.assertEqual(r.returncode, 0, msg=p)

    def test_path_prefixed_lookalike_does_not_register(self):
        # libraries/imgui/imgui.cpp is in CMakeLists.txt, but src/imgui.cpp is NOT registered.
        r = run_hook(self.SCRIPT, "src/imgui.cpp")
        self.assertEqual(r.returncode, 2)
        self.assertIn("CF_SRCS", r.stderr)


if __name__ == "__main__":
    unittest.main()
