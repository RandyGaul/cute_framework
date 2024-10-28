/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_audio.h>
#include <cute_app.h>
#include <cute_multithreading.h>

using namespace Cute;

#include "thingy.h"
#include "jump.h"

/* Load and free wav/ogg files synchronously. */
TEST_CASE(test_audio_load_synchronous)
{
	CHECK(cf_is_error(cf_make_app(NULL, 0, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT, NULL)));
	CF_Audio audio = cf_audio_load_ogg_from_memory(thingy_data, thingy_sz);
	REQUIRE(audio.id);
	cf_audio_destroy(audio);
	audio = cf_audio_load_wav_from_memory(jump_data, jump_sz);
	REQUIRE(audio.id);
	cf_audio_destroy(audio);
	cf_destroy_app();

	return true;
}

TEST_SUITE(test_audio)
{
	RUN_TEST_CASE(test_audio_load_synchronous);
}
