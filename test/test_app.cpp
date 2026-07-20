/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
using namespace Cute;

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

TEST_SUITE(test_app)
{
	// Requires headless GPU context support in CI -- see
	// https://github.com/RandyGaul/cute_framework/pull/517
	RUN_TEST_CASE(test_app_present_mode_vsync_always_supported);
	RUN_TEST_CASE(test_app_present_mode_off_round_trip);
	RUN_TEST_CASE(test_app_present_mode_mailbox_failure_does_not_corrupt_state);
}
