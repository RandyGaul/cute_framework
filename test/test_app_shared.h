/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Creating a GPU device dominates suite runtime -- a second or more per cf_make_app, and the
// suite used to do it dozens of times. GPU test cases share one app instead: test_make_app
// reuses the live app whenever the effective options match (resizing to the requested
// dimensions and ticking one frame so every test starts at a clean frame boundary), and
// test_destroy_app leaves it alive for the next test. The CF_TEST_GLES environment variable
// is folded into the options here, so tests no longer roll their own check.
//
// Set CF_TEST_FRESH_APP=1 to restore the old one-app-per-test behavior when hunting a
// cross-test state leak.
//
// main() calls test_shutdown_shared_app() after the last suite so the CRT leak checker sees
// a clean exit.

#ifndef TEST_APP_SHARED_H
#define TEST_APP_SHARED_H

// Returns false when no app can be made (headless CI: no display/GPU) -- return true from
// the test to skip, exactly like the old inline cf_make_app pattern.
bool test_make_app(int w, int h, int extra_options = 0);

// The pair for test_make_app: a no-op while sharing, a real destroy under CF_TEST_FRESH_APP.
void test_destroy_app();

void test_shutdown_shared_app();

#endif // TEST_APP_SHARED_H
