/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_RND_H
#define CF_RND_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_RndState
 * @category random
 * @brief    A random number generator.
 * @remarks  A random number generator of the type LFSR (linear feedback shift registers). This specific
 *           implementation uses the XorShift+ variation, and returns 64-bit random numbers. More information
 *           can be found on Wikipedia.
 *           https://en.wikipedia.org/wiki/Xorshift
 *           
 *           This implementation comes from Mattias Gustavsson's single-file header collection.
 *           https://github.com/mattiasgustavsson/libs/blob/main/rnd.h
 * @related  CF_RndState cf_rnd_seed cf_rnd
 */
typedef struct CF_RndState
{
	/* @member Just two `uint64_t`'s for the internal state. Very small! These are setup by `cf_rnd_seed`. */
	uint64_t state[2];
} CF_RndState;
// @end

/**
 * @function cf_rnd_seed
 * @category random
 * @brief    Returns an initialized `CF_Rnd` based on an initial `seed` value.
 * @param    seed         The initial seed value for the random number generator.
 * @remarks  The `seed` is used to control which set of random numbers get generated. The numbers are generated in a completely
 *           deterministic way, so it's often important for many games to control or note which seed is used.
 * @related  CF_RndState cf_rnd_seed cf_rnd
 */
CF_INLINE CF_RndState CF_CALL cf_rnd_seed(uint64_t seed);

/**
 * @function cf_rnd
 * @category random
 * @brief    Returns a random `uint64_t`.
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE uint64_t CF_CALL cf_rnd(CF_RndState* rnd);

/**
 * @function cf_rnd_float
 * @category random
 * @brief    Returns a random `float`.
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE float    CF_CALL cf_rnd_float(CF_RndState* rnd);

/**
 * @function cf_rnd_double
 * @category random
 * @brief    Returns a random `double`.
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE double   CF_CALL cf_rnd_double(CF_RndState* rnd);

/**
 * @function cf_rnd_range_int
 * @category random
 * @brief    Returns a random `int` from the range `min` to `max` (inclusive).
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE int      CF_CALL cf_rnd_range_int(CF_RndState* rnd, int min, int max);

/**
 * @function cf_rnd_range_uint64
 * @category random
 * @brief    Returns a random `uint64_t` from the range `min` to `max` (inclusive).
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE uint64_t CF_CALL cf_rnd_range_uint64(CF_RndState* rnd, uint64_t min, uint64_t max);

/**
 * @function cf_rnd_range_float
 * @category random
 * @brief    Returns a random `float` from the range `min` to `max` (inclusive).
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE float    CF_CALL cf_rnd_range_float(CF_RndState* rnd, float min, float max);

/**
 * @function cf_rnd_range_double
 * @category random
 * @brief    Returns a random `double` from the range `min` to `max` (inclusive).
 * @param    rnd          The random number generator state.
 * @related  CF_RndState cf_rnd_seed cf_rnd cf_rnd_float cf_rnd_double cf_rnd_range_int cf_rnd_range_uint64 cf_rnd_range_float cf_rnd_range_double
 */
CF_INLINE double   CF_CALL cf_rnd_range_double(CF_RndState* rnd, double min, double max);

// -------------------------------------------------------------------------------------------------

CF_INLINE uint64_t cf_internal_rnd_murmur3_avalanche64(uint64_t h)
{
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccd;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53;
	h ^= h >> 33;
	return h;
}

CF_INLINE CF_RndState cf_rnd_seed(uint64_t seed)
{
	CF_RndState rnd;
	uint64_t value = cf_internal_rnd_murmur3_avalanche64((seed << 1ULL) | 1ULL);
	rnd.state[0] = value;
	rnd.state[1] = cf_internal_rnd_murmur3_avalanche64(value);
	return rnd;
}

CF_INLINE uint64_t cf_rnd(CF_RndState* rnd)
{
	uint64_t x = rnd->state[0];
	uint64_t y = rnd->state[1];
	rnd->state[0] = y;
	x ^= x << 23;
	x ^= x >> 17;
	x ^= y ^ (y >> 26);
	rnd->state[1] = x;
	return x + y;
}

CF_INLINE float cf_rnd_float(CF_RndState* rnd)
{
	uint32_t value = (uint32_t)(cf_rnd(rnd) >> 32);

	// Convert a randomized uint32_t value to a float value x in the range 0.0f <= x < 1.0f.
	// Contributed by Jonatan Hedborg.
	uint32_t exponent = 127;
	uint32_t mantissa = value >> 9;
	uint32_t result = (exponent << 23) | mantissa;
	return *(float*)&result - 1.0f;
}

CF_INLINE double cf_rnd_double(CF_RndState* rnd)
{
	uint64_t value = cf_rnd(rnd);
	uint64_t exponent = 1023;
	uint64_t mantissa = value >> 12;
	uint64_t result = (exponent << 52) | mantissa;
	return *(double*)&result - 1.0;
}

CF_INLINE int cf_rnd_range_int(CF_RndState* rnd, int min, int max)
{
	int range = (max - min) + 1;
	int value = (int)(cf_rnd(rnd) % range);
	return min + value;
}

CF_INLINE uint64_t cf_rnd_range_uint64(CF_RndState* rnd, uint64_t min, uint64_t max)
{
	uint64_t range = (max - min) + 1;
	uint64_t value = cf_rnd(rnd) % range;
	return min + value;
}

CF_INLINE float cf_rnd_range_float(CF_RndState* rnd, float min, float max)
{
	float range = max - min;
	float value = cf_rnd_float(rnd) * range;
	return min + value;
}

CF_INLINE double cf_rnd_range_double(CF_RndState* rnd, double min, double max)
{
	double range = max - min;
	double value = cf_rnd_float(rnd) * range;
	return min + value;
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Rnd = CF_RndState;

namespace internal
{

CF_INLINE uint64_t rnd_murmur3_avalanche64(uint64_t h) { return cf_internal_rnd_murmur3_avalanche64(h); }

}

CF_INLINE Rnd      rnd_seed(uint64_t seed) { return cf_rnd_seed(seed); }
CF_INLINE uint64_t rnd(Rnd* rnd) { return cf_rnd(rnd); }
CF_INLINE float    rnd_float(Rnd* rnd) { return cf_rnd_float(rnd); }
CF_INLINE double   rnd_double(Rnd* rnd) { return cf_rnd_double(rnd); }
CF_INLINE int      rnd_range(Rnd* rnd, int min, int max) { return cf_rnd_range_int(rnd, min, max); }
CF_INLINE uint64_t rnd_range(Rnd* rnd, uint64_t min, uint64_t max) { return cf_rnd_range_uint64(rnd, min, max); }
CF_INLINE float    rnd_range(Rnd* rnd, float min, float max) { return cf_rnd_range_float(rnd, min, max); }
CF_INLINE double   rnd_range(Rnd* rnd, double min, double max) { return cf_rnd_range_double(rnd, min, max); }

CF_INLINE uint64_t rnd(Rnd& rnd) { return cf_rnd(&rnd); }
CF_INLINE float    rnd_float(Rnd& rnd) { return cf_rnd_float(&rnd); }
CF_INLINE double   rnd_double(Rnd& rnd) { return cf_rnd_double(&rnd); }
CF_INLINE int      rnd_range(Rnd& rnd, int min, int max) { return cf_rnd_range_int(&rnd, min, max); }
CF_INLINE uint64_t rnd_range(Rnd& rnd, uint64_t min, uint64_t max) { return cf_rnd_range_uint64(&rnd, min, max); }
CF_INLINE float    rnd_range(Rnd& rnd, float min, float max) { return cf_rnd_range_float(&rnd, min, max); }
CF_INLINE double   rnd_range(Rnd& rnd, double min, double max) { return cf_rnd_range_double(&rnd, min, max); }

}

#endif // CF_CPP

#endif // CF_RND_H
