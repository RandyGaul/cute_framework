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
typedef struct CF_Color
{
	float r;
	float g;
	float b;
	float a;
} CF_Color;

// 4-byte value with 0-255 components.
typedef union CF_Pixel
{
	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} colors;
	uint32_t val;
} CF_Pixel;

CUTE_INLINE CF_Color cf_make_color_rgb_f(float r, float g, float b) { CF_Color color; color.r = r; color.g = g; color.b = b; color.a = 1.0f; return color; }
CUTE_INLINE CF_Color cf_make_color_rgba_f(float r, float g, float b, float a) { CF_Color color; color.r = r; color.g = g; color.b = b; color.a = a; return color; }
CUTE_INLINE CF_Color cf_make_color_rgb(uint8_t r, uint8_t g, uint8_t b) { CF_Color color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = 1.0f; return color; }
CUTE_INLINE CF_Color cf_make_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { CF_Color color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = (float)a / 255.0f; return color; }
CUTE_INLINE CF_Color cf_make_color_hex(int hex) { return cf_make_color_rgba((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }
CUTE_INLINE CF_Color cf_make_color_hex_string(const char* hex) { cf_make_color_hex((int)stohex(hex)); }

CUTE_INLINE CF_Pixel cf_make_pixel_rgb_f(float r, float g, float b) { CF_Pixel p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = 255; return p; }
CUTE_INLINE CF_Pixel cf_make_pixel_rgba_f(float r, float g, float b, float a) { CF_Pixel p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = (uint8_t)(a * 255.0f); return p; }
CUTE_INLINE CF_Pixel cf_make_pixel_rgb(uint8_t r, uint8_t g, uint8_t b) { CF_Pixel p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = 255; return p; }
CUTE_INLINE CF_Pixel cf_make_pixel_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { CF_Pixel p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = a; return p; }
CUTE_INLINE CF_Pixel cf_make_pixel_hex(int hex) { return cf_make_pixel_rgba((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }
CUTE_INLINE CF_Pixel cf_make_pixel_hex_string(const char* hex) { cf_make_pixel_hex((int)stohex(hex)); }

CUTE_INLINE CF_Color cf_mul_color(CF_Color a, float s) { return cf_make_color_rgba_f(a.r * s, a.g * s, a.b * s, a.a * s); }
CUTE_INLINE CF_Color cf_div_color(CF_Color a, float s) { return cf_make_color_rgba_f(a.r / s, a.g / s, a.b / s, a.a / s); }
CUTE_INLINE CF_Color cf_add_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a); }
CUTE_INLINE CF_Color cf_sub_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a); }
CUTE_INLINE CF_Color cf_color_lerp(CF_Color a, CF_Color b, float s) { return cf_add_color(a, cf_mul_color(cf_sub_color(b, a), s)); }
CUTE_INLINE CF_Color cf_color_premultiply(CF_Color c) { c.r *= c.a; c.g *= c.a; c.b *= c.a; return c; }

CUTE_INLINE CF_Pixel cf_mul_pixel(CF_Pixel a, float s) { return cf_make_pixel_rgba_f(a.colors.r * s, a.colors.g * s, a.colors.b * s, a.colors.a * s); }
CUTE_INLINE CF_Pixel cf_div_pixel(CF_Pixel a, float s) { return cf_make_pixel_rgba_f(a.colors.r / s, a.colors.g / s, a.colors.b / s, a.colors.a / s); }
CUTE_INLINE CF_Pixel cf_add_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(a.colors.r + b.colors.r, a.colors.g + b.colors.g, a.colors.b + b.colors.b, a.colors.a + b.colors.a); }
CUTE_INLINE CF_Pixel cf_sub_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(a.colors.r - b.colors.r, a.colors.g - b.colors.g, a.colors.b - b.colors.b, a.colors.a - b.colors.a); }
CUTE_INLINE CF_Pixel cf_pixel_lerp(CF_Pixel a, CF_Pixel b, float s) { return cf_add_pixel(a, cf_mul_pixel(cf_sub_pixel(b, a), s)); }
CUTE_INLINE CF_Pixel cf_pixel_premultiply(CF_Pixel c) { c.colors.r *= c.colors.a; c.colors.g *= c.colors.a; c.colors.b *= c.colors.a; return c; }

CUTE_INLINE CF_Color CF_Pixelo_color(CF_Pixel p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t CF_Pixelo_int_rgba(CF_Pixel p) { return p.val; }
CUTE_INLINE uint32_t CF_Pixelo_int_rgb(CF_Pixel p) { return p.val | 0x000000FF; }
CUTE_INLINE char* CF_Pixelo_string(CF_Pixel p) { char* s = NULL; return shex(s, p.val); } // Call `sfree` when done.

CUTE_INLINE CF_Pixel CF_Coloro_pixel(CF_Color c) { CF_Pixel p; p.colors.r = (int)((uint8_t)(c.r * 255.0f)); p.colors.g = (int)((uint8_t)(c.g * 255.0f)); p.colors.b = (int)((uint8_t)(c.b * 255.0f)); p.colors.a = (int)((uint8_t)(c.a * 255.0f)); return p; }
CUTE_INLINE uint32_t CF_Coloro_int_rgb(CF_Color c) { return CF_Coloro_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t CF_Coloro_int_rgba(CF_Color c) { return CF_Coloro_pixel(c).val; }
CUTE_INLINE char* CF_Coloro_string(CF_Color c) { char* s = NULL; return shex(s, CF_Coloro_pixel(c).val); } // Call `sfree` when done.

CUTE_INLINE CF_Color cf_color_invisible() { return cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 0.0f); }
CUTE_INLINE CF_Color cf_color_black() { return cf_make_color_rgb_f(0.0f, 0.0f, 0.0f); }
CUTE_INLINE CF_Color cf_color_white() { return cf_make_color_rgb_f(1.0f, 1.0f, 1.0f); }
CUTE_INLINE CF_Color cf_color_red() { return cf_make_color_rgb_f(1.0f, 0.0f, 0.0f); }
CUTE_INLINE CF_Color cf_color_green() { return cf_make_color_rgb_f(0.0f, 1.0f, 0.0f); }
CUTE_INLINE CF_Color cf_color_blue() { return cf_make_color_rgb_f(0.0f, 0.0f, 1.0f); }
CUTE_INLINE CF_Color cf_color_yellow() { return cf_make_color_rgb_f(1.0f, 1.0f, 0.0f); }
CUTE_INLINE CF_Color cf_color_orange() { return cf_make_color_rgb_f(1.0f, 0.65f, 0.0f); }
CUTE_INLINE CF_Color cf_color_purple() { return cf_make_color_rgb_f(1.0f, 0.0f, 1.0f); }
CUTE_INLINE CF_Color cf_color_grey() { return cf_make_color_rgb_f(0.5f, 0.5f, 0.5f); }

CUTE_INLINE CF_Pixel cf_pixel_invisible() { return cf_make_pixel_hex(0); }
CUTE_INLINE CF_Pixel cf_pixel_black() { return cf_make_pixel_rgb(0, 0, 0); }
CUTE_INLINE CF_Pixel cf_pixel_white() { return cf_make_pixel_rgb(255, 255, 255); }
CUTE_INLINE CF_Pixel cf_pixel_red() { return cf_make_pixel_rgb(255, 0, 0); }
CUTE_INLINE CF_Pixel cf_pixel_green() { return cf_make_pixel_rgb(0, 255, 0); }
CUTE_INLINE CF_Pixel cf_pixel_blue() { return cf_make_pixel_rgb(0, 0, 255); }
CUTE_INLINE CF_Pixel cf_pixel_yellow() { return cf_make_pixel_rgb(255, 255, 0); }
CUTE_INLINE CF_Pixel cf_pixel_orange() { return cf_make_pixel_rgb(255, 165, 0); }
CUTE_INLINE CF_Pixel cf_pixel_purple() { return cf_make_pixel_rgb(255, 0, 255); }
CUTE_INLINE CF_Pixel cf_pixel_grey() { return cf_make_pixel_rgb(127, 127, 127); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Pixel = CF_Pixel;
using Color = CF_Color;

CUTE_INLINE Color make_color(float r, float g, float b) { return cf_make_color_rgb_f(r, g, b); }
CUTE_INLINE Color make_color(float r, float g, float b, float a) { return cf_make_color_rgba_f(r, g, b, a); }
CUTE_INLINE Color make_color(uint8_t r, uint8_t g, uint8_t b) { return cf_make_color_rgb(r, g, b); }
CUTE_INLINE Color make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_color_rgba(r, g, b, a); }
CUTE_INLINE Color make_color(int hex) { return cf_make_color_hex(hex); }
CUTE_INLINE Color make_color(const char* s) { return make_color((int)stohex(s)); }

CUTE_INLINE Pixel make_pixel(float r, float g, float b) { return cf_make_pixel_rgb_f(r, g, b); }
CUTE_INLINE Pixel make_pixel(float r, float g, float b, float a) { return cf_make_pixel_rgba_f(r, g, b, a); }
CUTE_INLINE Pixel make_pixel(uint8_t r, uint8_t g, uint8_t b) { cf_make_pixel_rgb(r, g, b); }
CUTE_INLINE Pixel make_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_pixel_rgba(r, g, b, a); }
CUTE_INLINE Pixel make_pixel(int hex) { return cf_make_pixel_hex(hex); }
CUTE_INLINE Pixel make_pixel(const char* hex) { cf_make_pixel_hex((int)stohex(hex)); }

CUTE_INLINE Color operator*(Color a, float s) { return cf_mul_color(a, s); }
CUTE_INLINE Color operator/(Color a, float s) { return cf_div_color(a, s); }
CUTE_INLINE Color operator+(Color a, Color b) { return cf_add_color(a, b); }
CUTE_INLINE Color operator-(Color a, Color b) { return cf_sub_color(a, b); }
CUTE_INLINE Color lerp(Color a, Color b, float s) { return cf_color_lerp(a, b, s); }
CUTE_INLINE Color premultiply(Color c) { return cf_color_premultiply(c); }

CUTE_INLINE Pixel operator*(Pixel a, float s) { return cf_mul_pixel(a, s); }
CUTE_INLINE Pixel operator/(Pixel a, float s) { return cf_div_pixel(a, s); }
CUTE_INLINE Pixel operator+(Pixel a, Pixel b) { return cf_add_pixel(a, b); }
CUTE_INLINE Pixel operator-(Pixel a, Pixel b) { return cf_sub_pixel(a, b); }
CUTE_INLINE Pixel lerp(Pixel a, Pixel b, float s) { return cf_pixel_lerp(a, b, s); }
CUTE_INLINE Pixel premultiply(Pixel p) { return cf_pixel_premultiply(p); }

CUTE_INLINE Color to_color(Pixel p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t to_int_rgba(Pixel p) { return p.val; }
CUTE_INLINE uint32_t to_int_rgb(Pixel p) { return p.val | 0x000000FF; }
CUTE_INLINE string_t to_string(Pixel p) { char* s = NULL; return shex(s, p.val); }

CUTE_INLINE Pixel to_pixel(Color c) { return CF_Coloro_pixel(c); }
CUTE_INLINE uint32_t to_int_rgb(Color c) { return CF_Coloro_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t to_int_rgba(Color c) { return CF_Coloro_pixel(c).val; }
CUTE_INLINE string_t to_string(Color c) { char* s = NULL; return shex(s, CF_Coloro_pixel(c).val); }

CUTE_INLINE Color color_invisible() { return cf_color_invisible(); }
CUTE_INLINE Color color_black() { return cf_color_black(); }
CUTE_INLINE Color color_white() { return cf_color_white(); }
CUTE_INLINE Color color_red() { return cf_color_red(); }
CUTE_INLINE Color color_green() { return cf_color_green(); }
CUTE_INLINE Color color_blue() { return cf_color_blue(); }
CUTE_INLINE Color color_yellow() { return cf_color_yellow(); }
CUTE_INLINE Color color_orange() { return cf_color_orange(); }
CUTE_INLINE Color color_purple() { return cf_color_purple(); }
CUTE_INLINE Color color_grey() { return cf_color_grey(); }

CUTE_INLINE Pixel pixel_invisible() { return cf_pixel_invisible(); }
CUTE_INLINE Pixel pixel_black() { return cf_pixel_black(); }
CUTE_INLINE Pixel pixel_white() { return cf_pixel_white(); }
CUTE_INLINE Pixel pixel_red() { return cf_pixel_red(); }
CUTE_INLINE Pixel pixel_green() { return cf_pixel_green(); }
CUTE_INLINE Pixel pixel_blue() { return cf_pixel_blue(); }
CUTE_INLINE Pixel pixel_yellow() { return cf_pixel_yellow(); }
CUTE_INLINE Pixel pixel_orange() { return cf_pixel_orange(); }
CUTE_INLINE Pixel pixel_purple() { return cf_pixel_purple(); }
CUTE_INLINE Pixel pixel_grey() { return cf_pixel_grey(); }

}

#endif // CUTE_CPP

#endif // CUTE_COLOR_H
