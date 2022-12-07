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
#include "cute_math.h"

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
CUTE_INLINE CF_Color cf_mul_color2(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a); }
CUTE_INLINE CF_Color cf_div_color(CF_Color a, float s) { return cf_make_color_rgba_f(a.r / s, a.g / s, a.b / s, a.a / s); }
CUTE_INLINE CF_Color cf_add_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a); }
CUTE_INLINE CF_Color cf_sub_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a); }
CUTE_INLINE CF_Color cf_abs_color(CF_Color a) { return cf_make_color_rgba_f(cf_abs(a.r), cf_abs(a.g), cf_abs(a.b), cf_abs(a.a)); }
CUTE_INLINE CF_Color cf_fract_color(CF_Color a) { return cf_make_color_rgba_f(cf_fract(a.r), cf_fract(a.g), cf_fract(a.b), cf_fract(a.a)); }
CUTE_INLINE CF_Color cf_splat_color(float v) { CF_Color color; color.r = v; color.g = v; color.b = v; color.a = v; return color; }
CUTE_INLINE CF_Color cf_mod_color(CF_Color a, float m) { return cf_make_color_rgba_f(cf_mod(a.r, m), cf_mod(a.g, m), cf_mod(a.b, m), cf_mod(a.a, m)); }
CUTE_INLINE CF_Color cf_clamp_color(CF_Color a, CF_Color lo, CF_Color hi) { return cf_make_color_rgba_f(cf_clamp(a.r, lo.r, hi.r), cf_clamp(a.g, lo.g, hi.g), cf_clamp(a.b, lo.b, hi.b), cf_clamp(a.a, lo.a, hi.a)); }
CUTE_INLINE CF_Color cf_clamp_color01(CF_Color a) { return cf_make_color_rgba_f(cf_clamp(a.r, 0, 1.0f), cf_clamp(a.g, 0, 1.0f), cf_clamp(a.b, 0, 1.0f), cf_clamp(a.a, 0, 1.0f)); }
CUTE_INLINE CF_Color cf_color_lerp(CF_Color a, CF_Color b, float s) { return cf_add_color(a, cf_mul_color(cf_sub_color(b, a), s)); }
CUTE_INLINE CF_Color cf_color_premultiply(CF_Color c) { c.r *= c.a; c.g *= c.a; c.b *= c.a; return c; }

// HSV <-> RGB from : http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// And https://www.shadertoy.com/view/MsS3Wc

CUTE_INLINE CF_Color cf_rgb_to_hsv(CF_Color c)
{
	CF_Color K = cf_make_color_rgba_f(0, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
	CF_Color p = c.g < c.b ? cf_make_color_rgba_f(c.b, c.g, K.a, K.b) : cf_make_color_rgba_f(c.g, c.b, K.r, K.g);
	CF_Color q = c.r < p.r ? cf_make_color_rgba_f(p.r, p.g, p.a, c.r) : cf_make_color_rgba_f(c.r, p.g, p.b, p.r);
	float d = q.r - cf_min(q.a, q.g);
	float e = 1.0e-10f;
	return cf_make_color_rgba_f(cf_abs(q.b + (q.a - q.g) / (6.0f * d + e)), d / (q.r + e), q.r, c.a);
}

CUTE_INLINE CF_Color cf_hsv_to_rgb(CF_Color c)
{
	float alpha = c.a;
	CF_Color one = cf_splat_color(1.0f);
	CF_Color three = cf_splat_color(3.0f);
	CF_Color x = cf_add_color(cf_splat_color(c.r*6.0f), cf_make_color_rgb_f(0,4.0f,2.0f));
	CF_Color rgb = cf_clamp_color01(cf_sub_color(cf_abs_color(cf_sub_color(cf_mod_color(x,6.0f), three)), one));
	rgb = cf_mul_color2(cf_mul_color2(cf_sub_color(three, cf_mul_color(rgb, 2.0f)), rgb), rgb);
	CF_Color result = cf_mul_color(cf_color_lerp(one, rgb, c.g), c.b);
	result.a = alpha;
	return result;
}

CUTE_INLINE CF_Color cf_tint(CF_Color base, CF_Color tint)
{
	float alpha = base.a;
	base = cf_rgb_to_hsv(base);
	tint = cf_rgb_to_hsv(tint);
	return cf_hsv_to_rgb(cf_make_color_rgba_f(tint.r, base.g, base.b, alpha));
}

CUTE_INLINE uint8_t cf_mul_un8(int a, int b) { int t = (a * b) + 0x80; return (uint8_t)(((t >> 8) + t) >> 8); }
CUTE_INLINE uint8_t cf_div_un8(int a, int b) { return (uint8_t)(((b) * 0xFF + ((a) / 2)) / (a)); }
CUTE_INLINE uint8_t cf_add_un8(int a, int b) { int t = a + b; return (uint8_t)(t | (t >> 8)); }
CUTE_INLINE uint8_t cf_sub_un8(int a, int b) { int t = a - b; if (t < 0) t = 0; return (uint8_t)(t | (t >> 8)); }
CUTE_INLINE CF_Pixel cf_mul_pixel(CF_Pixel a, uint8_t s) { return cf_make_pixel_rgba_f(cf_mul_un8(a.colors.r, s), cf_mul_un8(a.colors.g, s), cf_mul_un8(a.colors.b, s), cf_mul_un8(a.colors.a, s)); }
CUTE_INLINE CF_Pixel cf_div_pixel(CF_Pixel a, uint8_t s) { return cf_make_pixel_rgba_f(cf_div_un8(a.colors.r, s), cf_div_un8(a.colors.g, s), cf_div_un8(a.colors.b, s), cf_div_un8(a.colors.a, s)); }
CUTE_INLINE CF_Pixel cf_add_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(cf_add_un8(a.colors.r, b.colors.r), cf_add_un8(a.colors.g, b.colors.g), cf_add_un8(a.colors.b, b.colors.b), cf_add_un8(a.colors.a, b.colors.a)); }
CUTE_INLINE CF_Pixel cf_sub_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(cf_sub_un8(a.colors.r, b.colors.r), cf_sub_un8(a.colors.g, b.colors.g), cf_sub_un8(a.colors.b, b.colors.b), cf_sub_un8(a.colors.a, b.colors.a)); }
CUTE_INLINE CF_Pixel cf_pixel_lerp(CF_Pixel a, CF_Pixel b, uint8_t s) { return cf_add_pixel(a, cf_mul_pixel(cf_sub_pixel(b, a), s)); }
CUTE_INLINE CF_Pixel cf_pixel_premultiply(CF_Pixel c) { c.colors.r = cf_mul_un8(c.colors.r, c.colors.a); c.colors.g = cf_mul_un8(c.colors.g, c.colors.a); c.colors.b = cf_mul_un8(c.colors.b, c.colors.a); return c; }

CUTE_INLINE CF_Color cf_pixel_to_color(CF_Pixel p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t cf_pixel_to_int_rgba(CF_Pixel p) { return p.val; }
CUTE_INLINE uint32_t cf_pixel_to_int_rgb(CF_Pixel p) { return p.val | 0x000000FF; }
CUTE_INLINE char* cf_pixel_to_string(CF_Pixel p) { char* s = NULL; return shex(s, p.val); } // Call `sfree` when done.

CUTE_INLINE CF_Pixel cf_color_to_pixel(CF_Color c) { CF_Pixel p; p.colors.r = (int)((uint8_t)(c.r * 255.0f)); p.colors.g = (int)((uint8_t)(c.g * 255.0f)); p.colors.b = (int)((uint8_t)(c.b * 255.0f)); p.colors.a = (int)((uint8_t)(c.a * 255.0f)); return p; }
CUTE_INLINE uint32_t cf_color_to_int_rgb(CF_Color c) { return cf_color_to_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t cf_color_to_int_rgba(CF_Color c) { return cf_color_to_pixel(c).val; }
CUTE_INLINE char* cf_color_to_string(CF_Color c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); } // Call `sfree` when done.

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

namespace Cute
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
CUTE_INLINE Color operator*(Color a, Color b) { CF_Color c; c.r = a.r * b.r; c.g = a.g * b.g; c.b = a.b * b.b; c.a = a.a * b.a; return c; }
CUTE_INLINE bool operator==(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
CUTE_INLINE bool operator!=(Color a, Color b) { return !(a == b); }
CUTE_INLINE CF_Color abs_color(CF_Color a) { return cf_abs_color(a); }
CUTE_INLINE CF_Color fract_color(CF_Color a) { return cf_fract_color(a); }
CUTE_INLINE CF_Color splat_color(float v) { return cf_splat_color(v); }
CUTE_INLINE CF_Color clamp_color(CF_Color a, CF_Color lo, CF_Color hi) { return cf_clamp_color(a, lo, hi); }
CUTE_INLINE CF_Color clamp_color01(CF_Color a) { return cf_clamp_color01(a); }
CUTE_INLINE Color lerp(Color a, Color b, float s) { return cf_color_lerp(a, b, s); }
CUTE_INLINE Color premultiply(Color c) { return cf_color_premultiply(c); }
CUTE_INLINE CF_Color rgb_to_hsv(CF_Color c) { return cf_rgb_to_hsv(c); }
CUTE_INLINE CF_Color hsv_to_rgb(CF_Color c) { return cf_hsv_to_rgb(c); }
CUTE_INLINE CF_Color tint(CF_Color base, CF_Color tint) { return cf_tint(base, tint); }

CUTE_INLINE Pixel operator*(Pixel a, int s) { return cf_mul_pixel(a, s); }
CUTE_INLINE Pixel operator/(Pixel a, int s) { return cf_div_pixel(a, s); }
CUTE_INLINE Pixel operator+(Pixel a, Pixel b) { return cf_add_pixel(a, b); }
CUTE_INLINE Pixel operator-(Pixel a, Pixel b) { return cf_sub_pixel(a, b); }
CUTE_INLINE bool operator==(Pixel a, Pixel b) { return a.val == b.val; }
CUTE_INLINE bool operator!=(Pixel a, Pixel b) { return a.val != b.val; }
CUTE_INLINE Pixel lerp(Pixel a, Pixel b, uint8_t s) { return cf_pixel_lerp(a, b, s); }
CUTE_INLINE Pixel premultiply(Pixel p) { return cf_pixel_premultiply(p); }

CUTE_INLINE Color to_color(Pixel p) { return cf_make_color_hex((int)p.val); }
CUTE_INLINE uint32_t to_int_rgba(Pixel p) { return p.val; }
CUTE_INLINE uint32_t to_int_rgb(Pixel p) { return p.val | 0x000000FF; }
CUTE_INLINE String to_string(Pixel p) { char* s = NULL; return shex(s, p.val); }

CUTE_INLINE Pixel to_pixel(Color c) { return cf_color_to_pixel(c); }
CUTE_INLINE uint32_t to_int_rgb(Color c) { return cf_color_to_pixel(c).val | 0x000000FF; }
CUTE_INLINE uint32_t to_int_rgba(Color c) { return cf_color_to_pixel(c).val; }
CUTE_INLINE String to_string(Color c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); }

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
