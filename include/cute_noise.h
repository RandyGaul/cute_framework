/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CUTE_PERLIN_H
#define CUTE_PERLIN_H

#include "cute_defines.h"
#include "cute_graphics.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Noise
 * @category noise
 * @brief    Used to generate noise at specified points.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           struct is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
typedef struct CF_Noise { uint64_t id; } CF_Noise;
// @end

/**
 * @function cf_make_noise
 * @category noise
 * @brief    Returns a `CF_Noise` for generating noise at specified points.
 * @param    seed        Used to seed the sequence of numbers generated. Default 0.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           function is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_make_noise_fbm cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API CF_Noise CF_CALL cf_make_noise(uint64_t seed);

/**
 * @function cf_make_noise_fbm
 * @category noise
 * @brief    Returns a `CF_Noise` for generating noise at specified points using fractal brownian motion.
 * @param    seed        Used to seed the sequence of numbers generated. Default 0.
 * @param    scale       Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
 * @param    lacunarity  The difference in the period of the noise between each octave. Default 2.0f.
 * @param    octaves     The number of octaves to sum together. Higher numbers has worse performance. Default 3.
 * @param    falloff     How much contribution higher octaves have compared to lower ones. Default 0.5f.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           function is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API CF_Noise CF_CALL cf_make_noise_fbm(uint64_t seed, float scale, float lacunarity, int octaves, float falloff);

/**
 * @function cf_destroy_noise
 * @category noise
 * @brief    Destroys a `CF_Noise` created by `cf_make_noise` or `cf_make_noise_fbm`.
 * @param    CF_Noise    The `CF_Noise` to destroy.
 * @related  CF_Noise cf_make_noise cf_make_noise_fbm cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API void CF_CALL cf_destroy_noise(CF_Noise noise);

/**
 * @function cf_noise2
 * @category noise
 * @brief    Generates a random value given a 2D coordinate.
 * @param    noise      The noise settings.
 * @param    x          Noise at this x-component.
 * @param    y          Noise at this y-component.
 * @return   Returns a random value at the specified point.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           function is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API float CF_CALL cf_noise2(CF_Noise noise, float x, float y);

/**
 * @function cf_noise3
 * @category noise
 * @brief    Generates a random value given a 3D coordinate.
 * @param    noise      The noise settings.
 * @param    x          Noise at this x-component.
 * @param    y          Noise at this y-component.
 * @param    z          Noise at this z-component.
 * @return   Returns a random value at the specified point.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           function is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API float CF_CALL cf_noise3(CF_Noise noise, float x, float y, float z);

/**
 * @function cf_noise4
 * @category noise
 * @brief    Generates a random value given a 4D coordinate.
 * @param    noise      The noise settings.
 * @param    x          Noise at this x-component.
 * @param    y          Noise at this y-component.
 * @param    z          Noise at this z-component.
 * @param    w          Noise at this w-component.
 * @return   Returns a random value at the specified point.
 * @remarks  You're probably looking for image generation functions such as `cf_noise_pixels` or `cf_noise_fbm_pixels`. This
 *           function is fairly low-level and intended for those who know what they're doing.
 * @related  CF_Noise cf_make_noise cf_destroy_noise cf_noise2 cf_noise3 cf_noise4
 */
CF_API float CF_CALL cf_noise4(CF_Noise noise, float x, float y, float z, float w);

/**
 * @function cf_noise_pixels
 * @category noise
 * @brief    Creates an image from noise.
 * @param    w          The width of the image.
 * @param    h          The height of the image.
 * @param    seed       Used to seed the sequence of numbers generated. Default 0.
 * @param    scale      Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
 * @return   Returns a generated image filled with noise.
 * @remarks  The generated noise is quite simple -- you're probably looking for the more advanced `cf_noise_fbm_pixels`, or `cf_noise_fbm_pixels_wrapped`.
 * @related  cf_noise_pixels cf_noise_pixels_wrapped cf_noise_fbm_pixels cf_noise_fbm_pixels_wrapped
 */
CF_API CF_Pixel* CF_CALL cf_noise_pixels(int w, int h, uint64_t seed, float scale);

/**
 * @function cf_noise_pixels_wrapped
 * @category noise
 * @brief    Creates an image from noise that can animate in a loop, and tiles seamlessly.
 * @param    w               The width of the image.
 * @param    h               The height of the image.
 * @param    seed            Used to seed the sequence of numbers generated. Default 0.
 * @param    scale           Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
 * @param    time            A time parameter for animation.
 * @param    time_amplitude  Adjusts how much the animation evolves over the period. Default 1.0f.
 * @return   Returns a generated image filled with noise.
 * @remarks  The generated image can be animated over a loop, and tiles seamlessly in the x-y directions. To control the animation
 *           pass in a float starting at 0, and incremented with `CF_DELTA_TIME` each game tick. This will loop the animation
 *           over a one-second period. You can divide your accumulated time by a frequency to set a number of seconds to loop over.
 *           
 *           If you want the animation to move faster without adjusting the loop time, then adjust `time_amplitude`. This scales how
 *           much the animation will evolve over time. Higher values will have faster and more volatile looking motions.
 * @related  cf_noise_pixels cf_noise_pixels_wrapped cf_noise_fbm_pixels cf_noise_fbm_pixels_wrapped
 */
CF_API CF_Pixel* CF_CALL cf_noise_pixels_wrapped(int w, int h, uint64_t seed, float scale, float time, float time_amplitude);

/**
 * @function cf_noise_fbm_pixels
 * @category noise
 * @brief    Creates an image from noise using fractal brownian motion.
 * @param    w           The width of the image.
 * @param    h           The height of the image.
 * @param    seed        Used to seed the sequence of numbers generated. Default 0.
 * @param    scale       Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
 * @param    lacunarity  The difference in the period of the noise between each octave. Default 2.0f.
 * @param    octaves     The number of octaves to sum together. Higher numbers has worse performance. Default 3.
 * @param    falloff     How much contribution higher octaves have compared to lower ones. Default 0.5f.
 * @return   Returns a generated image filled with noise.
 * @remarks  If you want the image to animate over a loop, or tile seamlessly, then check out `cf_noise_fbm_pixels_wrapped`.
 * @related  cf_noise_pixels cf_noise_pixels_wrapped cf_noise_fbm_pixels cf_noise_fbm_pixels_wrapped
 */
CF_API CF_Pixel* CF_CALL cf_noise_fbm_pixels(int w, int h, uint64_t seed, float scale, float lacunarity, int octaves, float falloff);

/**
 * @function cf_noise_fbm_pixels_wrapped
 * @category noise
 * @brief    Creates an image from noise using fractal brownian motion, that can also animate in a loop, and tiles seamlessly.
 * @param    w           The width of the image.
 * @param    h           The height of the image.
 * @param    seed        Used to seed the sequence of numbers generated. Default 0.
 * @param    scale       Scales up or down the noise in the image, like zooming in or out. Default 1.0f.
 * @param    lacunarity  The difference in the period of the noise between each octave. Default 2.0f.
 * @param    octaves     The number of octaves to sum together. Higher numbers has worse performance. Default 3.
 * @param    falloff     How much contribution higher octaves have compared to lower ones. Default 0.5f.
 * @return   Returns a generated image filled with noise.
 * @remarks  The generated image can be animated over a loop, and tiles seamlessly in the x-y directions. To control the animation
 *           pass in a float starting at 0, and incremented with `CF_DELTA_TIME` each game tick. This will loop the animation
 *           over a one-second period. You can divide your accumulated time by a frequency to set a number of seconds to loop over.
 *           
 *           If you want the animation to move faster without adjusting the loop time, then adjust `time_amplitude`. This scales how
 *           much the animation will evolve over time. Higher values will have faster and more volatile looking motions.
 * @related  cf_noise_pixels cf_noise_pixels_wrapped cf_noise_fbm_pixels cf_noise_fbm_pixels_wrapped
 */
CF_API CF_Pixel* CF_CALL cf_noise_fbm_pixels_wrapped(int w, int h, uint64_t seed, float scale, float lacunarity, int octaves, float falloff, float time, float time_amplitude);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Noise = CF_Noise;

CF_INLINE Noise make_noise(uint64_t seed) { return cf_make_noise(seed); }
CF_INLINE Noise make_noise_fbm(uint64_t seed, float scale, float lacunarity, int octaves, float falloff) { return cf_make_noise_fbm(seed, scale, lacunarity, octaves, falloff); }
CF_INLINE void destroy_noise(CF_Noise noise) { cf_destroy_noise(noise); }
CF_INLINE float noise(CF_Noise noise, v2 p) { return cf_noise2(noise, p.x, p.y); }
CF_INLINE float noise(CF_Noise noise, float x, float y) { return cf_noise2(noise, x, y); }
CF_INLINE float noise(CF_Noise noise, float x, float y, float z) { return cf_noise3(noise, x, y, z); }
CF_INLINE float noise(CF_Noise noise, float x, float y, float z, float w) { return cf_noise4(noise, x, y, z, w); }
CF_INLINE CF_Pixel* noise_pixels(int w, int h, uint64_t seed, float scale) { return cf_noise_pixels(w, h, seed, scale); }
CF_INLINE CF_Pixel* noise_pixels_wrapped(int w, int h, uint64_t seed, float scale, float time, float time_amplitude) { return cf_noise_pixels_wrapped(w, h, seed, scale, time, time_amplitude); }
CF_INLINE CF_Pixel* noise_fbm_pixels(int w, int h, uint64_t seed, float scale, float lacunarity, int octaves, float falloff) { return cf_noise_fbm_pixels(w, h, seed, scale, lacunarity, octaves, falloff); }
CF_INLINE CF_Pixel* noise_fbm_pixels_wrapped(int w, int h, uint64_t seed, float scale, float lacunarity, int octaves, float falloff, float time, float time_amplitude) { return cf_noise_fbm_pixels_wrapped(w, h, seed, scale, lacunarity, octaves, falloff, time, time_amplitude); }

}

#endif // CF_CPP

#endif // CUTE_PERLIN_H
