/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_joypad.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

#include <internal/cute_input_internal.h>
#include <internal/cute_app_internal.h>
#include <data/cute_joypad_mapping_db.h>

#include <SDL3/SDL.h>

struct CF_Joypad
{
	SDL_Gamepad* gamepad = NULL;
	SDL_JoystickID id = 0;
	const char* serial = NULL;
	int buttons[CF_JOYPAD_BUTTON_COUNT] = { 0 };
	int buttons_prev[CF_JOYPAD_BUTTON_COUNT] = { 0 };
	int axes[CF_JOYPAD_AXIS_COUNT] = { 0 };
	int axes_prev[CF_JOYPAD_AXIS_COUNT] = { 0 };
};

CF_Joypad joypads[CF_MAX_JOYPADS];

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
	for (int i = 0; i < CF_MAX_JOYPADS; i++) {
		if (joypads[i].id) count++;
	}
	return count;
}

bool cf_joypad_is_connected(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	return joypads[player_index].id ? true : false;
}

CF_JoypadPowerLevel cf_joypad_power_level(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	CF_Joypad* joypad = &joypads[player_index];
	if (!joypad->id) return CF_JOYPAD_POWER_LEVEL_UNKNOWN;
	int percent = 0;
	SDL_PowerState state = SDL_GetGamepadPowerInfo(joypad->gamepad, &percent);

	if (state == SDL_POWERSTATE_CHARGED || state == SDL_POWERSTATE_NO_BATTERY) {
		return CF_JOYPAD_POWER_LEVEL_WIRED;
	} else if (percent >= 75) {
		return CF_JOYPAD_POWER_LEVEL_FULL;
	} else if (percent >= 50) {
		return CF_JOYPAD_POWER_LEVEL_MEDIUM;
	} else {
		return CF_JOYPAD_POWER_LEVEL_LOW;
	}
	return CF_JOYPAD_POWER_LEVEL_UNKNOWN;
}

const char* cf_joypad_name(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return NULL;
	return SDL_GetGamepadName(joypads[player_index].gamepad);
}

CF_JoypadType cf_joypad_type(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return CF_JOYPAD_TYPE_UNKNOWN;
	return (CF_JoypadType)SDL_GetGamepadType(joypads[player_index].gamepad);
}

uint16_t cf_joypad_vendor(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return SDL_GetGamepadVendor(joypads[player_index].gamepad);
}

uint16_t cf_joypad_product_id(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return SDL_GetGamepadProduct(joypads[player_index].gamepad);
}

const char* cf_joypad_serial_number(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return NULL;
	return SDL_GetGamepadSerial(joypads[player_index].gamepad);
}

uint16_t cf_joypad_firmware_version(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return SDL_GetGamepadFirmwareVersion(joypads[player_index].gamepad);
}

uint16_t cf_joypad_product_version(int player_index)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return SDL_GetGamepadProductVersion(joypads[player_index].gamepad);
}

bool cf_joypad_button_down(int player_index, CF_JoypadButton button)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return false;
	return joypads[player_index].buttons[button];
}

bool cf_joypad_button_just_pressed(int player_index, CF_JoypadButton button)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return false;
	return joypads[player_index].buttons[button] && !joypads[player_index].buttons_prev[button];
}

bool cf_joypad_button_just_released(int player_index, CF_JoypadButton button)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return false;
	return !joypads[player_index].buttons[button] && joypads[player_index].buttons_prev[button];
}

int16_t cf_joypad_axis(int player_index, CF_JoypadAxis axis)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return joypads[player_index].axes[axis];
}

int16_t cf_joypad_axis_prev(int player_index, CF_JoypadAxis axis)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return 0;
	return joypads[player_index].axes_prev[axis];
}

void cf_joypad_rumble(int player_index, uint16_t lo_frequency_rumble, uint16_t hi_frequency_rumble, int duration_ms)
{
	CF_ASSERT(player_index >= 0 && player_index < CF_MAX_JOYPADS);
	if (!joypads[player_index].id) return;
	SDL_RumbleGamepad(joypads[player_index].gamepad, lo_frequency_rumble, hi_frequency_rumble, (Uint32)duration_ms);
}

void cf_joypad_update()
{
	int num_gamepads = 0;
	SDL_JoystickID* gamepad_ids = SDL_GetGamepads(&num_gamepads);

	// Handle new joypad connections.
	for (int i = 0; i < num_gamepads; ++i) {
		SDL_JoystickID id = gamepad_ids[i];

		// Try and connect the gamepad.
		SDL_Gamepad* gamepad = SDL_GetGamepadFromID(id);
		if (!gamepad) {
			gamepad = SDL_OpenGamepad(id);
			if (!gamepad) {
				CF_WARN("Failed to open gamepad with ID %d.", id);
				continue;
			}
		}

		// Skip if already connected.
		bool already_connected = false;
		for (int i = 0; i < CF_MAX_JOYPADS; ++i) {
			const char* serial = SDL_GetGamepadSerial(gamepad);
			if (joypads[i].id == id || (serial && sequ(joypads->serial, serial))) {
				already_connected = true;
				break;
			}
		}
		if (already_connected) continue;

		// This is a new gamepad instance, map it to next available player index.
		int player_index = -1;
		for (int i = 0; i < CF_MAX_JOYPADS; ++i) {
			if (!joypads[i].id) {
				player_index = i;
				break;
			}
		}
		if (player_index != -1) {
			SDL_SetGamepadPlayerIndex(gamepad, player_index);
			joypads[player_index].id = id;
			joypads[player_index].gamepad = gamepad;
			joypads[player_index].serial = SDL_GetGamepadSerial(gamepad);
		} else {
			SDL_CloseGamepad(gamepad);
		}
	}

	SDL_free(gamepad_ids);

	// Handle disconnections (close gamepads that are no longer connected).
	for (int i = 0; i < CF_MAX_JOYPADS; ++i) {
		CF_Joypad* joypad = &joypads[i];
		if (joypad->id && !SDL_GamepadConnected(joypad->gamepad)) {
			SDL_CloseGamepad(joypad->gamepad);
			*joypad = {};
		}
	}

	// Copy over prev inputs.
	for (int i = 0; i < CF_MAX_JOYPADS; ++i) {
		CF_Joypad* joypad = &joypads[i];
		CF_MEMCPY(joypad->buttons_prev, joypad->buttons, sizeof(joypad->buttons));
		CF_MEMCPY(joypad->axes_prev, joypad->axes, sizeof(joypad->axes));
	}
}

void cf_joypad_on_button_up(SDL_JoystickID id, int button)
{
	int player_index = SDL_GetGamepadPlayerIndexForID(id);
	if (player_index < 0 || player_index >= CF_MAX_JOYPADS) return;
	CF_Joypad* joypad = &joypads[player_index];
	if (joypad->id && joypad->id == id) {
		joypad->buttons[button] = 0;
	}
}

void cf_joypad_on_button_down(SDL_JoystickID id, int button)
{
	int player_index = SDL_GetGamepadPlayerIndexForID(id);
	if (player_index < 0 || player_index >= CF_MAX_JOYPADS) return;
	CF_Joypad* joypad = &joypads[player_index];
	if (joypad->id && joypad->id == id) {
		joypad->buttons[button] = 1;
	}
}

void cf_joypad_on_axis_motion(SDL_JoystickID id, int axis, int value)
{
	int player_index = SDL_GetGamepadPlayerIndexForID(id);
	if (player_index < 0 || player_index >= CF_MAX_JOYPADS) return;
	CF_Joypad* joypad = &joypads[player_index];
	if (joypad->id && joypad->id == id) {
		joypad->axes[axis] = value;
	}
}