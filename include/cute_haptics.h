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
typedef struct cf_haptic_t cf_haptic_t;

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
typedef struct cf_haptic_envelope_t
{
	float attack;
	int attack_milliseconds;
	float fade;
	int fade_milliseconds;
} cf_haptic_envelope_t;

typedef enum cf_haptic_type_t
{
	CF_HAPTIC_TYPE_INVALID,
	CF_HAPTIC_TYPE_LEFTRIGHT,
	CF_HAPTIC_TYPE_PERIODIC,
	CF_HAPTIC_TYPE_RAMP,
} cf_haptic_type_t;

/**
 * The leftright haptic allows direct control of one larger and one smaller freqeuncy motors,
 * as commonly found in game controllers.
 *
 * `lo_motor_strength` must be from 0 to 1.
 * `hi_motor_strength` must be from 0 to 1.
 */
typedef struct cf_haptic_leftright_t
{
	int duration_milliseconds;
	float lo_motor_strength;
	float hi_motor_strength;
} cf_haptic_leftright_t;

typedef enum cf_haptic_wave_type_t
{
	CF_HAPTIC_WAVE_TYPE_SINE,
	CF_HAPTIC_WAVE_TYPE_TRIANGLE,
	CF_HAPTIC_WAVE_TYPE_SAW,
} cf_haptic_wave_type_t;

/**
 * A basic haptic for sine-based waveforms (https://en.wikipedia.org/wiki/Sine_wave).
 *
 * `period` (time between each wave, or 1 / frequency).
 * `magnitude` (aka amplitude) must be from 0 to 1.
 */
typedef struct cf_haptic_periodic_t
{
	cf_haptic_wave_type_t wave_type;
	int duration_milliseconds;
	int period_milliseconds;
	float magnitude;
	/* phase - not included (not very useful) */
	cf_haptic_envelope_t envelope;
} cf_haptic_periodic_t;

/**
 * Adjusts the strength from `start` to `end` over `duration_milliseconds`.
 *
 * `start` must be from 0 to 1.
 * `end` must be from 0 to 1.
 */
typedef struct cf_haptic_ramp_t
{
	int duration_milliseconds;
	float start;
	float end;
	cf_haptic_envelope_t envelope;
} cf_haptic_ramp_t;

typedef struct cf_haptic_data_t
{
	cf_haptic_type_t type;
	union
	{
		cf_haptic_leftright_t leftright;
		cf_haptic_periodic_t periodic;
		cf_haptic_ramp_t ramp;
	} u;
} cf_haptic_data_t;

// -------------------------------------------------------------------------------------------------

typedef struct cf_joypad_t cf_joypad_t;

/**
 * Attempts to open a joypad for haptics use.
 * Returns `NULL` upon any errors, including missing support from the underlying device.
 */
CUTE_API cf_haptic_t* CUTE_CALL cf_haptic_open(cf_joypad_t* joypad);
CUTE_API void CUTE_CALL cf_haptic_close(cf_haptic_t* haptic);

/**
 * Checks to see if a certain type of haptic is supported on this device.
 * Returns true if supported, false otherwise.
 */
CUTE_API bool CUTE_CALL cf_haptic_supports(cf_haptic_t* haptic, cf_haptic_type_t type);

/**
 * Sets the global gain for all haptics on this device. This is like a global "volume" for
 * the strength of all haptics that run on this device.
 * `gain` must be from 0 to 1.
 */
CUTE_API void CUTE_CALL cf_haptic_set_gain(cf_haptic_t* haptic, float gain);

/**
 * Represents a single instance of a `haptic_data_t` on a device.
 */
typedef struct cf_haptic_effect_t
{
	int id;

	#ifdef CUTE_CPP
	CUTE_INLINE bool is_valid();
	#endif // CUTE_CPP

} cf_haptic_effect_t;

CUTE_API CUTE_INLINE bool CUTE_CALL cf_haptic_effect_is_valid( const cf_haptic_effect_t* effect) { return effect->id > 0 ? true : false; }

/**
 * Creates a single effect instance on the device. It can be run with `haptic_run_effect`.
 */
CUTE_API cf_haptic_effect_t CUTE_CALL cf_haptic_create_effect(cf_haptic_t* haptic, cf_haptic_data_t data);

/**
 * Starts playing the specified effect a number of times.
 */
CUTE_API void CUTE_CALL cf_haptic_run_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect, int iterations);

/**
 * Dynamically updates an effect on the device. This can *not* change the effect type.
 */
CUTE_API void CUTE_CALL cf_haptic_update_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect, cf_haptic_data_t data);

/**
 * Stops playing the specified effect. The effect is not destroyed.
 */
CUTE_API void CUTE_CALL cf_haptic_stop_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect);

/**
 * Destroys an instance of an effect on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_destroy_effect(cf_haptic_t* haptic, cf_haptic_effect_t effect);

/**
 * Pause all haptics on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_pause(cf_haptic_t* haptic);

/**
 * Unpause all haptics on the device.
 */
CUTE_API void CUTE_CALL cf_haptic_unpause(cf_haptic_t* haptic);

/**
 * Stops all haptics on the device. The effects are not destroyed.
 */
CUTE_API void CUTE_CALL cf_haptic_stop_all(cf_haptic_t* haptic);

// -------------------------------------------------------------------------------------------------

/**
 * Checks to see if a simple sine/leftright haptic can be supported on the device. Creates
 * a set of decent parameters to play a rumbling effect in an easy to use way.
 */
CUTE_API bool CUTE_CALL cf_haptic_rumble_supported(cf_haptic_t* haptic);

/**
 * Starts playing a simple rumble effect with `strength` (value from 0 to 1) for `duration_milliseconds`.
 * Successive calls on this function will update the underlying rumble effect, instead of creating
 * and playing multiple different effect instances.
 */
CUTE_API void CUTE_CALL cf_haptic_rumble_play(cf_haptic_t* haptic, float strength, int duration_milliseconds);

/**
 * Stops playing the simple rumble effect.
 */
CUTE_API void CUTE_CALL cf_haptic_rumble_stop(cf_haptic_t* haptic);


#ifdef CUTE_CPP

CUTE_INLINE bool cf_haptic_effect_t::is_valid() { return cf_haptic_effect_is_valid(this); }

namespace cute
{

using haptic_t = cf_haptic_t;
using haptic_envelope_t = cf_haptic_envelope_t;
using haptic_type_t = cf_haptic_type_t;
using haptic_leftright_t = cf_haptic_leftright_t;
using haptic_wave_type_t = cf_haptic_wave_type_t;
using cf_haptic_periodic_t = cf_haptic_periodic_t;
using haptic_ramp_t = cf_haptic_ramp_t;
using haptic_data_t = cf_haptic_data_t;
using joypad_t = cf_joypad_t;
using haptic_effect_t = cf_haptic_effect_t;

CUTE_INLINE haptic_t* haptic_open(joypad_t* joypad) { return cf_haptic_open(joypad); }
CUTE_INLINE void haptic_close(haptic_t* haptic) { cf_haptic_close(haptic); }
CUTE_INLINE bool haptic_supports(haptic_t* haptic, haptic_type_t type) { return cf_haptic_supports(haptic,type); }
CUTE_INLINE void haptic_set_gain(haptic_t* haptic, float gain) { cf_haptic_set_gain(haptic,gain); }
CUTE_INLINE bool haptic_effect_is_valid(const haptic_effect_t* effect) { return cf_haptic_effect_is_valid(effect); }
CUTE_INLINE haptic_effect_t haptic_create_effect(haptic_t* haptic, haptic_data_t data) { return cf_haptic_create_effect(haptic,data); }
CUTE_INLINE void haptic_run_effect(haptic_t* haptic, haptic_effect_t effect, int iterations) { cf_haptic_run_effect(haptic,effect,iterations); }
CUTE_INLINE void haptic_update_effect(haptic_t* haptic, haptic_effect_t effect, haptic_data_t data) { cf_haptic_update_effect(haptic,effect,data); }
CUTE_INLINE void haptic_stop_effect(haptic_t* haptic, haptic_effect_t effect) { cf_haptic_stop_effect(haptic,effect); }
CUTE_INLINE void haptic_destroy_effect(haptic_t* haptic, haptic_effect_t effect) { cf_haptic_destroy_effect(haptic,effect); }
CUTE_INLINE void haptic_pause(haptic_t* haptic) { cf_haptic_pause(haptic); }
CUTE_INLINE void haptic_unpause(haptic_t* haptic) { cf_haptic_unpause(haptic); }
CUTE_INLINE void haptic_stop_all(haptic_t* haptic) { cf_haptic_stop_all(haptic); }
CUTE_INLINE bool haptic_rumble_supported(haptic_t* haptic) { cf_haptic_rumble_supported(haptic); }
CUTE_INLINE void haptic_rumble_play(haptic_t* haptic, float strength, int duration_milliseconds) { cf_haptic_rumble_play(haptic,strength,duration_milliseconds); }
CUTE_INLINE void haptic_rumble_stop(haptic_t* haptic) { cf_haptic_rumble_stop(haptic); }

}

#endif // CUTE_CPP



#endif // CUTE_HAPTICS_H
