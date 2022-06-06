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

#include <SDL.h>

struct cf_haptic_t
{
	SDL_Haptic* ptr = NULL;
	bool rumble_initialized = false;
};

cf_haptic_t* cf_haptic_open(cf_joypad_t* joypad)
{
	cf_haptic_t* haptic = CUTE_NEW(cf_haptic_t, NULL);
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

void cf_haptic_close(cf_haptic_t* haptic)
{
	SDL_HapticClose(haptic->ptr);
	CUTE_FREE(haptic, NULL);
}

bool cf_haptic_supports(cf_haptic_t* haptic, cf_haptic_type_t type)
{
	int result = SDL_HapticQuery(haptic->ptr);
	if (type == CF_HAPTIC_TYPE_LEFTRIGHT) {
		if (result & SDL_HAPTIC_LEFTRIGHT) {
			return true;
		}
	} else if (type == CF_HAPTIC_TYPE_PERIODIC) {
		if (result & SDL_HAPTIC_SINE) {
			return true;
		}
	} else if (type == CF_HAPTIC_TYPE_RAMP) {
		if (result & SDL_HAPTIC_RAMP) {
			return true;
		}
	}
	return false;
}

void cf_haptic_set_gain(cf_haptic_t* haptic, float gain)
{
	if (gain > 1.0f) gain = 1.0f;
	else if (gain < 0) gain = 0;
	SDL_HapticSetGain(haptic->ptr, (int)(gain * 100.0f + 0.5f));
}

static Sint16 cf_s_f32_to_s16(float a)
{
	if (a > 1.0f) a = 1.0f;
	else if (a < 0) a = 0;
	return (Sint16)(int)(a * 32767.0f + 0.5f);
}

static Uint16 cf_s_f32_to_u16(float a)
{
	if (a > 1.0f) a = 1.0f;
	else if (a < 0) a = 0;
	return (Uint16)(int)(a * 32767.0f + 0.5f);
}

static Uint32 cf_s_saturate32(int a)
{
	if ((unsigned)a >= 0xFFFFFFFF) return 0xFFFFFFFF;
	else return (Uint32)a;
}

static Uint16 cf_s_saturate16(int a)
{
	if (a > 0xFFFF) return 0xFFFF;
	else return (Uint16)a;
}

static SDL_HapticEffect cf_s_cute_to_sdl(cf_haptic_data_t data)
{
	SDL_HapticEffect effect = { 0 };
	if (data.type == CF_HAPTIC_TYPE_LEFTRIGHT) {
		effect.type = SDL_HAPTIC_LEFTRIGHT;
		effect.leftright.length = cf_s_saturate32(data.u.leftright.duration_milliseconds);
		effect.leftright.small_magnitude = cf_s_f32_to_u16(data.u.leftright.lo_motor_strength);
		effect.leftright.large_magnitude = cf_s_f32_to_u16(data.u.leftright.hi_motor_strength);
	} else if (data.type == CF_HAPTIC_TYPE_PERIODIC) {
		if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_SINE) {
			effect.type = SDL_HAPTIC_SINE;
		} else if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_TRIANGLE) {
			effect.type = SDL_HAPTIC_TRIANGLE;
		} else if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_SAW) {
			effect.type = SDL_HAPTIC_SAWTOOTHUP;
		}
		effect.periodic.length = cf_s_saturate32(data.u.periodic.duration_milliseconds);
		effect.periodic.period = cf_s_saturate16(data.u.periodic.period_milliseconds);
		effect.periodic.magnitude = cf_s_f32_to_s16(data.u.periodic.magnitude);
		effect.periodic.attack_level = cf_s_f32_to_u16(data.u.periodic.envelope.attack);
		effect.periodic.attack_length = cf_s_saturate16(data.u.periodic.envelope.attack_milliseconds);
		effect.periodic.fade_level = cf_s_f32_to_u16(data.u.periodic.envelope.fade);
		effect.periodic.fade_length = cf_s_saturate16(data.u.periodic.envelope.fade_milliseconds);
	} else if (data.type == CF_HAPTIC_TYPE_RAMP) {
		effect.type = SDL_HAPTIC_RAMP;
		effect.ramp.length = cf_s_saturate32(data.u.ramp.duration_milliseconds);
		effect.ramp.start = cf_s_f32_to_s16(data.u.ramp.start);
		effect.ramp.end = cf_s_f32_to_s16(data.u.ramp.end);
		effect.ramp.attack_level = cf_s_f32_to_u16(data.u.ramp.envelope.attack);
		effect.ramp.attack_length = cf_s_saturate16(data.u.ramp.envelope.attack_milliseconds);
		effect.ramp.fade_level = cf_s_f32_to_u16(data.u.ramp.envelope.fade);
		effect.ramp.fade_length = cf_s_saturate16(data.u.ramp.envelope.fade_milliseconds);
	}
	return effect;
}

cf_haptic_effect_t cf_haptic_create_effect(cf_haptic_t* haptic, cf_haptic_data_t data)
{
	SDL_HapticEffect effect = cf_s_cute_to_sdl(data);
	int id = SDL_HapticNewEffect(haptic->ptr, &effect);
	cf_haptic_effect_t result = { id };
	return result;
}

void cf_haptic_run_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect, int iterations)
{
	SDL_HapticRunEffect(haptic->ptr, effect.id, cf_s_saturate32(iterations));
}

void cf_haptic_update_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect, cf_haptic_data_t data)
{
	SDL_HapticEffect update = cf_s_cute_to_sdl(data);
	SDL_HapticUpdateEffect(haptic->ptr, effect.id, &update);
}

void cf_haptic_stop_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect)
{
	SDL_HapticStopEffect(haptic->ptr, effect.id);
}

void cf_haptic_destroy_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect)
{
	SDL_HapticDestroyEffect(haptic->ptr, effect.id);
}

void cf_haptic_pause(cf_haptic_t* haptic)
{
	SDL_HapticPause(haptic->ptr);
}

void cf_haptic_unpause(cf_haptic_t* haptic)
{
	SDL_HapticUnpause(haptic->ptr);
}

void cf_haptic_stop_all(cf_haptic_t* haptic)
{
	SDL_HapticStopAll(haptic->ptr);
}

bool cf_haptic_rumble_supported(cf_haptic_t* haptic)
{
	int result = SDL_HapticRumbleSupported(haptic->ptr);
	return result == SDL_TRUE ? true : false;
}

void cf_haptic_rumble_play(cf_haptic_t* haptic, float strength, int duration_milliseconds)
{
	if (!haptic->rumble_initialized) {
		SDL_HapticRumbleInit(haptic->ptr);
		haptic->rumble_initialized = true;
	}
	SDL_HapticRumblePlay(haptic->ptr, strength, cf_s_saturate32(duration_milliseconds));
}

void cf_haptic_rumble_stop(cf_haptic_t* haptic)
{
	SDL_HapticRumbleStop(haptic->ptr);
}
