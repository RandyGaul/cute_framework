/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_HAPTICS_H
#define CF_HAPTICS_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Haptic
 * @category haptic
 * @brief    An opaque pointer representing a haptic.
 * @remarks  Haptics is for rumbling or vibrating devices or controllers.
 *           
 *           A haptic pointer can be opened by calling `haptic_open` on a joypad. See cute_joypad.h for info on joypads.
 *           
 *           Haptics can be a little complicated -- if you just want a simple rumble effect then the rumble functions might be a good option for you.
 *           - `cf_haptic_rumble_supported`
 *           - `cf_haptic_rumble_play`
 *           - `cf_haptic_rumble_stop`
 *           
 *           TODO - Open haptic on the device itself (e.g. for phones).
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect
 */
typedef struct CF_Haptic CF_Haptic;
// @end

/**
 * @struct   CF_HapticEnvelope
 * @category haptic
 * @brief    Defines a fade in and fade out of strength for a `CF_HapticEffect`.
 * @remarks  The envelope defines a fade in and fade out of strength for a haptic effect. The strength
 *           starts at zero, and linearly fades up to `attack` over `attack_milliseconds`. The strength
 *           then linearly interpolates down to `fade` over the duration of the haptic effect. Finally, the
 *           strength diminishes from `fade` to zero over `fade milliseconds`.
 *           
 *           `attack` and `fade` should be values from 0 to 1, where 0 means do nothing (no strength) and
 *           1 means run the haptic motors as hard as possible (max strength).
 *           
 *           Here's an example.
 *           
 *           ```
 *                    ^
 *                    1      (2)--.
 *                    |      /     `--.
 *           strength |     /          `--(3)
 *                    |    /                \
 *                    0  (1)                (4)
 *                    +-------------------------->
 *                               time
 *           ```
 *           
 *           From (1) to (2) is the `attack_milliseconds`.
 *           From (1) to (4) is the effect duration (search for `duration_milliseconds` in this file).
 *           From (3) to (4) is the `fade_milliseconds`.
 * @related  CF_Haptic CF_HapticEnvelope CF_HapticEffect CF_HapticData cf_haptic_create_effect
 */
typedef struct CF_HapticEnvelope
{
	/* @member A value from 0.0f to 1.0f. Represents the initial rise in the envelope curve. */
	float attack;

	/* @member The duration of `attack` in milliseconds. */
	int attack_milliseconds;

	/* @member A value from 0.0f to 1.0f. Represents the fall in the envelope curve. */
	float fade;

	/* @member The duration for `fade` to back down to zero at the end of the envelope, in milliseconds. */
	int fade_milliseconds;
} CF_HapticEnvelope;
// @end

/**
 * @enum     CF_HapticType
 * @category haptic
 * @brief    Various types of supported haptic effects.
 * @related  cf_haptic_type_to_string CF_HapticData CF_HapticLeftRight CF_HapticPeriodic CF_HapticRamp
 */
#define CF_HAPTIC_TYPE_DEFS \
	/* @entry An invalid haptic type. */           \
	CF_ENUM(HAPTIC_TYPE_INVALID,   0)              \
	/* @entry Left to right rumble haptic. */      \
	CF_ENUM(HAPTIC_TYPE_LEFTRIGHT, 1)              \
	/* @entry Periodic wave-form rumble haptic. */ \
	CF_ENUM(HAPTIC_TYPE_PERIODIC,  2)              \
	/* @entry A ramp over time rumble haptic. */   \
	CF_ENUM(HAPTIC_TYPE_RAMP,      3)              \
	/* @end */

typedef enum CF_HapticType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
} CF_HapticType;

/**
 * @function cf_haptic_type_to_string
 * @category haptic
 * @brief    Converts a `CF_HapticType` to a C string.
 * @param    type       The type to convert.
 * @related  CF_HapticType cf_haptic_type_to_string
 */
CF_INLINE const char* cf_haptic_type_to_string(CF_HapticType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_HapticLeftRight
 * @category haptic
 * @brief    The leftright haptic allows direct control of one larger and one smaller freqeuncy motors, as commonly found in game controllers.
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect cf_haptic_create_effect
 */
typedef struct CF_HapticLeftRight
{
	/* @member The delay between `attack` and `fade` in the envelope (see `CF_HapticEnvelope` for more details). */
	int duration_milliseconds;

	/* @member From 0.0f to 1.0f. */
	float lo_motor_strength;

	/* @member From 0.0f to 1.0f. */
	float hi_motor_strength;
} CF_HapticLeftRight;
// @end

/**
 * @enum     CF_HapticWaveType
 * @category haptic
 * @brief    Various types of supported haptic waves.
 * @related  CF_HapticWaveType cf_haptic_wave_type_to_string CF_HapticPeriodic CF_HapticEffect cf_haptic_create_effect
 */
#define CF_HAPTIC_WAVE_TYPE_DEFS \
	/* @entry Sin wave haptic type. */         \
	CF_ENUM(HAPTIC_WAVE_TYPE_SINE,     0)      \
	/* @entry Triangle wave haptic type. */    \
	CF_ENUM(HAPTIC_WAVE_TYPE_TRIANGLE, 1)      \
	/* @entry Saw pattern wave haptic type. */ \
	CF_ENUM(HAPTIC_WAVE_TYPE_SAW,      2)      \
	/* @end */

typedef enum CF_HapticWaveType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
} CF_HapticWaveType;

/**
 * @function cf_haptic_wave_type_to_string
 * @category haptic
 * @brief    Converts a `CF_HapticWaveType` to a C string.
 * @param    type       The string to convert.
 * @related  CF_HapticWaveType cf_haptic_wave_type_to_string
 */
CF_INLINE const char* cf_haptic_wave_type_to_string(CF_HapticWaveType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_HapticPeriodic
 * @category haptic
 * @brief    A basic haptic for sine-based waveforms (https://en.wikipedia.org/wiki/Sine_wave).
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect cf_haptic_create_effect
 */
typedef struct CF_HapticPeriodic
{
	/* @member The delay between `attack` and `fade` in the envelope (see `CF_HapticEnvelope` for more details). */
	CF_HapticWaveType wave_type;

	/* @member Time between each wave in milliseconds, or 1.0f/frequency. See `CF_HapticEnvelope` for details. */
	int duration_milliseconds;

	/* @member The period of the sin wave. Must be from 0.0f to 1.0f. */
	int period_milliseconds;

	/* @member The strength/amplitude of the sin wave. */
	float magnitude;

	/* @member The envelope for the haptic. See `CF_HapticEnvelope` for details. */
	CF_HapticEnvelope envelope;
} CF_HapticPeriodic;
// @end

/**
 * @struct   CF_HapticRamp
 * @category haptic
 * @brief    Adjusts the strength from `start` to `end` over `duration_milliseconds`.
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect cf_haptic_create_effect
 */
typedef struct CF_HapticRamp
{
	/* @member The delay between `attack` and `fade` in the envelope (see `CF_HapticEnvelope` for more details). */
	int duration_milliseconds;

	/* @member Strength value. Must be from 0.0f to 1.0f. */
	float start;

	/* @member Strength value. Must be from 0.0f to 1.0f. */
	float end;

	/* @member The envelope for the haptic. See `CF_HapticEnvelope` for details. */
	CF_HapticEnvelope envelope;
} CF_HapticRamp;
// @end

/**
 * @struct   CF_HapticData
 * @category haptic
 * @brief    Container struct for all possible supported haptic types.
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect cf_haptic_create_effect CF_HapticRamp CF_HapticPeriodic CF_HapticLeftRight
 */
typedef struct CF_HapticData
{
	/* @member The type of the haptic, see `CF_HapticType`. */
	CF_HapticType type;
	union
	{
		/* @member A leftright haptic if `type` is `CF_HAPTIC_TYPE_LEFTRIGHT`, see `CF_HapticLeftRight`. */
		CF_HapticLeftRight leftright;

		/* @member A periodic haptic if `type` is `CF_HAPTIC_TYPE_PERIODIC`, see `CF_HapticLeftRight`. */
		CF_HapticPeriodic periodic;

		/* @member A ramp haptic if `type` is `CF_HAPTIC_TYPE_RAMP`, see `CF_HapticLeftRight`. */
		CF_HapticRamp ramp;
	} u;
} CF_HapticData;
// @end

// -------------------------------------------------------------------------------------------------

typedef struct CF_Joypad CF_Joypad;

/**
 * @function cf_haptic_open
 * @category haptic
 * @brief    Attempts to open a joypad for haptics use.
 * @param    joypad         A joypad (see `CF_Joypad`).
 * @return   Returns a new `CF_Haptic`.
 * @remarks  Returns `NULL` upon any errors, including missing support from the underlying device.
 * @related  CF_Haptic CF_Joypad cf_haptic_open cf_haptic_close cf_haptic_create_effect cf_haptic_run_effect cf_haptic_rumble_play
 */
CF_API CF_Haptic* CF_CALL cf_haptic_open(CF_Joypad* joypad);

/**
 * @function cf_haptic_close
 * @category haptic
 * @brief    Frees up a `CF_Haptic` previously created by `cf_haptic_open`.
 * @param    haptic         The haptic.
 * @related  CF_Haptic cf_haptic_open cf_haptic_close cf_haptic_create_effect cf_haptic_run_effect cf_haptic_rumble_play
 */
CF_API void CF_CALL cf_haptic_close(CF_Haptic* haptic);

/**
 * @function cf_haptic_supports
 * @category haptic
 * @brief    Checks to see if a certain type of haptic is supported on this device.
 * @param    haptic         The haptic.
 * @param    type           The `CF_HapticType` to check support for.
 * @return   Returns true if supported, false otherwise.
 * @related  CF_Haptic cf_haptic_open cf_haptic_close cf_haptic_create_effect cf_haptic_run_effect
 */
CF_API bool CF_CALL cf_haptic_supports(CF_Haptic* haptic, CF_HapticType type);

/**
 * @function cf_haptic_set_gain
 * @category haptic
 * @brief    Sets the global gain for all haptics on this device.
 * @param    haptic         The haptic.
 * @param    gain           Must be from 0 to 1. This is like a global "volume" for the strength of all haptics that run on this device.
 * @related  CF_Haptic cf_haptic_open cf_haptic_close cf_haptic_create_effect cf_haptic_run_effect
 */
CF_API void CF_CALL cf_haptic_set_gain(CF_Haptic* haptic, float gain);

/**
 * @struct   CF_HapticEffect
 * @category haptic
 * @brief    An opaque handle representing a single instance of a `CF_HapticData` on a device.
 * @related  CF_Haptic CF_HapticType cf_haptic_open cf_haptic_close CF_HapticEffect cf_haptic_create_effect CF_HapticRamp CF_HapticPeriodic CF_HapticLeftRight
 */
typedef struct CF_HapticEffect { int id; } CF_HapticEffect;
// @end

/**
 * @function cf_haptic_create_effect
 * @category haptic
 * @brief    Creates a single effect instance on the device.
 * @param    haptic         The haptic.
 * @param    data           The haptic specification.
 * @remarks  Run the haptic with `haptic_run_effect`.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_run_effect
 */
CF_API CF_HapticEffect CF_CALL cf_haptic_create_effect(CF_Haptic* haptic, CF_HapticData data);

/**
 * @function cf_haptic_run_effect
 * @category haptic
 * @brief    Starts playing the specified effect a number of times.
 * @param    haptic         The haptic.
 * @param    effect         The haptic effect created by `cf_haptic_create_effect`.
 * @param    iterations     A number of times to play the effect.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_run_effect cf_haptic_stop_effect
 */
CF_API void CF_CALL cf_haptic_run_effect(CF_Haptic* haptic, CF_HapticEffect effect, int iterations);

/**
 * @function cf_haptic_update_effect
 * @category haptic
 * @brief    Dynamically updates an effect on the device. This _can not_ change the effect type.
 * @param    haptic         The haptic.
 * @param    effect         The haptic effect created by `cf_haptic_create_effect`.
 * @param    data           The updated haptic specification.
 * @related  CF_Haptic cf_haptic_open CF_HapticData CF_HapticEffect cf_haptic_create_effect cf_haptic_run_effect cf_haptic_stop_effect
 */
CF_API void CF_CALL cf_haptic_update_effect(CF_Haptic* haptic, CF_HapticEffect effect, CF_HapticData data);

/**
 * @function cf_haptic_stop_effect
 * @category haptic
 * @brief    Stops playing the specified effect.
 * @param    haptic         The haptic.
 * @param    effect         The haptic effect created by `cf_haptic_create_effect`.
 * @remarks  The effect is not destroyed.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_stop_effect cf_haptic_destroy_effect cf_haptic_pause cf_haptic_unpause cf_haptic_stop_all
 */
CF_API void CF_CALL cf_haptic_stop_effect(CF_Haptic* haptic, CF_HapticEffect effect);

/**
 * @function cf_haptic_destroy_effect
 * @category haptic
 * @brief    Destroys an instance of an effect on the device.
 * @param    haptic         The haptic.
 * @param    effect         The haptic effect created by `cf_haptic_create_effect`.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_stop_effect cf_haptic_destroy_effect cf_haptic_pause cf_haptic_unpause cf_haptic_stop_all
 */
CF_API void CF_CALL cf_haptic_destroy_effect(CF_Haptic* haptic, CF_HapticEffect effect);

/**
 * @function cf_haptic_pause
 * @category haptic
 * @brief    Pause all haptics on the device.
 * @param    haptic         The haptic.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_stop_effect cf_haptic_destroy_effect cf_haptic_pause cf_haptic_unpause cf_haptic_stop_all
 */
CF_API void CF_CALL cf_haptic_pause(CF_Haptic* haptic);

/**
 * @function cf_haptic_unpause
 * @category haptic
 * @brief    Unpause all haptics on the device.
 * @param    haptic         The haptic.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_stop_effect cf_haptic_destroy_effect cf_haptic_pause cf_haptic_unpause cf_haptic_stop_all
 */
CF_API void CF_CALL cf_haptic_unpause(CF_Haptic* haptic);

/**
 * @function cf_haptic_stop_all
 * @category haptic
 * @brief    Stops all haptics on the device.
 * @param    haptic         The haptic.
 * @remarks  The effects are not destroyed.
 * @related  CF_Haptic cf_haptic_open CF_HapticEffect cf_haptic_create_effect cf_haptic_stop_effect cf_haptic_destroy_effect cf_haptic_pause cf_haptic_unpause cf_haptic_stop_all
 */
CF_API void CF_CALL cf_haptic_stop_all(CF_Haptic* haptic);

// -------------------------------------------------------------------------------------------------

/**
 * @function cf_haptic_rumble_supported
 * @category haptic
 * @brief    Checks to see if a simple sine/leftright haptic can be supported on the device.
 * @param    haptic         The haptic.
 * @remarks  Creates a set of decent parameters to play a rumbling effect in an easy to use way.
 * @related  CF_Haptic cf_haptic_open cf_haptic_rumble_supported cf_haptic_rumble_play cf_haptic_rumble_stop
 */
CF_API bool CF_CALL cf_haptic_rumble_supported(CF_Haptic* haptic);

/**
 * @function cf_haptic_rumble_play
 * @category haptic
 * @brief    Starts playing a simple rumble effect
 * @param    haptic                 The haptic.
 * @param    strength               Must be from 0.0f to 1.0f. How strong the rumble is.
 * @param    duration_milliseconds  The duration of the rumble in milliseconds.
 * @remarks  Successive calls on this function will update the underlying rumble effect, instead of creating and playing multiple different effect instances.
 * @related  CF_Haptic cf_haptic_open cf_haptic_rumble_supported cf_haptic_rumble_play cf_haptic_rumble_stop
 */
CF_API void CF_CALL cf_haptic_rumble_play(CF_Haptic* haptic, float strength, int duration_milliseconds);

/**
 * @function cf_haptic_rumble_stop
 * @category haptic
 * @brief    Stops playing the simple rumble effect.
 * @param    haptic         The haptic.
 * @remarks  See `cf_haptic_rumble_play` for more details.
 * @related  CF_Haptic cf_haptic_open cf_haptic_rumble_supported cf_haptic_rumble_play cf_haptic_rumble_stop
 */
CF_API void CF_CALL cf_haptic_rumble_stop(CF_Haptic* haptic);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
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
#define CF_ENUM(K, V) CF_INLINE constexpr HapticType K = CF_##K;
CF_HAPTIC_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(HapticType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HAPTIC_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

using HapticWaveType = CF_HapticWaveType;
#define CF_ENUM(K, V) CF_INLINE constexpr HapticWaveType K = CF_##K;
CF_HAPTIC_WAVE_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(HapticWaveType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HAPTIC_WAVE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE Haptic* haptic_open(Joypad* joypad) { return cf_haptic_open(joypad); }
CF_INLINE void haptic_close(Haptic* haptic) { cf_haptic_close(haptic); }
CF_INLINE bool haptic_supports(Haptic* haptic, HapticType type) { return cf_haptic_supports(haptic,type); }
CF_INLINE void haptic_set_gain(Haptic* haptic, float gain) { cf_haptic_set_gain(haptic,gain); }
CF_INLINE HapticEffect haptic_create_effect(Haptic* haptic, HapticData data) { return cf_haptic_create_effect(haptic,data); }
CF_INLINE void haptic_run_effect(Haptic* haptic, HapticEffect effect, int iterations) { cf_haptic_run_effect(haptic,effect,iterations); }
CF_INLINE void haptic_update_effect(Haptic* haptic, HapticEffect effect, HapticData data) { cf_haptic_update_effect(haptic,effect,data); }
CF_INLINE void haptic_stop_effect(Haptic* haptic, HapticEffect effect) { cf_haptic_stop_effect(haptic,effect); }
CF_INLINE void haptic_destroy_effect(Haptic* haptic, HapticEffect effect) { cf_haptic_destroy_effect(haptic,effect); }
CF_INLINE void haptic_pause(Haptic* haptic) { cf_haptic_pause(haptic); }
CF_INLINE void haptic_unpause(Haptic* haptic) { cf_haptic_unpause(haptic); }
CF_INLINE void haptic_stop_all(Haptic* haptic) { cf_haptic_stop_all(haptic); }
CF_INLINE bool haptic_rumble_supported(Haptic* haptic) { return cf_haptic_rumble_supported(haptic); }
CF_INLINE void haptic_rumble_play(Haptic* haptic, float strength, int duration_milliseconds) { cf_haptic_rumble_play(haptic,strength,duration_milliseconds); }
CF_INLINE void haptic_rumble_stop(Haptic* haptic) { cf_haptic_rumble_stop(haptic); }

}

#endif // CF_CPP

#endif // CF_HAPTICS_H
