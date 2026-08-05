/*
    Cute Framework
    Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

    This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_app_internal.h>
#include <stdlib.h>

static bool s_alive = false;
static int s_options = 0;

static bool s_fresh_mode()
{
	const char* fresh = getenv("CF_TEST_FRESH_APP");
	return fresh && *fresh == '1';
}

bool test_make_app(int w, int h, int extra_options)
{
	int options = CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | extra_options;
	const char* gles = getenv("CF_TEST_GLES");
	if (gles && *gles == '1') options |= CF_APP_OPTIONS_GFX_OPENGL_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT;

	if (s_fresh_mode()) {
		// A failed REQUIRE returns out of a test before its trailing test_destroy_app, and
		// the leaked app would make cf_make_app fail for every test after it. Destroying a
		// nonexistent app is a safe no-op, so always sweep before creating.
		cf_destroy_app();
		return !cf_is_error(cf_make_app(NULL, 0, 0, 0, w, h, options, NULL));
	}

	if (s_alive && options == s_options) {
		cf_app_force_pixel_scale(0); // Sweep a forced density leaked by a failed HiDPI test.
		cf_app_set_size(w, h); // Recreates the app canvas + default 2d projection immediately.
		// Well-known process globals a test legitimately mutates and rarely thinks to restore
		// -- with one app per test their reset came free from cf_destroy_app. Everything else
		// (push/pop stacks, canvases, shaders) is the test's own balance to keep; run with
		// CF_TEST_FRESH_APP=1 when hunting a leak.
		cf_clear_color(0, 0, 0, 0);
		cf_clear_depth_stencil(1.0f, 0);
		cf_app_update(NULL); // Each test starts at a clean frame boundary.
		return true;
	}
	// Destroys the options-mismatched shared app, or an app leaked by a failed test that
	// manages its own app directly (cf_destroy_app with no app alive is a safe no-op).
	cf_destroy_app();
	s_alive = false;
	if (cf_is_error(cf_make_app(NULL, 0, 0, 0, w, h, options, NULL))) return false;
	s_alive = true;
	s_options = options;
	return true;
}

void test_destroy_app()
{
	if (s_fresh_mode()) {
		cf_destroy_app();
		return;
	}
	// Shared: the app stays up for the next test; main() shuts it down at exit.
}

void test_shutdown_shared_app()
{
	if (s_alive) {
		cf_destroy_app();
		s_alive = false;
	}
}
