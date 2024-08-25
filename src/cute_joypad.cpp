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

#include <SDL3/SDL.h>

// TODO - Lock on joypad list? Race condition with pumping input messages.

void cf_joypad_system_init()
{
	static bool init;
	if (!init) {
		init = true;
		int result = SDL_AddGamepadMapping((const char*)joypad_mapping_db_data);
		CF_ASSERT(result != -1);
	}
}

CF_Result cf_joypad_add_mapping(const char* mapping)
{
	int result = SDL_AddGamepadMapping(mapping);
	if (result == -1) return cf_result_error("Failed to add mapping.");
	else return cf_result_success();
}

int cf_joypad_count()
{
	int count = 0;
	SDL_GetGamepads(&count);
	return count;
}

CF_Joypad cf_joypad_open(int index)
{
	cf_joypad_system_init();
	CF_Joypad handle = { 0 };
	if (SDL_IsGamepad(index)) {
		SDL_Gamepad* controller = SDL_OpenGamepad(index);
		if (controller) {
			SDL_Joystick* joy = SDL_GetGamepadJoystick(controller);
			CF_JoypadInstance* joypad = CF_NEW(CF_JoypadInstance);
			joypad->controller = controller;
			joypad->id = SDL_GetJoystickID(joy);
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
	SDL_CloseGamepad(joypad->controller);
	CF_FREE(joypad);
}

bool cf_joypad_is_connected(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GamepadConnected(joypad->controller);
}

CF_JoypadPowerLevel cf_joypad_power_level(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	int percent = 0;
	SDL_PowerState state = SDL_GetGamepadPowerInfo(joypad->controller, &percent);
	if (state == SDL_POWERSTATE_CHARGED || state == SDL_POWERSTATE_CHARGED || state == SDL_POWERSTATE_NO_BATTERY) {
		return CF_JOYPAD_POWER_LEVEL_WIRED;
	} else {
		if (percent >= 75) {
			return CF_JOYPAD_POWER_LEVEL_FULL;
		}
		else if (percent >= 50) {
			return CF_JOYPAD_POWER_LEVEL_MEDIUM;
		} else {
			return CF_JOYPAD_POWER_LEVEL_LOW;
		}
	}
	return CF_JOYPAD_POWER_LEVEL_UNKNOWN;
}

const char* cf_joypad_name(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadName(joypad->controller);
}

CF_JoypadType cf_joypad_type(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return (CF_JoypadType)SDL_GetGamepadType(joypad->controller);
}

uint16_t cf_joypad_vendor(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadVendor(joypad->controller);
}

uint16_t cf_joypad_product_id(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadProduct(joypad->controller);
}

const char* cf_joypad_serial_number(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadSerial(joypad->controller);
}

uint16_t cf_joypad_firmware_version(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadFirmwareVersion(joypad->controller);
}

uint16_t cf_joypad_product_version(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	return SDL_GetGamepadProductVersion(joypad->controller);
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
