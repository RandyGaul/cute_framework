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

#include <stdio.h>

#include <cute_app.h>
#include <cute_timer.h>
#include <cute_input.h>
#include <cute_window.h>
#include <cute_audio.h>
#include <cute_file_system_utils.h>

cute::audio_t* jump_audio = NULL;
cute::audio_t* music_audio = NULL;

void load_jump_promise(cute::error_t status, void* param, void* promise_udata)
{
	jump_audio = (cute::audio_t*)param;
	printf("Loaded jump.wav\n");
}

void load_music_promise(cute::error_t status, void* param, void* promise_udata)
{
	music_audio = (cute::audio_t*)param;
	printf("Loaded 3-6-19-blue-suit-jam.ogg\n");
}

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_GFX_D3D9 | CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	cute::app_t* app = cute::app_make("Cute Snake", 0, 0, 640, 480, options);

	cute::audio_stream_wav(app, "jump.wav", cute::promise_t(load_jump_promise));
	cute::audio_stream_ogg(app, "3-6-19-blue-suit-jam.ogg", cute::promise_t(load_music_promise));

	printf("Running from: %s\n", cute::file_system_get_working_directory());

	while (cute::app_is_running(app)) {
		float dt = cute::calc_dt();
		cute::app_update(app, dt);

		if (cute::key_was_pressed(app, cute::KEY_SPACE)) {
			printf("space\n");
			if (jump_audio) cute::sound_play(app, jump_audio);
		}

		if (cute::key_was_pressed(app, cute::KEY_1)) {
			printf("key 1\n");
			if (music_audio) cute::music_play(app, music_audio);
		}

		if (cute::mouse_was_pressed(app, cute::MOUSE_BUTTON_LEFT)) {
			printf("left click\n");
		}

		if (cute::mouse_double_click_was_pressed(app, cute::MOUSE_BUTTON_LEFT)) {
			printf("left double click\n");
		}

		if (cute::app_window_mouse_entered(app)) {
			printf("mouse entered\n");
		}

		if (cute::app_window_was_minimized(app)) {
			printf("minimized\n");
		}

		if (cute::app_window_was_maximized(app)) {
			printf("maximized\n");
		}

		if (cute::app_window_was_restored(app)) {
			printf("restored\n");
		}

		if (cute::app_window_keyboard_gained_focus(app)) {
			printf("gained keyboard focus\n");
		}

		if (cute::app_window_keyboard_lost_focus(app)) {
			printf("lost keyboard focus\n");
		}

		if (cute::app_window_was_size_changed(app)) {
			int w, h;
			cute::app_window_size(app, &w, &h);
			printf("size changed to %d, %d\n", w, h);
		}

		if (cute::app_window_was_moved(app)) {
			int x, y;
			cute::app_window_position(app, &x, &y);
			printf("size moved to %d, %d\n", x, y);
		}
	}

	if (jump_audio) cute::audio_destroy(jump_audio);
	if (music_audio) cute::audio_destroy(music_audio);

	return 0;
}
