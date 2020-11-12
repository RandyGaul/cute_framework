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

#ifndef CUTE_APP_H
#define CUTE_APP_H

#include <cute_defines.h>
#include <cute_error.h>

struct ImGuiContext;

namespace cute
{

struct strpool_t;

#define CUTE_APP_OPTIONS_OPENGL_CONTEXT                 (1 << 0)
#define CUTE_APP_OPTIONS_OPENGLES_CONTEXT               (1 << 1)
#define CUTE_APP_OPTIONS_D3D11_CONTEXT                  (1 << 2)
#define CUTE_APP_OPTIONS_FULLSCREEN                     (1 << 3)
#define CUTE_APP_OPTIONS_RESIZABLE                      (1 << 4)
#define CUTE_APP_OPTIONS_HIDDEN                         (1 << 5)
#define CUTE_APP_OPTIONS_WINDOW_POS_CENTERED            (1 << 6)
#define CUTE_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT (1 << 7)

CUTE_API app_t* CUTE_CALL app_make(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, const char* argv0 = NULL, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL app_destroy(app_t* app);

CUTE_API bool CUTE_CALL app_is_running(app_t* app);
CUTE_API void CUTE_CALL app_stop_running(app_t* app);
CUTE_API void CUTE_CALL app_update(app_t* app, float dt);
CUTE_API void CUTE_CALL app_present(app_t* app);

CUTE_API error_t CUTE_CALL app_init_net(app_t* app);
CUTE_API error_t CUTE_CALL app_init_audio(app_t* app, bool spawn_mix_thread = true, int max_simultaneous_sounds = 5000);
CUTE_API void CUTE_CALL app_do_mixing(app_t* app);
CUTE_API ImGuiContext* CUTE_CALL app_init_imgui(app_t* app);
CUTE_API strpool_t* CUTE_CALL app_get_strpool(app_t* app);

enum upscale_t
{
	UPSCALE_PIXEL_PERFECT_AT_LEAST_1X,
	UPSCALE_PIXEL_PERFECT_AT_LEAST_2X,
	UPSCALE_PIXEL_PERFECT_AT_LEAST_3X,
	UPSCALE_PIXEL_PERFECT_AT_LEAST_4X,
	UPSCALE_STRETCH,
};

CUTE_API error_t CUTE_CALL app_init_upscaling(app_t* app, upscale_t upscaling, int offscreen_w, int offscreen_h);
CUTE_API void CUTE_CALL app_offscreen_size(app_t* app, int* offscreen_w, int* offscreen_h);

enum power_state_t
{
	POWER_STATE_UNKNOWN,    // Cannot determine power status.
	POWER_STATE_ON_BATTERY, // Not plugged in and running on battery.
	POWER_STATE_NO_BATTERY, // Plugged in with no battery available.
	POWER_STATE_CHARGING,   // Plugged in and charging battery.
	POWER_STATE_CHARGED,    // Plugged in and battery is charged.
};

struct power_info_t
{
	power_state_t state;
	int seconds_left;    // The seconds of battery life left. -1 means not running on the battery, or unable to get a valid value.
	int percentage_left; // The percentage of battery life left from 0 to 100. -1 means not running on the battery, or unable to get a valid value.
};

CUTE_API power_info_t CUTE_CALL app_power_info(app_t* app);

}

#endif // CUTE_APP_H
