/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_audio.h>
#include <cute_app.h>
using namespace cute;

CUTE_TEST_CASE(test_audio_load_synchronous, "Load and free wav/ogg files synchronously.");
int test_audio_load_synchronous()
{
	audio_t* audio = audio_load_ogg("3-6-19-blue-suit-jam.ogg");
	CUTE_TEST_CHECK_POINTER(audio);
	CUTE_TEST_ASSERT(!audio_destroy(audio).is_error());

	audio = audio_load_wav("jump.wav");
	CUTE_TEST_CHECK_POINTER(audio);
	CUTE_TEST_ASSERT(!audio_destroy(audio).is_error());

	return 0;
}

static error_t s_audio_error;
static audio_t* s_audio;

static void s_audio_promise(error_t status, void* param, void* udata)
{
	s_audio_error = status;
	s_audio = (audio_t*)param;
	CUTE_ASSERT(udata == NULL);
}

CUTE_TEST_CASE(test_audio_load_asynchronous, "Load and free wav/ogg files asynchronously.");
int test_audio_load_asynchronous()
{
	app_t* app = app_make("audio test", 0, 0, 0, 0, APP_OPTIONS_HEADLESS);
	CUTE_TEST_CHECK_POINTER(app);

	promise_t promise;
	promise.callback = s_audio_promise;

	s_audio_error = error_success();
	s_audio = NULL;
	audio_stream_ogg(app, "3-6-19-blue-suit-jam.ogg", promise);

	while (!s_audio)
		;

	CUTE_TEST_ASSERT(!audio_destroy(s_audio).is_error());

	s_audio_error = error_success();
	s_audio = NULL;
	audio_stream_wav(app, "jump.wav", promise);

	while (!s_audio)
		;

	CUTE_TEST_ASSERT(!audio_destroy(s_audio).is_error());

	app_destroy(app);

	return 0;
}
