/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_INPUT_INTERNAL_H
#define CF_INPUT_INTERNAL_H

#include <cute_input.h>
#include <cute_joypad.h>
#include <cute_haptics.h>
#include <cute_doubly_list.h>

#include <SDL3/SDL.h>

enum CF_MouseClick
{
	CF_MOUSE_CLICK_SINGLE,
	CF_MOUSE_CLICK_DOUBLE,
};

struct CF_Haptic;

struct CF_JoypadInstance
{
	CF_ListNode node;
	SDL_Gamepad* controller = NULL;
	CF_Haptic haptic = { 0 };
	SDL_JoystickID id = -1;
	int buttons[CF_JOYPAD_BUTTON_COUNT] = { 0 };
	int buttons_prev[CF_JOYPAD_BUTTON_COUNT] = { 0 };
	int axes[CF_JOYPAD_AXIS_COUNT] = { 0 };
};

void cf_pump_input_msgs();

#ifdef CF_CPP

namespace Cute
{

using MouseClick = CF_MouseClick;
using JoypadInstance = CF_JoypadInstance;

}

#endif // CF_CPP

#endif // CF_INPUT_INTERNAL_H
