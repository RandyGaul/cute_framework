/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

#include <internal/cute_app_internal.h>

TEST_CASE(test_app_destroy_safety)
{
	// Destroying when no app was ever created must be a safe no-op.
	cf_destroy_app();
	// Create and destroy an app, then destroy again -- the second call must be a no-op.
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	cf_destroy_app();
	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_power_state_mapping)
{
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_ERROR) == CF_POWER_STATE_ERROR);
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_UNKNOWN) == CF_POWER_STATE_UNKNOWN);
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_ON_BATTERY) == CF_POWER_STATE_ON_BATTERY);
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_NO_BATTERY) == CF_POWER_STATE_NO_BATTERY);
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_CHARGING) == CF_POWER_STATE_CHARGING);
	REQUIRE(cf_power_state_from_sdl(SDL_POWERSTATE_CHARGED) == CF_POWER_STATE_CHARGED);
	return true;
}

TEST_CASE(test_app_no_gfx_state_defaults)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	int x = -1, y = -1;
	cf_app_get_position(&x, &y);
	REQUIRE(x == 0);
	REQUIRE(y == 0);
	REQUIRE(cf_app_get_canvas_width() == 0);
	REQUIRE(cf_app_get_canvas_height() == 0);
	cf_destroy_app();
	return true;
}

TEST_CASE(test_display_count_matches_list)
{
	int count = cf_display_count();
	REQUIRE(count >= 1);
	CF_DisplayID* list = cf_get_display_list();
	REQUIRE(list != NULL);
	int n = 0;
	while (list[n]) ++n;
	REQUIRE(n == count);
	cf_free_display_list(list);
	return true;
}

TEST_CASE(test_display_invalid_id_is_safe)
{
	CF_DisplayID bogus = (CF_DisplayID)0xFFFFFFFFu;
	REQUIRE(cf_display_refresh_rate(bogus) == 0);
	REQUIRE(cf_display_x(bogus) == 0);
	REQUIRE(cf_display_y(bogus) == 0);
	REQUIRE(cf_display_width(bogus) == 0);
	REQUIRE(cf_display_height(bogus) == 0);
	CF_Rect r = cf_display_bounds(bogus);
	REQUIRE(r.x == 0);
	REQUIRE(r.y == 0);
	REQUIRE(r.w == 0);
	REQUIRE(r.h == 0);
	return true;
}

// A failed REQUIRE returns out of the test case early, so destroying the app must happen
// via RAII rather than a trailing call -- otherwise the leaked app breaks cf_make_app in
// whichever test case runs next. Deliberately NOT named AppDestroyGuard: that name is taken
// by test_canvas_clear.cpp's shared-fixture guard (whose destructor is a no-op while apps
// are shared), and a same-named struct with a different inline destructor is an ODR
// violation -- the linker silently picks one destructor for both, destroying the shared
// app behind the fixture's back.
//
// Gfx-app test cases must run after the display query cases: cf_destroy_app calls
// SDL_Quit, after which cf_display_count reports 0.
struct OwnedAppGuard
{
	~OwnedAppGuard() { cf_destroy_app(); }
};

TEST_CASE(test_app_set_canvas_size_is_persistent)
{
	REQUIRE(!cf_is_error(cf_make_app(NULL, 0, 0, 0, 200, 100, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	OwnedAppGuard guard;

	// Startup creates the default canvas at window_points * natural density -- the one and
	// only automatic sizing.
	float scale = cf_app_get_pixel_scale();
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(200 * scale));
	REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(100 * scale));

	// An explicit resize takes effect immediately...
	cf_app_set_canvas_size(320, 180);
	REQUIRE(cf_app_get_canvas_width() == 320);
	REQUIRE(cf_app_get_canvas_height() == 180);

	// ...and persists: a window resize is bookkeeping only, nothing resizes the canvas
	// behind the user's back.
	cf_app_set_size(256, 128);
	REQUIRE(cf_app_get_canvas_width() == 320);
	REQUIRE(cf_app_get_canvas_height() == 180);

	return true;
}

TEST_CASE(test_app_msaa_change_preserves_canvas_size)
{
	REQUIRE(!cf_is_error(cf_make_app(NULL, 0, 0, 0, 200, 100, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	OwnedAppGuard guard;

	cf_app_set_canvas_size(320, 180);
	REQUIRE(cf_app_get_canvas_width() == 320);
	REQUIRE(cf_app_get_canvas_height() == 180);

	if (cf_app_set_msaa(2)) { // MSAA support varies by backend/driver.
		// The canvas is rebuilt for the new sample count at its CURRENT size -- an MSAA
		// change must not stomp a user-chosen canvas size.
		REQUIRE(cf_app_get_canvas_width() == 320);
		REQUIRE(cf_app_get_canvas_height() == 180);
	}

	return true;
}

TEST_CASE(test_app_present_mode_vsync_always_supported)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// SDL guarantees VSYNC + SDR composition is always supported on every backend.
	REQUIRE(cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC));
	REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_VSYNC);

	destroy_app();

	return true;
}

TEST_CASE(test_app_present_mode_off_round_trip)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	REQUIRE(cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC));
	REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_VSYNC);

	bool ok = cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
	if (ok) {
		REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_IMMEDIATE);
	} else {
		// IMMEDIATE isn't guaranteed by SDL on every backend -- a rejection must leave the prior mode intact.
		REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_VSYNC);
	}

	destroy_app();

	return true;
}

// Regression test: cf_app_set_vsync_mailbox(true) used to silently report success even when the backend
// (e.g. Metal) rejected SDL_GPU_PRESENTMODE_MAILBOX, leaving the app lying about its vsync state. A
// rejected request must never be reflected by cf_app_get_present_mode().
TEST_CASE(test_app_present_mode_mailbox_failure_does_not_corrupt_state)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	REQUIRE(cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC));
	REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_VSYNC);

	bool ok = cf_app_set_present_mode(CF_PRESENT_MODE_MAILBOX);
	if (ok) {
		REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_MAILBOX);
	} else {
		REQUIRE(cf_app_get_present_mode() == CF_PRESENT_MODE_VSYNC);
		REQUIRE(cf_app_get_present_mode() != CF_PRESENT_MODE_MAILBOX);
	}

	destroy_app();

	return true;
}

// Regression test: cf_sdlgpu_supports_msaa used to cast the raw sample count straight to
// SDL_GPUSampleCount instead of converting it to the enum's index (1/2/4/8 -> 0/1/2/3), so
// cf_app_set_msaa(4) could never succeed on Metal/Vulkan/D3D12 (SDL_GPUSampleCount only goes
// up to 3) while cf_app_set_msaa(2) accidentally worked (raw value 2 happens to equal the
// enum index for 4x). This asserts 4x support whenever 2x is reported: red on this exact
// case pre-fix (2x yes, 4x no) on real Metal hardware, green after. 4x-given-2x isn't a
// documented guarantee on every backend (Vulkan's spec, for one, only mandates 1x), so this
// is an empirically-motivated regression check, not a proven cross-backend invariant.
TEST_CASE(test_app_msaa_4x_supported_when_2x_is)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	if (cf_app_set_msaa(2)) {
		REQUIRE(cf_app_set_msaa(4));
	}

	destroy_app();

	return true;
}

TEST_SUITE(test_app)
{
	RUN_TEST_CASE(test_app_destroy_safety);
	RUN_TEST_CASE(test_app_power_state_mapping);
	RUN_TEST_CASE(test_app_no_gfx_state_defaults);
	RUN_TEST_CASE(test_display_count_matches_list);
	RUN_TEST_CASE(test_display_invalid_id_is_safe);

	// Requires headless GPU context support in CI -- see
	// https://github.com/RandyGaul/cute_framework/pull/517
	RUN_TEST_CASE(test_app_set_canvas_size_is_persistent);
	RUN_TEST_CASE(test_app_msaa_change_preserves_canvas_size);
	RUN_TEST_CASE(test_app_present_mode_vsync_always_supported);
	RUN_TEST_CASE(test_app_present_mode_off_round_trip);
	RUN_TEST_CASE(test_app_present_mode_mailbox_failure_does_not_corrupt_state);
	RUN_TEST_CASE(test_app_msaa_4x_supported_when_2x_is);
}
