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
	// Queries must be safe (not crash) after destruction -- the cute_main.h glue
	// calls cf_app_is_running right after a user's cf_main_update, which may have destroyed
	// the app.
	REQUIRE(!cf_app_is_running());
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

TEST_CASE(test_app_main_callbacks_event_buffering)
{
	// Feeding an event with no app must be a safe no-op.
	SDL_Event event = { };
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.scancode = SDL_SCANCODE_SPACE;
	event.key.repeat = false;
	cf_app_process_event(&event);

	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// Events fed from SDL_AppEvent are buffered, not applied immediately -- otherwise
	// cf_app_update's begin-frame copy of key state to `prev` would erase the transition
	// and just_pressed could never fire in callback mode.
	cf_app_process_event(&event);
	REQUIRE(!cf_key_down(CF_KEY_SPACE));

	cf_app_update(NULL);
	REQUIRE(cf_key_down(CF_KEY_SPACE));
	REQUIRE(cf_key_just_pressed(CF_KEY_SPACE));

	// A stray cf_app_process_event call must NOT flip the app into callback mode --
	// that would permanently disable the internal event pump for classic-loop apps.
	// Only CF_APP_OPTIONS_MAIN_CALLBACKS_BIT enables callback mode.
	REQUIRE(!(cf_app_get_options() & CF_APP_OPTIONS_MAIN_CALLBACKS_BIT));

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_main_callbacks_text_event_deep_copy)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// SDL text events carry pointers into SDL temporary memory that is freed before the
	// next update runs -- the buffer must deep-copy the string, not the pointer. Simulate
	// the free by clobbering the string after feeding the event.
	char text[8];
	CF_STRCPY(text, "hi");
	SDL_Event event = { };
	event.type = SDL_EVENT_TEXT_INPUT;
	event.text.text = text;
	cf_app_process_event(&event);
	CF_MEMSET(text, 'X', sizeof(text) - 1);
	text[sizeof(text) - 1] = 0;

	cf_app_update(NULL);
	REQUIRE(cf_input_text_has_data());
	// cf_input_text_pop_utf32 pops from the end of the buffer.
	REQUIRE(cf_input_text_pop_utf32() == (int)'i');
	REQUIRE(cf_input_text_pop_utf32() == (int)'h');
	cf_input_text_clear();

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_main_callbacks_null_text_is_safe)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// s_deep_copy_event's SDL_strdup can return NULL under OOM, leaving text.text/edit.text
	// NULL -- s_handle_event must not dereference it unconditionally.
	SDL_Event text_input = { };
	text_input.type = SDL_EVENT_TEXT_INPUT;
	text_input.text.text = NULL;
	cf_app_process_event(&text_input);

	SDL_Event text_editing = { };
	text_editing.type = SDL_EVENT_TEXT_EDITING;
	text_editing.edit.text = NULL;
	text_editing.edit.start = 3;
	text_editing.edit.length = 2;
	cf_app_process_event(&text_editing);

	cf_app_update(NULL);
	REQUIRE(!cf_input_text_has_data());
	REQUIRE(app->ime_composition_cursor == 0);
	REQUIRE(app->ime_composition_selection_len == 0);

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_main_callbacks_event_buffer_cap)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// The buffer must not grow without bound if events arrive while the app is not
	// updating; oldest events are dropped first.
	SDL_Event event = { };
	event.type = SDL_EVENT_MOUSE_WHEEL;
	for (int i = 0; i < CF_MAX_BUFFERED_EVENTS + 10; ++i) {
		cf_app_process_event(&event);
	}
	REQUIRE(app->buffered_events.count() == CF_MAX_BUFFERED_EVENTS);

	cf_app_update(NULL);
	REQUIRE(app->buffered_events.count() == 0);

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_touch_motion_updates_stored_touch)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	SDL_Event event = { };
	event.type = SDL_EVENT_FINGER_DOWN;
	event.tfinger.fingerID = 7;
	event.tfinger.x = 0.25f;
	event.tfinger.y = 0.25f;
	event.tfinger.pressure = 0.5f;
	cf_app_process_event(&event);
	cf_app_update(NULL);

	// Finger motion must update the touch stored in the app, not a local copy.
	event.type = SDL_EVENT_FINGER_MOTION;
	event.tfinger.x = 0.75f;
	event.tfinger.y = 0.75f;
	event.tfinger.pressure = 1.0f;
	cf_app_process_event(&event);
	cf_app_update(NULL);

	CF_Touch touch = { };
	REQUIRE(cf_touch_get(7, &touch));
	REQUIRE(touch.pressure == 1.0f);

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_main_callbacks_quit_event)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	REQUIRE(cf_app_is_running());

	SDL_Event event = { };
	event.type = SDL_EVENT_QUIT;
	cf_app_process_event(&event);
	REQUIRE(cf_app_is_running());

	cf_app_update(NULL);
	REQUIRE(!cf_app_is_running());

	cf_destroy_app();
	return true;
}

TEST_CASE(test_app_main_callbacks_option_bit)
{
	// The bit is what turns the internal pump off, and cf_app_get_options reports it back --
	// the cute_main.h glue reads it to catch an app that forgot to pass it.
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT | CF_APP_OPTIONS_MAIN_CALLBACKS_BIT, NULL)));
	REQUIRE(cf_app_get_options() & CF_APP_OPTIONS_MAIN_CALLBACKS_BIT);
	cf_destroy_app();

	// Unlike the process-wide latch this replaced, the mode is per-app: recreating without
	// the bit must come back in classic-loop mode, with no lingering process state.
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	REQUIRE(!(cf_app_get_options() & CF_APP_OPTIONS_MAIN_CALLBACKS_BIT));
	cf_destroy_app();

	// Null-safe outside an app's lifetime, so the glue's guard can't fault when cf_main_init
	// returns success without ever calling cf_make_app.
	REQUIRE(cf_app_get_options() == 0);
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

TEST_CASE(test_app_set_canvas_size_is_one_shot)
{
	REQUIRE(!cf_is_error(cf_make_app(NULL, 0, 0, 0, 200, 100, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	OwnedAppGuard guard;

	// The default canvas tracks the window at window_points * pixel_scale.
	float scale = cf_app_get_pixel_scale();
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(200 * scale));
	REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(100 * scale));

	// An explicit resize takes effect immediately...
	cf_app_set_canvas_size(320, 180);
	REQUIRE(cf_app_get_canvas_width() == 320);
	REQUIRE(cf_app_get_canvas_height() == 180);

	// ...but is one-shot: the next recreation event snaps back to window * pixel_scale.
	cf_app_set_size(256, 128);
	scale = cf_app_get_pixel_scale();
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(256 * scale));
	REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(128 * scale));

	return true;
}

TEST_CASE(test_app_msaa_change_resets_canvas_size)
{
	REQUIRE(!cf_is_error(cf_make_app(NULL, 0, 0, 0, 200, 100, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));
	OwnedAppGuard guard;

	cf_app_set_canvas_size(320, 180);
	REQUIRE(cf_app_get_canvas_width() == 320);
	REQUIRE(cf_app_get_canvas_height() == 180);

	if (cf_app_set_msaa(2)) { // MSAA support varies by backend/driver.
		// An MSAA change is a recreation event like any other -- the one-shot size does not persist.
		float scale = cf_app_get_pixel_scale();
		REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(200 * scale));
		REQUIRE(cf_app_get_canvas_height() == (int)CF_ROUNDF(100 * scale));
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
	RUN_TEST_CASE(test_app_main_callbacks_event_buffering);
	RUN_TEST_CASE(test_app_main_callbacks_text_event_deep_copy);
	RUN_TEST_CASE(test_app_main_callbacks_null_text_is_safe);
	RUN_TEST_CASE(test_app_main_callbacks_event_buffer_cap);
	RUN_TEST_CASE(test_app_touch_motion_updates_stored_touch);
	RUN_TEST_CASE(test_app_main_callbacks_quit_event);
	RUN_TEST_CASE(test_app_main_callbacks_option_bit);
	RUN_TEST_CASE(test_display_count_matches_list);
	RUN_TEST_CASE(test_display_invalid_id_is_safe);

	// Requires headless GPU context support in CI -- see
	// https://github.com/RandyGaul/cute_framework/pull/517
	RUN_TEST_CASE(test_app_set_canvas_size_is_one_shot);
	RUN_TEST_CASE(test_app_msaa_change_resets_canvas_size);
	RUN_TEST_CASE(test_app_present_mode_vsync_always_supported);
	RUN_TEST_CASE(test_app_present_mode_off_round_trip);
	RUN_TEST_CASE(test_app_present_mode_mailbox_failure_does_not_corrupt_state);
	RUN_TEST_CASE(test_app_msaa_4x_supported_when_2x_is);
}
