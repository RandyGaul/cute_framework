/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_joypad.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>
#include <cute_haptics.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/cute_app_internal.h>
#include <data/cute_joypad_mapping_db.h>

#include <SDL.h>

// TODO - Lock on joypad list? Race condition with pumping input messages.

void cf_joypad_system_init()
{
	static bool init;
	if (!init) {
		init = true;
		int result = SDL_GameControllerAddMapping((const char*)joypad_mapping_db_data);
		CF_ASSERT(result != -1);
	}
}

CF_Result cf_joypad_add_mapping(const char* mapping)
{
	int result = SDL_GameControllerAddMapping(mapping);
	if (result == -1) return cf_result_error("Failed to add mapping.");
	else return cf_result_success();
}

int cf_joypad_count()
{
	return SDL_NumJoysticks();
}

CF_Joypad cf_joypad_open(int index)
{
	cf_joypad_system_init();
	CF_Joypad handle = { 0 };
	if (SDL_IsGameController(index)) {
		SDL_GameController* controller = SDL_GameControllerOpen(index);
		if (controller) {
			SDL_Joystick* joy = SDL_GameControllerGetJoystick(controller);
			CF_JoypadInstance* joypad = CF_NEW(CF_JoypadInstance);
			joypad->controller = controller;
			joypad->id = SDL_JoystickInstanceID(joy);
			cf_list_push_front(&app->joypads, &joypad->node);
			handle.id = (uint64_t)joypad;
			return handle;
		}
	}
	return handle;
}

void cf_joypad_close(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	cf_list_remove(&joypad->node);
	if (joypad->haptic.id) {
		cf_haptic_close(joypad->haptic);
	}
	SDL_GameControllerClose(joypad->controller);
	CF_FREE(joypad);
}

bool cf_joypad_is_connected(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetAttached(joypad->controller);
}

CF_JoypadPowerLevel cf_joypad_power_level(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(joypad->controller);
	SDL_JoystickPowerLevel level = SDL_JoystickCurrentPowerLevel(joy);
	switch (level) {
	case SDL_JOYSTICK_POWER_UNKNOWN: return CF_JOYPAD_POWER_LEVEL_UNKNOWN;
	case SDL_JOYSTICK_POWER_EMPTY: return CF_JOYPAD_POWER_LEVEL_EMPTY;
	case SDL_JOYSTICK_POWER_LOW: return CF_JOYPAD_POWER_LEVEL_LOW;
	case SDL_JOYSTICK_POWER_MEDIUM: return CF_JOYPAD_POWER_LEVEL_MEDIUM;
	case SDL_JOYSTICK_POWER_FULL: return CF_JOYPAD_POWER_LEVEL_FULL;
	case SDL_JOYSTICK_POWER_WIRED: return CF_JOYPAD_POWER_LEVEL_WIRED;
	case SDL_JOYSTICK_POWER_MAX: return CF_JOYPAD_POWER_LEVEL_COUNT;
	}
	return CF_JOYPAD_POWER_LEVEL_UNKNOWN;
}

const char* cf_joypad_name(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerName(joypad->controller);
}

CF_JoypadType cf_joypad_type(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return (CF_JoypadType)SDL_GameControllerGetType(joypad->controller);
}

uint16_t cf_joypad_vendor(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetVendor(joypad->controller);
}

uint16_t cf_joypad_product_id(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetProduct(joypad->controller);
}

const char* cf_joypad_serial_number(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetSerial(joypad->controller);
}

uint16_t cf_joypad_firmware_version(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetFirmwareVersion(joypad->controller);
}

uint16_t cf_joypad_product_version(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GameControllerGetProductVersion(joypad->controller);
}

bool cf_joypad_button_down(CF_Joypad joypad_handle, CF_JoypadButton button)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return joypad->buttons[button];
}

bool cf_joypad_button_just_pressed(CF_Joypad joypad_handle, CF_JoypadButton button)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return joypad->buttons[button] && !joypad->buttons_prev[button];
}

bool cf_joypad_button_just_released(CF_Joypad joypad_handle, CF_JoypadButton button)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return !joypad->buttons[button] && joypad->buttons_prev[button];
}

int16_t cf_joypad_axis(CF_Joypad joypad_handle, CF_JoypadAxis axis)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return joypad->axes[axis];
}
