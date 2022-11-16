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

#ifndef CUTE_COLOR_H
#define CUTE_COLOR_H

#include "cute_defines.h"
#include "cute_string.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 16-byte value with 0.0f to 1.0f components.
typedef struct cf_color_t
{
	float r;
	float g;
	float b;
	float a;
} cf_color_t;

// 4-byte value with 0-255 components.
typedef union cf_pixel_t
{
	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} colors;
	uint32_t val;
} cf_pixel_t;

CUTE_INLINE cf_color_t cf_make_color_rgb_f(float r, float g, float b) { cf_color_t color; color.r = r; color.g = g; color.b = b; color.a = 1.0f; return color; }
CUTE_INLINE cf_color_t cf_make_color_rgba_f(float r, float g, float b, float a) { cf_color_t color; color.r = r; color.g = g; color.b = b; color.a = a; return color; }
CUTE_INLINE cf_color_t cf_make_color_rgb(uint8_t r, uint8_t g, uint8_t b) { cf_color_t color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = 1.0f; return color; }
CUTE_INLINE cf_color_t cf_make_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { cf_color_t color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = (float)a / 255.0f; return color; }
CUTE_INLINE cf_color_t cf_make_color_hex(int hex) { return cf_make_color_rgba((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }
CUTE_INLINE cf_color_t cf_make_color_hex_string(const char* hex) { cf_make_color_hex((int)stohex(hex)); }

CUTE_INLINE cf_pixel_t cf_make_pixel_rgb_f(float r, float g, float b) { cf_pixel_t p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = 255; return p; }
CUTE_INLINE cf_pixel_t cf_make_pixel_rgba_f(float r, float g, float b, float a) { cf_pixel_t p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = (uint8_t)(a * 255.0f); return p; }
CUTE_INLINE cf_pixel_t cf_make_pixel_rgb(uint8_t r, uint8_t g, uint8_t b) { cf_pixel_t p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = 255; return p; }
CUTE_INLINE cf_pixel_t cf_make_pixel_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { cf_pixel_t p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = a; return p; }
CUTE_INLINE cf_pixel_t cf_make_pixel_hex(int hex) { return cf_make_pixel_rgba((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }
CUTE_INLINE cf_pixel_t cf_make_pixel_hex_string(const char* hex) { cf_make_pixel_hex((int)stohex(hex)); }

CUTE_INLINE cf_color_t cf_mul_color(cf_color_t a, float s) { return cf_make_color_rgba_f(a.r * s, a.g * s, a.b * s, a.a * s); }
CUTE_INLINE cf_color_t cf_div_color(cf_color_t a, float s) { return cf_make_color_rgba_f(a.r / s, a.g / s, a.b / s, a.a / s); }
CUTE_INLINE cf_color_t cf_add_color(cf_color_t a, cf_color_t b) { return cf_make_color_rgba_f(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a); }
CUTE_INLINE cf_color_t cf_sub_color(cf_color_t a, cf_color_t b) { return cf_make_color_rgba_f(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a); }
CUTE_INLINE cf_color_t cf_color_lerp(cf_color_t a, cf_color_t b, float s) { return cf_add_color(a, cf_mul_color(cf_sub_color(b, a), s)); }
CUTE_INLINE cf_color_t cf_color_premultiply(cf_color_t c) { c.r *= c.a; c.g *= c.a; c.b *= c.a; return c; }

CUTE_INLINE cf_pixel_t cf_mul_pixel(cf_pixel_t a, float s) { return cf_make_pixel_rgba_f(a.colors.r * s, a.colors.g * s, a.colors.b * s, a.colors.a * s); }
CUTE_INLINE cf_pixel_t cf_div_pixel(cf_pixel_t a, float s) { return cf_make_pixel_rgba_f(a.colors.r / s, a.colors.g / s, a.colors.b / s, a.colors.a / s); }
CUTE_INLINE cf_pixel_t cf_add_pixel(cf_pixel_t a, cf_pixel_t b) { return cf_make_pixel_rgba(a.colors.r + b.colors.r, a.colors.g + b.colors.g, a.colors.b + b.colors.b, a.colors.a + b.colors.a); }
CUTE_INLINE cf_pixel_t cf_sub_pixel(cf_pixel_t a, cf_pixel_t b) { return cf_make_pixel_rgba(a.colors.r - b.colors.r, a.colors.g - b.colors.g, a.colors.b - b.colors.b, a.colors.a - b.colors.a); }
CUTE_INLINE cf_pixel_t cf_pixel_lerp(cf_pixel_t a, cf_pixel_t b, float s) { return cf_add_pixel(a, cf_mul_pixel(cf_sub_pixel(b, a), s)); }
CUTE_INLINE cf_pixel_t cf_pixel_premultiply(cf_pixel_t c) { c.colors.r *= c.colors.a; c.colors.g *= c.colors.a; c.colors.b *= c.colors.a; return c; }

CUTE_INLINE cf_color_t cf_pixel_to_color(cf_pixel_t p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t cf_pixel_to_int_rgba(cf_pixel_t p) { return p.val; }
CUTE_INLINE uint32_t cf_pixel_to_int_rgb(cf_pixel_t p) { return p.val | 0x000000FF; }
CUTE_INLINE char* cf_pixel_to_string(cf_pixel_t p) { char* s = NULL; return shex(s, p.val); } // Call `sfree` when done.

CUTE_INLINE cf_pixel_t cf_color_to_pixel(cf_color_t c) { cf_pixel_t p; p.colors.r = (int)((uint8_t)(c.r * 255.0f)); p.colors.g = (int)((uint8_t)(c.g * 255.0f)); p.colors.b = (int)((uint8_t)(c.b * 255.0f)); p.colors.a = (int)((uint8_t)(c.a * 255.0f)); return p; }
CUTE_INLINE uint32_t cf_color_to_int_rgb(cf_color_t c) { return cf_color_to_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t cf_color_to_int_rgba(cf_color_t c) { return cf_color_to_pixel(c).val; }
CUTE_INLINE char* cf_color_to_string(cf_color_t c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); } // Call `sfree` when done.

CUTE_INLINE cf_color_t cf_color_invisible() { return cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_black() { return cf_make_color_rgb_f(0.0f, 0.0f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_white() { return cf_make_color_rgb_f(1.0f, 1.0f, 1.0f); }
CUTE_INLINE cf_color_t cf_color_red() { return cf_make_color_rgb_f(1.0f, 0.0f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_green() { return cf_make_color_rgb_f(0.0f, 1.0f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_blue() { return cf_make_color_rgb_f(0.0f, 0.0f, 1.0f); }
CUTE_INLINE cf_color_t cf_color_yellow() { return cf_make_color_rgb_f(1.0f, 1.0f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_orange() { return cf_make_color_rgb_f(1.0f, 0.65f, 0.0f); }
CUTE_INLINE cf_color_t cf_color_purple() { return cf_make_color_rgb_f(1.0f, 0.0f, 1.0f); }

CUTE_INLINE cf_pixel_t cf_pixel_invisible() { return cf_make_pixel_hex(0); }
CUTE_INLINE cf_pixel_t cf_pixel_black() { return cf_make_pixel_rgb(0, 0, 0); }
CUTE_INLINE cf_pixel_t cf_pixel_white() { return cf_make_pixel_rgb(255, 255, 255); }
CUTE_INLINE cf_pixel_t cf_pixel_red() { return cf_make_pixel_rgb(255, 0, 0); }
CUTE_INLINE cf_pixel_t cf_pixel_green() { return cf_make_pixel_rgb(0, 255, 0); }
CUTE_INLINE cf_pixel_t cf_pixel_blue() { return cf_make_pixel_rgb(0, 0, 255); }
CUTE_INLINE cf_pixel_t cf_pixel_yellow() { return cf_make_pixel_rgb(255, 255, 0); }
CUTE_INLINE cf_pixel_t cf_pixel_orange() { return cf_make_pixel_rgb(255, 165, 0); }
CUTE_INLINE cf_pixel_t cf_pixel_purple() { return cf_make_pixel_rgb(255, 0, 255); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using pixel_t = cf_pixel_t;
using color_t = cf_color_t;

CUTE_INLINE color_t make_color(float r, float g, float b) { return cf_make_color_rgb_f(r, g, b); }
CUTE_INLINE color_t make_color(float r, float g, float b, float a) { return cf_make_color_rgba_f(r, g, b, a); }
CUTE_INLINE color_t make_color(uint8_t r, uint8_t g, uint8_t b) { return cf_make_color_rgb(r, g, b); }
CUTE_INLINE color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_color_rgba(r, g, b, a); }
CUTE_INLINE color_t make_color(int hex) { return cf_make_color_hex(hex); }
CUTE_INLINE color_t make_color(const char* s) { return make_color((int)stohex(s)); }

CUTE_INLINE pixel_t make_pixel(float r, float g, float b) { return cf_make_pixel_rgb_f(r, g, b); }
CUTE_INLINE pixel_t make_pixel(float r, float g, float b, float a) { return cf_make_pixel_rgba_f(r, g, b, a); }
CUTE_INLINE pixel_t make_pixel(uint8_t r, uint8_t g, uint8_t b) { cf_make_pixel_rgb(r, g, b); }
CUTE_INLINE pixel_t make_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_pixel_rgba(r, g, b, a); }
CUTE_INLINE pixel_t make_pixel(int hex) { return cf_make_pixel_hex(hex); }
CUTE_INLINE pixel_t make_pixel(const char* hex) { cf_make_pixel_hex((int)stohex(hex)); }

CUTE_INLINE color_t operator*(color_t a, float s) { return cf_mul_color(a, s); }
CUTE_INLINE color_t operator/(color_t a, float s) { return cf_div_color(a, s); }
CUTE_INLINE color_t operator+(color_t a, color_t b) { return cf_add_color(a, b); }
CUTE_INLINE color_t operator-(color_t a, color_t b) { return cf_sub_color(a, b); }
CUTE_INLINE color_t lerp(color_t a, color_t b, float s) { return cf_color_lerp(a, b, s); }
CUTE_INLINE color_t premultiply(color_t c) { return cf_color_premultiply(c); }

CUTE_INLINE pixel_t operator*(pixel_t a, float s) { return cf_mul_pixel(a, s); }
CUTE_INLINE pixel_t operator/(pixel_t a, float s) { return cf_div_pixel(a, s); }
CUTE_INLINE pixel_t operator+(pixel_t a, pixel_t b) { return cf_add_pixel(a, b); }
CUTE_INLINE pixel_t operator-(pixel_t a, pixel_t b) { return cf_sub_pixel(a, b); }
CUTE_INLINE pixel_t lerp(pixel_t a, pixel_t b, float s) { return cf_pixel_lerp(a, b, s); }
CUTE_INLINE pixel_t premultiply(pixel_t p) { return cf_pixel_premultiply(p); }

CUTE_INLINE color_t to_color(pixel_t p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t to_int_rgba(pixel_t p) { return p.val; }
CUTE_INLINE uint32_t to_int_rgb(pixel_t p) { return p.val | 0x000000FF; }
CUTE_INLINE string_t to_string(pixel_t p) { char* s = NULL; return shex(s, p.val); }

CUTE_INLINE pixel_t to_pixel(color_t c) { return cf_color_to_pixel(c); }
CUTE_INLINE uint32_t to_int_rgb(color_t c) { return cf_color_to_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t to_int_rgba(color_t c) { return cf_color_to_pixel(c).val; }
CUTE_INLINE string_t to_string(color_t c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); }

CUTE_INLINE color_t color_invisible() { return cf_color_invisible(); }
CUTE_INLINE color_t color_black() { return cf_color_black(); }
CUTE_INLINE color_t color_white() { return cf_color_white(); }
CUTE_INLINE color_t color_red() { return cf_color_red(); }
CUTE_INLINE color_t color_green() { return cf_color_green(); }
CUTE_INLINE color_t color_blue() { return cf_color_blue(); }
CUTE_INLINE color_t color_yellow() { return cf_color_yellow(); }
CUTE_INLINE color_t color_orange() { return cf_color_orange(); }
CUTE_INLINE color_t color_purple() { return cf_color_purple(); }

CUTE_INLINE pixel_t pixel_invisible() { return cf_pixel_invisible(); }
CUTE_INLINE pixel_t pixel_black() { return cf_pixel_black(); }
CUTE_INLINE pixel_t pixel_white() { return cf_pixel_white(); }
CUTE_INLINE pixel_t pixel_red() { return cf_pixel_red(); }
CUTE_INLINE pixel_t pixel_green() { return cf_pixel_green(); }
CUTE_INLINE pixel_t pixel_blue() { return cf_pixel_blue(); }
CUTE_INLINE pixel_t pixel_yellow() { return cf_pixel_yellow(); }
CUTE_INLINE pixel_t pixel_orange() { return cf_pixel_orange(); }
CUTE_INLINE pixel_t pixel_purple() { return cf_pixel_purple(); }

}

#endif // CUTE_CPP

#endif // CUTE_COLOR_H
