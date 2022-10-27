/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#ifndef CUTE_RND_H
#define CUTE_RND_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * A random number generator of the type LFSR (linear feedback shift registers). This specific
 * implementation uses the XorShift+ variation, and returns 64-bit random numbers. More information
 * can be found on Wikipedia.
 * https://en.wikipedia.org/wiki/Xorshift
 *
 * This implementation comes from Mattias Gustavsson's single-file header collection.
 * https://github.com/mattiasgustavsson/libs/blob/main/rnd.h
 */
typedef struct cf_rnd_t
{
	uint64_t state[2];
} cf_rnd_t;

static CUTE_INLINE cf_rnd_t CUTE_CALL cf_rnd_seed(uint64_t seed);

static CUTE_INLINE uint64_t CUTE_CALL cf_rnd_next(cf_rnd_t* rnd);
static CUTE_INLINE float    CUTE_CALL cf_rnd_next_float(cf_rnd_t* rnd);
static CUTE_INLINE double   CUTE_CALL cf_rnd_next_double(cf_rnd_t* rnd);
static CUTE_INLINE int      CUTE_CALL cf_rnd_next_range_int(cf_rnd_t* rnd, int min, int max);
static CUTE_INLINE uint64_t CUTE_CALL cf_rnd_next_range_uint64(cf_rnd_t* rnd, uint64_t min, uint64_t max);
static CUTE_INLINE float    CUTE_CALL cf_rnd_next_range_float(cf_rnd_t* rnd, float min, float max);
static CUTE_INLINE double   CUTE_CALL cf_rnd_next_range_double(cf_rnd_t* rnd, double min, double max);

// -------------------------------------------------------------------------------------------------

static CUTE_INLINE uint64_t cf_internal_rnd_murmur3_avalanche64(uint64_t h)
{
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccd;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53;
	h ^= h >> 33;
	return h;
}

static CUTE_INLINE cf_rnd_t cf_rnd_seed(uint64_t seed)
{
	cf_rnd_t rnd;
	uint64_t value = cf_internal_rnd_murmur3_avalanche64((seed << 1ULL) | 1ULL);
	rnd.state[0] = value;
	rnd.state[1] = cf_internal_rnd_murmur3_avalanche64(value);
	return rnd;
}

static CUTE_INLINE uint64_t cf_rnd_next(cf_rnd_t* rnd)
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

static CUTE_INLINE float cf_rnd_next_float(cf_rnd_t* rnd)
{
	uint32_t value = (uint32_t)(cf_rnd_next(rnd) >> 32);

	// Convert a randomized uint32_t value to a float value x in the range 0.0f <= x < 1.0f.
	// Contributed by Jonatan Hedborg.
	uint32_t exponent = 127;
	uint32_t mantissa = value >> 9;
	uint32_t result = (exponent << 23) | mantissa;
	return *(float*)&result - 1.0f;
}

static CUTE_INLINE double cf_rnd_next_double(cf_rnd_t* rnd)
{
	uint64_t value = cf_rnd_next(rnd);
	uint64_t exponent = 1023;
	uint64_t mantissa = value >> 12;
	uint64_t result = (exponent << 52) | mantissa;
	return *(double*)&result - 1.0;
}

static CUTE_INLINE int cf_rnd_next_range_int(cf_rnd_t* rnd, int min, int max)
{
	int range = (max - min) + 1;
	int value = (int)(cf_rnd_next(rnd) % range);
	return min + value;
}

static CUTE_INLINE uint64_t cf_rnd_next_range_uint64(cf_rnd_t* rnd, uint64_t min, uint64_t max)
{
	uint64_t range = (max - min) + 1;
	uint64_t value = cf_rnd_next(rnd) % range;
	return min + value;
}

static CUTE_INLINE float cf_rnd_next_range_float(cf_rnd_t* rnd, float min, float max)
{
	float range = max - min;
	float value = cf_rnd_next_float(rnd) * range;
	return min + value;
}

static CUTE_INLINE double cf_rnd_next_range_double(cf_rnd_t* rnd, double min, double max)
{
	double range = max - min;
	double value = cf_rnd_next_float(rnd) * range;
	return min + value;
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using rnd_t = cf_rnd_t;

namespace internal
{

static CUTE_INLINE uint64_t rnd_murmur3_avalanche64(uint64_t h) { return cf_internal_rnd_murmur3_avalanche64(h); }

}

static CUTE_INLINE cf_rnd_t rnd_seed(uint64_t seed) { return cf_rnd_seed(seed); }
static CUTE_INLINE uint64_t rnd_next(cf_rnd_t* rnd) { return cf_rnd_next(rnd); }
static CUTE_INLINE float    rnd_next_float(cf_rnd_t* rnd) { return cf_rnd_next_float(rnd); }
static CUTE_INLINE double   rnd_next_double(cf_rnd_t* rnd) { return cf_rnd_next_double(rnd); }
static CUTE_INLINE int      rnd_next_range(cf_rnd_t* rnd, int min, int max) { return cf_rnd_next_range_int(rnd, min, max); }
static CUTE_INLINE uint64_t rnd_next_range(cf_rnd_t* rnd, uint64_t min, uint64_t max) { return cf_rnd_next_range_uint64(rnd, min, max); }
static CUTE_INLINE float    rnd_next_range(cf_rnd_t* rnd, float min, float max) { return cf_rnd_next_range_float(rnd, min, max); }
static CUTE_INLINE double   rnd_next_range(cf_rnd_t* rnd, double min, double max) { return cf_rnd_next_range_double(rnd, min, max); }

static CUTE_INLINE uint64_t rnd_next(cf_rnd_t& rnd) { return cf_rnd_next(&rnd); }
static CUTE_INLINE float    rnd_next_float(cf_rnd_t& rnd) { return cf_rnd_next_float(&rnd); }
static CUTE_INLINE double   rnd_next_double(cf_rnd_t& rnd) { return cf_rnd_next_double(&rnd); }
static CUTE_INLINE int      rnd_next_range(cf_rnd_t& rnd, int min, int max) { return cf_rnd_next_range_int(&rnd, min, max); }
static CUTE_INLINE uint64_t rnd_next_range(cf_rnd_t& rnd, uint64_t min, uint64_t max) { return cf_rnd_next_range_uint64(&rnd, min, max); }
static CUTE_INLINE float    rnd_next_range(cf_rnd_t& rnd, float min, float max) { return cf_rnd_next_range_float(&rnd, min, max); }
static CUTE_INLINE double   rnd_next_range(cf_rnd_t& rnd, double min, double max) { return cf_rnd_next_range_double(&rnd, min, max); }

}

#endif // CUTE_CPP

#endif // CUTE_RND_H
