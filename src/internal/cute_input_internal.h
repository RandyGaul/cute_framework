/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_INPUT_INTERNAL_H
#define CF_INPUT_INTERNAL_H

#include <cute_input.h>
#include <cute_joypad.h>

#include <SDL3/SDL.h>

enum CF_MouseClick
{
	CF_MOUSE_CLICK_SINGLE,
	CF_MOUSE_CLICK_DOUBLE,
};

struct CF_Haptic;

void cf_begin_frame_input();
void cf_pump_input_msgs();
void cf_joypad_update();
void cf_joypad_on_button_up(SDL_JoystickID id, int button);
void cf_joypad_on_button_down(SDL_JoystickID id, int button);
void cf_joypad_on_axis_motion(SDL_JoystickID id, int axis, int value);

#endif // CF_INPUT_INTERNAL_H
