/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_haptics.h>
#include <cute_alloc.h>

#include <internal/cute_input_internal.h>

#include <SDL2/SDL.h>

namespace cute
{

struct haptic_t
{
	SDL_Haptic* ptr = NULL;
	bool rumble_initialized = false;
};

haptic_t* haptic_open(joypad_t* joypad)
{
	haptic_t* haptic = CUTE_NEW(haptic_t, NULL);
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(joypad->controller);
	if (!joy) {
		CUTE_FREE(haptic, NULL);
		return NULL;
	}
	haptic->ptr = SDL_HapticOpenFromJoystick(joy);
	if (!haptic->ptr) {
		CUTE_FREE(haptic, NULL);
		return NULL;
	}
	joypad->haptic = haptic;
	return haptic;
}

void haptic_close(haptic_t* haptic)
{
	SDL_HapticClose(haptic->ptr);
	CUTE_FREE(haptic, NULL);
}

bool haptic_supports(haptic_t* haptic, haptic_type_t type)
{
	int result = SDL_HapticQuery(haptic->ptr);
	if (type == HAPTIC_TYPE_LEFTRIGHT) {
		if (result & SDL_HAPTIC_LEFTRIGHT) {
			return true;
		}
	} else if (type == HAPTIC_TYPE_PERIODIC) {
		if (result & SDL_HAPTIC_SINE) {
			return true;
		}
	} else if (type == HAPTIC_TYPE_RAMP) {
		if (result & SDL_HAPTIC_RAMP) {
			return true;
		}
	}
	return false;
}

void haptic_set_gain(haptic_t* haptic, float gain)
{
	if (gain > 1.0f) gain = 1.0f;
	else if (gain < 0) gain = 0;
	SDL_HapticSetGain(haptic->ptr, (int)(gain * 100.0f + 0.5f));
}

static Sint16 s_f32_to_s16(float a)
{
	if (a > 1.0f) a = 1.0f;
	else if (a < 0) a = 0;
	return (Sint16)(int)(a * 32767.0f + 0.5f);
}

static Uint16 s_f32_to_u16(float a)
{
	if (a > 1.0f) a = 1.0f;
	else if (a < 0) a = 0;
	return (Uint16)(int)(a * 32767.0f + 0.5f);
}

static Uint32 s_saturate32(int a)
{
	if ((unsigned)a >= 0xFFFFFFFF) return 0xFFFFFFFF;
	else return (Uint32)a;
}

static Uint16 s_saturate16(int a)
{
	if (a > 0xFFFF) return 0xFFFF;
	else return (Uint16)a;
}

static SDL_HapticEffect s_cute_to_sdl(haptic_data_t data)
{
	SDL_HapticEffect effect = { 0 };
	if (data.type == HAPTIC_TYPE_LEFTRIGHT) {
		effect.type = SDL_HAPTIC_LEFTRIGHT;
		effect.leftright.length = s_saturate32(data.u.leftright.duration_milliseconds);
		effect.leftright.small_magnitude = s_f32_to_u16(data.u.leftright.lo_motor_strength);
		effect.leftright.large_magnitude = s_f32_to_u16(data.u.leftright.hi_motor_strength);
	} else if (data.type == HAPTIC_TYPE_PERIODIC) {
		if (data.u.periodic.wave_type == HAPTIC_WAVE_TYPE_SINE) {
			effect.type = SDL_HAPTIC_SINE;
		} else if (data.u.periodic.wave_type == HAPTIC_WAVE_TYPE_TRIANGLE) {
			effect.type = SDL_HAPTIC_TRIANGLE;
		} else if (data.u.periodic.wave_type == HAPTIC_WAVE_TYPE_SAW) {
			effect.type = SDL_HAPTIC_SAWTOOTHUP;
		}
		effect.periodic.length = s_saturate32(data.u.periodic.duration_milliseconds);
		effect.periodic.period = s_saturate16(data.u.periodic.period_milliseconds);
		effect.periodic.magnitude = s_f32_to_s16(data.u.periodic.magnitude);
		effect.periodic.attack_level = s_f32_to_u16(data.u.periodic.envelope.attack);
		effect.periodic.attack_length = s_saturate16(data.u.periodic.envelope.attack_milliseconds);
		effect.periodic.fade_level = s_f32_to_u16(data.u.periodic.envelope.fade);
		effect.periodic.fade_length = s_saturate16(data.u.periodic.envelope.fade_milliseconds);
	} else if (data.type == HAPTIC_TYPE_RAMP) {
		effect.type = SDL_HAPTIC_RAMP;
		effect.ramp.length = s_saturate32(data.u.ramp.duration_milliseconds);
		effect.ramp.start = s_f32_to_s16(data.u.ramp.start);
		effect.ramp.end = s_f32_to_s16(data.u.ramp.end);
		effect.ramp.attack_level = s_f32_to_u16(data.u.ramp.envelope.attack);
		effect.ramp.attack_length = s_saturate16(data.u.ramp.envelope.attack_milliseconds);
		effect.ramp.fade_level = s_f32_to_u16(data.u.ramp.envelope.fade);
		effect.ramp.fade_length = s_saturate16(data.u.ramp.envelope.fade_milliseconds);
	}
	return effect;
}

haptic_effect_t haptic_create_effect(haptic_t* haptic, haptic_data_t data)
{
	SDL_HapticEffect effect = s_cute_to_sdl(data);
	int id = SDL_HapticNewEffect(haptic->ptr, &effect);
	haptic_effect_t result = { id };
	return result;
}

void haptic_run_effect(haptic_t* haptic, haptic_effect_t effect, int iterations)
{
	SDL_HapticRunEffect(haptic->ptr, effect.id, s_saturate32(iterations));
}

void haptic_update_effect(haptic_t* haptic, haptic_effect_t effect, haptic_data_t data)
{
	SDL_HapticEffect update = s_cute_to_sdl(data);
	SDL_HapticUpdateEffect(haptic->ptr, effect.id, &update);
}

void haptic_stop_effect(haptic_t* haptic, haptic_effect_t effect)
{
	SDL_HapticStopEffect(haptic->ptr, effect.id);
}

void haptic_destroy_effect(haptic_t* haptic, haptic_effect_t effect)
{
	SDL_HapticDestroyEffect(haptic->ptr, effect.id);
}

void haptic_pause(haptic_t* haptic)
{
	SDL_HapticPause(haptic->ptr);
}

void haptic_unpause(haptic_t* haptic)
{
	SDL_HapticUnpause(haptic->ptr);
}

void haptic_stop_all(haptic_t* haptic)
{
	SDL_HapticStopAll(haptic->ptr);
}

bool haptic_rumble_supported(haptic_t* haptic)
{
	int result = SDL_HapticRumbleSupported(haptic->ptr);
	return result == SDL_TRUE ? true : false;
}

void haptic_rumble_play(haptic_t* haptic, float strength, int duration_milliseconds)
{
	if (!haptic->rumble_initialized) {
		SDL_HapticRumbleInit(haptic->ptr);
		haptic->rumble_initialized = true;
	}
	SDL_HapticRumblePlay(haptic->ptr, strength, s_saturate32(duration_milliseconds));
}

void haptic_rumble_stop(haptic_t* haptic)
{
	SDL_HapticRumbleStop(haptic->ptr);
}

}
