/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_haptics.h>
#include <cute_alloc.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_input_internal.h>

#include <SDL.h>

struct CF_HapticInstance
{
	SDL_Haptic* ptr = NULL;
	bool rumble_initialized = false;
};

CF_Haptic cf_haptic_open(CF_Joypad joypad_handle)
{
	CF_JoypadInstance* joypad = (CF_JoypadInstance*)joypad_handle.id;
	CF_HapticInstance* haptic = CF_NEW(CF_HapticInstance);
	CF_Haptic handle = { 0 };
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(joypad->controller);
	if (!joy) {
		CF_FREE(haptic);
		return handle;
	}
	haptic->ptr = SDL_HapticOpenFromJoystick(joy);
	if (!haptic->ptr) {
		CF_FREE(haptic);
		return handle;
	}
	handle.id = (uint64_t)haptic;
	joypad->haptic = handle;
	return handle;
}

void cf_haptic_close(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticClose(haptic->ptr);
	CF_FREE(haptic);
}

bool cf_haptic_supports(CF_Haptic haptic_handle, CF_HapticType type)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
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

void cf_haptic_set_gain(CF_Haptic haptic_handle, float gain)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
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

static SDL_HapticEffect s_cute_to_sdl(CF_HapticData data)
{
	SDL_HapticEffect effect = { 0 };
	if (data.type == CF_HAPTIC_TYPE_LEFTRIGHT) {
		effect.type = SDL_HAPTIC_LEFTRIGHT;
		effect.leftright.length = s_saturate32(data.u.leftright.duration_milliseconds);
		effect.leftright.small_magnitude = s_f32_to_u16(data.u.leftright.lo_motor_strength);
		effect.leftright.large_magnitude = s_f32_to_u16(data.u.leftright.hi_motor_strength);
	} else if (data.type == CF_HAPTIC_TYPE_PERIODIC) {
		if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_SINE) {
			effect.type = SDL_HAPTIC_SINE;
		} else if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_TRIANGLE) {
			effect.type = SDL_HAPTIC_TRIANGLE;
		} else if (data.u.periodic.wave_type == CF_HAPTIC_WAVE_TYPE_SAW) {
			effect.type = SDL_HAPTIC_SAWTOOTHUP;
		}
		effect.periodic.length = s_saturate32(data.u.periodic.duration_milliseconds);
		effect.periodic.period = s_saturate16(data.u.periodic.period_milliseconds);
		effect.periodic.magnitude = s_f32_to_s16(data.u.periodic.magnitude);
		effect.periodic.attack_level = s_f32_to_u16(data.u.periodic.envelope.attack);
		effect.periodic.attack_length = s_saturate16(data.u.periodic.envelope.attack_milliseconds);
		effect.periodic.fade_level = s_f32_to_u16(data.u.periodic.envelope.fade);
		effect.periodic.fade_length = s_saturate16(data.u.periodic.envelope.fade_milliseconds);
	} else if (data.type == CF_HAPTIC_TYPE_RAMP) {
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

CF_HapticEffect cf_haptic_create_effect(CF_Haptic haptic_handle, CF_HapticData data)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticEffect effect = s_cute_to_sdl(data);
	int id = SDL_HapticNewEffect(haptic->ptr, &effect);
	CF_HapticEffect result = { id };
	return result;
}

void cf_haptic_run_effect(CF_Haptic haptic_handle, CF_HapticEffect effect, int iterations)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticRunEffect(haptic->ptr, effect.id, s_saturate32(iterations));
}

void cf_haptic_update_effect(CF_Haptic haptic_handle, CF_HapticEffect effect, CF_HapticData data)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticEffect update = s_cute_to_sdl(data);
	SDL_HapticUpdateEffect(haptic->ptr, effect.id, &update);
}

void cf_haptic_stop_effect(CF_Haptic haptic_handle, CF_HapticEffect effect)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticStopEffect(haptic->ptr, effect.id);
}

void cf_haptic_destroy_effect(CF_Haptic haptic_handle, CF_HapticEffect effect)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticDestroyEffect(haptic->ptr, effect.id);
}

void cf_haptic_pause(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticPause(haptic->ptr);
}

void cf_haptic_unpause(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticUnpause(haptic->ptr);
}

void cf_haptic_stop_all(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticStopAll(haptic->ptr);
}

bool cf_haptic_rumble_supported(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	int result = SDL_HapticRumbleSupported(haptic->ptr);
	return result == SDL_TRUE ? true : false;
}

void cf_haptic_rumble_play(CF_Haptic haptic_handle, float strength, int duration_milliseconds)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	if (!haptic->rumble_initialized) {
		SDL_HapticRumbleInit(haptic->ptr);
		haptic->rumble_initialized = true;
	}
	SDL_HapticRumblePlay(haptic->ptr, strength, s_saturate32(duration_milliseconds));
}

void cf_haptic_rumble_stop(CF_Haptic haptic_handle)
{
	CF_HapticInstance* haptic = (CF_HapticInstance*)haptic_handle.id;
	SDL_HapticRumbleStop(haptic->ptr);
}
