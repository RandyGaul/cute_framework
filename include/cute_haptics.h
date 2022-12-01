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

#ifndef CUTE_HAPTICS_H
#define CUTE_HAPTICS_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Haptics is for rumbling or vibrating devices or controllers.
 *
 * A haptic pointer can be opened by calling `haptic_open` on a joypad. See cute_joypad.h for
 * info on joypads.
 *
 * Haptics can be a little complicated -- if you just want a simple rumble effect then the
 * rumble functions might be a good option for you.
 * - `haptic_rumble_supported`
 * - `haptic_rumble_play`
 * - `haptic_rumble_stop`
 *
 * TODO - Open haptic on the device itself (e.g. for phones).
 */
typedef struct CF_Haptic CF_Haptic;

/**
 * The envelope defines a fade in and fade out of strength for a haptic effect. The strength
 * starts at zero, and linearly fades up to `attack` over `attack_milliseconds`. The strength
 * then linearly interpolates down to `fade` over the duration of the haptic effect. Finally, the
 * strength diminishes from `fade` to zero over `fade milliseconds`.
 *
 * `attack` and `fade` should be values from 0 to 1, where 0 means do nothing (no strength) and
 * 1 means run the haptic motors as hard as possible (max strength).
 *
 * Here's an example.
 *
 *          ^
 *          1      (2)--.
 *          |      /     `--.
 * strength |     /          `--(3)
 *          |    /                \
 *          0  (1)                (4)
 *          +-------------------------->
 *                     time
 *
 * From (1) to (2) is the `attack_milliseconds`.
 * From (1) to (4) is the effect duration (search for `duration_milliseconds` in this file).
 * From (3) to (4) is the `fade_milliseconds`.
 */
typedef struct CF_HapticEnvelope
{
	float attack;
	int attack_milliseconds;
	float fade;
	int fade_milliseconds;
} CF_HapticEnvelope;

#define CF_HAPTIC_TYPE_DEFS \
	CF_ENUM(HAPTIC_TYPE_INVALID,   0) \
	CF_ENUM(HAPTIC_TYPE_LEFTRIGHT, 1) \
	CF_ENUM(HAPTIC_TYPE_PERIODIC,  2) \
	CF_ENUM(HAPTIC_TYPE_RAMP,      3) \

typedef enum CF_HapticType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
} CF_HapticType;

CUTE_INLINE const char* cf_haptic_type_to_string(CF_HapticType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * The leftright haptic allows direct control of one larger and one smaller freqeuncy motors,
 * as commonly found in game controllers.
 *
 * `lo_motor_strength` must be from 0 to 1.
 * `hi_motor_strength` must be from 0 to 1.
 */
typedef struct CF_HapticLeftRight
{
	int duration_milliseconds;
	float lo_motor_strength;
	float hi_motor_strength;
} CF_HapticLeftRight;

#define CF_HAPTIC_WAVE_TYPE_DEFS \
	CF_ENUM(HAPTIC_WAVE_TYPE_SINE,     0) \
	CF_ENUM(HAPTIC_WAVE_TYPE_TRIANGLE, 1) \
	CF_ENUM(HAPTIC_WAVE_TYPE_SAW,      2) \

typedef enum CF_HapticWaveType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
} CF_HapticWaveType;

CUTE_INLINE const char* cf_haptic_wave_type_to_string(CF_HapticWaveType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * A basic haptic for sine-based waveforms (https://en.wikipedia.org/wiki/Sine_wave).
 *
 * `period` (time between each wave, or 1 / frequency).
 * `magnitude` (aka amplitude) must be from 0 to 1.
 */
typedef struct CF_HapticPeriodic
{
	CF_HapticWaveType wave_type;
	int duration_milliseconds;
	int period_milliseconds;
	float magnitude;
	/* phase - not included (not very useful) */
	CF_HapticEnvelope envelope;
} CF_HapticPeriodic;

/**
 * Adjusts the strength from `start` to `end` over `duration_milliseconds`.
 *
 * `start` must be from 0 to 1.
 * `end` must be from 0 to 1.
 */
typedef struct CF_HapticRamp
{
	int duration_milliseconds;
	float start;
	float end;
	CF_HapticEnvelope envelope;
} CF_HapticRamp;

typedef struct CF_HapticData
{
	CF_HapticType type;
	union
	{
		CF_HapticLeftRight leftright;
		CF_HapticPeriodic periodic;
		CF_HapticRamp ramp;
	} u;
} CF_HapticData;

// -------------------------------------------------------------------------------------------------

typedef struct CF_Joypad CF_Joypad;

/**
 * Attempts to open a joypad for haptics use.
 * Returns `NULL` upon any errors, including missing support from the underlying device.
 */
CUTE_API CF_Haptic* CUTE_CALL cf_haptic_open(CF_Joypad* joypad);
CUTE_API void CUTE_CALL cf_haptic_close(CF_Haptic* haptic);

/**
 * Checks to see if a certain type of haptic is supported on this device.
 * Returns true if supported, false otherwise.
 */
CUTE_API bool CUTE_CALL cf_haptic_supports(CF_Haptic* haptic, CF_HapticType type);

/**
 * Sets the global gain for all haptics on this device. This is like a global "volume" for
 * the strength of all haptics that run on this device.
 * `gain` must be from 0 to 1.
 */
CUTE_API void CUTE_CALL cf_haptic_set_gain(CF_Haptic* haptic, float gain);

/**
 * Represents a single instance of a `HapticData` on a device.
 */
typedef struct CF_HapticEffect
{
	int id;

	#ifdef CUTE_CPP
	CUTE_INLINE bool is_valid();
	#endif // CUTE_CPP

} CF_HapticEffect;

CUTE_API CUTE_INLINE bool CUTE_CALL cf_haptic_effect_is_valid( const CF_HapticEffect* effect) { return effect->id > 0 ? true : false; }

/**
 * Creates a single effect instance on the device. It can be run with `haptic_run_effect`.
 */
CUTE_API CF_HapticEffect CUTE_CALL cf_haptic_create_effect(CF_Haptic* haptic, CF_HapticData data);

/**
 * Starts playing the specified effect a number of times.
 */
CUTE_API void CUTE_CALL cf_haptic_run_effect(CF_Haptic* haptic, CF_HapticEffect effect, int iterations);

/**
 * Dynamically updates an effect on the device. This can *not* change the effect type.
 */
CUTE_API void CUTE_CALL cf_haptic_update_effect(CF_Haptic* haptic, CF_HapticEffect effect, CF_HapticData data);

/**
 * Stops playing the specified effect. The effect is not destroyed.
 */
CUTE_API void CUTE_CALL cf_haptic_stop_effect(CF_Haptic* haptic, CF_HapticEffect effect);

/**
 * Destroys an instance of an effect on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_destroy_effect(CF_Haptic* haptic, CF_HapticEffect effect);

/**
 * Pause all haptics on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_pause(CF_Haptic* haptic);

/**
 * Unpause all haptics on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_unpause(CF_Haptic* haptic);

/**
 * Stops all haptics on the device. The effects are not destroyed.
 */
CUTE_API void CUTE_CALL cf_haptic_stop_all(CF_Haptic* haptic);

// -------------------------------------------------------------------------------------------------

/**
 * Checks to see if a simple sine/leftright haptic can be supported on the device. Creates
 * a set of decent parameters to play a rumbling effect in an easy to use way.
 */
CUTE_API bool CUTE_CALL cf_haptic_rumble_supported(CF_Haptic* haptic);

/**
 * Starts playing a simple rumble effect with `strength` (value from 0 to 1) for `duration_milliseconds`.
 * Successive calls on this function will update the underlying rumble effect, instead of creating
 * and playing multiple different effect instances.
 */
CUTE_API void CUTE_CALL cf_haptic_rumble_play(CF_Haptic* haptic, float strength, int duration_milliseconds);

/**
 * Stops playing the simple rumble effect.
 */
CUTE_API void CUTE_CALL cf_haptic_rumble_stop(CF_Haptic* haptic);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

CUTE_INLINE bool CF_HapticEffect::is_valid() { return cf_haptic_effect_is_valid(this); }

namespace cute
{

using Haptic = CF_Haptic;
using HapticEnvelope = CF_HapticEnvelope;
using HapticLeftRight = CF_HapticLeftRight;
using HapticPeriodic = CF_HapticPeriodic;
using HapticRamp = CF_HapticRamp;
using HapticData = CF_HapticData;
using Joypad = CF_Joypad;
using HapticEffect = CF_HapticEffect;

using HapticType = CF_HapticType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr HapticType K = CF_##K;
CF_HAPTIC_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(HapticType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using HapticWaveType = CF_HapticWaveType;
#define CF_ENUM(K, V) CUTE_INLINE constexpr HapticWaveType K = CF_##K;
CF_HAPTIC_WAVE_TYPE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(HapticWaveType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CUTE_INLINE Haptic* haptic_open(Joypad* joypad) { return cf_haptic_open(joypad); }
CUTE_INLINE void haptic_close(Haptic* haptic) { cf_haptic_close(haptic); }
CUTE_INLINE bool haptic_supports(Haptic* haptic, HapticType type) { return cf_haptic_supports(haptic,type); }
CUTE_INLINE void haptic_set_gain(Haptic* haptic, float gain) { cf_haptic_set_gain(haptic,gain); }
CUTE_INLINE bool haptic_effect_is_valid(const HapticEffect* effect) { return cf_haptic_effect_is_valid(effect); }
CUTE_INLINE HapticEffect haptic_create_effect(Haptic* haptic, HapticData data) { return cf_haptic_create_effect(haptic,data); }
CUTE_INLINE void haptic_run_effect(Haptic* haptic, HapticEffect effect, int iterations) { cf_haptic_run_effect(haptic,effect,iterations); }
CUTE_INLINE void haptic_update_effect(Haptic* haptic, HapticEffect effect, HapticData data) { cf_haptic_update_effect(haptic,effect,data); }
CUTE_INLINE void haptic_stop_effect(Haptic* haptic, HapticEffect effect) { cf_haptic_stop_effect(haptic,effect); }
CUTE_INLINE void haptic_destroy_effect(Haptic* haptic, HapticEffect effect) { cf_haptic_destroy_effect(haptic,effect); }
CUTE_INLINE void haptic_pause(Haptic* haptic) { cf_haptic_pause(haptic); }
CUTE_INLINE void haptic_unpause(Haptic* haptic) { cf_haptic_unpause(haptic); }
CUTE_INLINE void haptic_stop_all(Haptic* haptic) { cf_haptic_stop_all(haptic); }
CUTE_INLINE bool haptic_rumble_supported(Haptic* haptic) { return cf_haptic_rumble_supported(haptic); }
CUTE_INLINE void haptic_rumble_play(Haptic* haptic, float strength, int duration_milliseconds) { cf_haptic_rumble_play(haptic,strength,duration_milliseconds); }
CUTE_INLINE void haptic_rumble_stop(Haptic* haptic) { cf_haptic_rumble_stop(haptic); }

}

#endif // CUTE_CPP

#endif // CUTE_HAPTICS_H
