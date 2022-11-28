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

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_graphics.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct ImGuiContext ImGuiContext;
typedef struct sg_imgui_t sg_imgui_t;
typedef struct sg_image sg_image;

#define CF_APP_OPTION_DEFS \
	CF_ENUM(APP_OPTIONS_OPENGL_CONTEXT,                 1 << 0) \
	CF_ENUM(APP_OPTIONS_OPENGLES_CONTEXT,               1 << 1) \
	CF_ENUM(APP_OPTIONS_D3D11_CONTEXT,                  1 << 2) \
	CF_ENUM(APP_OPTIONS_DEFAULT_GFX_CONTEXT,            1 << 3) \
	CF_ENUM(APP_OPTIONS_FULLSCREEN,                     1 << 4) \
	CF_ENUM(APP_OPTIONS_RESIZABLE,                      1 << 5) \
	CF_ENUM(APP_OPTIONS_HIDDEN,                         1 << 6) \
	CF_ENUM(APP_OPTIONS_WINDOW_POS_CENTERED,            1 << 7) \
	CF_ENUM(APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT, 1 << 8) \
	CF_ENUM(APP_OPTIONS_NO_AUDIO,                       1 << 9) \

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_APP_OPTION_DEFS
	#undef CF_ENUM
};

CUTE_API CF_Result CUTE_CALL cf_make_app(const char* window_title, int x, int y, int w, int h, int options /*= 0*/, const char* argv0 /*= NULL*/);
CUTE_API void CUTE_CALL cf_destroy_app();

CUTE_API bool CUTE_CALL cf_app_is_running();
CUTE_API void CUTE_CALL cf_app_stop_running();
CUTE_API void CUTE_CALL cf_app_update(float dt);
CUTE_API void CUTE_CALL cf_app_present();

CUTE_API ImGuiContext* CUTE_CALL cf_app_init_imgui(bool no_default_font /*= false*/);
CUTE_API sg_imgui_t* CUTE_CALL cf_app_get_sokol_imgui();

CUTE_API CF_Texture CUTE_CALL cf_app_get_backbuffer();
CUTE_API CF_Texture CUTE_CALL cf_app_get_backbuffer_depth_stencil();
CUTE_API void CUTE_CALL cf_app_get_backbuffer_size(int* x, int* y);

#define CF_POWER_STATE_DEFS \
	CF_ENUM(POWER_STATE_UNKNOWN, 0)    /* Cannot determine power status. */ \
	CF_ENUM(POWER_STATE_ON_BATTERY, 1) /* Not plugged in and running on battery. */ \
	CF_ENUM(POWER_STATE_NO_BATTERY, 2) /* Plugged in with no battery available. */ \
	CF_ENUM(POWER_STATE_CHARGING, 3)   /* Plugged in and charging battery. */ \
	CF_ENUM(POWER_STATE_CHARGED, 4)    /* Plugged in and battery is charged. */ \

typedef enum cf_power_state_t
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_POWER_STATE_DEFS
	#undef CF_ENUM
} cf_power_state_t;

typedef struct cf_power_info_t
{
	cf_power_state_t state;
	int seconds_left;    // The seconds of battery life left. -1 means not running on the battery, or unable to get a valid value.
	int percentage_left; // The percentage of battery life left from 0 to 100. -1 means not running on the battery, or unable to get a valid value.
} cf_power_info_t;

CUTE_API cf_power_info_t CUTE_CALL cf_app_power_info();

// TODO - Where to put this?
CUTE_API void CUTE_CALL cf_sleep(int milliseconds);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using power_info_t = cf_power_info_t;

enum power_state_t : int
{
	#define CF_ENUM(K, V) K = V,
	CF_POWER_STATE_DEFS
	#undef CF_ENUM
};

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_APP_OPTION_DEFS
	#undef CF_ENUM
};

CUTE_INLINE result_t make_app(const char* window_title, int x, int y, int w, int h, uint32_t options = 0, const char* argv0 = NULL) { return cf_make_app(window_title, x, y, w, h, options, argv0); }
CUTE_INLINE void destroy_app() { cf_destroy_app(); }
CUTE_INLINE bool app_is_running() { return cf_app_is_running(); }
CUTE_INLINE void app_stop_running() { cf_app_stop_running(); }
CUTE_INLINE void app_update(float dt) { cf_app_update(dt); }
CUTE_INLINE CF_Texture app_get_backbuffer() { return cf_app_get_backbuffer(); }
CUTE_INLINE void app_present() { cf_app_present(); }
CUTE_INLINE ImGuiContext* app_init_imgui(bool no_default_font = false) { return cf_app_init_imgui(no_default_font); }
CUTE_INLINE sg_imgui_t* app_get_sokol_imgui() { return cf_app_get_sokol_imgui(); }
CUTE_INLINE power_info_t app_power_info() { return cf_app_power_info(); }
CUTE_INLINE void sleep(int milliseconds) { cf_sleep(milliseconds); }

}

#endif // CUTE_CPP

#endif // CUTE_APP_H
