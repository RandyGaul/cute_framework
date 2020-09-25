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

#include <cute_defines.h>

namespace cute
{

union pixel_t
{
	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} colors;
	uint32_t val;
};

struct color_t
{
	float r;
	float g;
	float b;
	float a;
};

inline color_t make_color(float r, float g, float b)                  { color_t color; color.r = r; color.g = g; color.b = b; color.a = 1.0f; return color; }
inline color_t make_color(float r, float g, float b, float a)         { color_t color; color.r = r; color.g = g; color.b = b; color.a = a; return color; }
inline color_t make_color(uint8_t r, uint8_t g, uint8_t b)            { color_t color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = 1.0f; return color; }
inline color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { color_t color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = (float)a / 255.0f; return color; }
inline color_t make_color(int hex)                                    { return make_color((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }

inline color_t color_black()             { return make_color(0.0f, 0.0f, 0.0f); }
inline color_t color_red()               { return make_color(1.0f, 0.0f, 0.0f); }
inline color_t color_green()               { return make_color(0.0f, 1.0f, 0.0f); }
inline color_t color_blue()               { return make_color(0.0f, 0.0f, 1.0f); }
inline color_t color_white()             { return make_color(1.0f, 1.0f, 1.0f); }
inline color_t color_invisible()         { return make_color(0.0f, 0.0f, 0.0f, 0.0f); }

inline color_t operator*(color_t a, float s) { return make_color(a.r * s, a.g * s, a.b * s, a.a * s); }
inline color_t operator/(color_t a, float s) { return make_color(a.r / s, a.g / s, a.b / s, a.a / s); }
inline color_t operator+(color_t a, color_t b) { return make_color(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a); }
inline color_t operator-(color_t a, color_t b) { return make_color(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a); }

inline color_t lerp(color_t a, color_t b, float s) { return a + (b - a) * s; }

inline pixel_t to_pixel(color_t c)
{
	pixel_t p;
	p.colors.r = (int)((uint8_t)(c.r * 255.0f));
	p.colors.g = (int)((uint8_t)(c.g * 255.0f));
	p.colors.b = (int)((uint8_t)(c.b * 255.0f));
	p.colors.a = (int)((uint8_t)(c.a * 255.0f));
	return p;
}

inline color_t to_color(pixel_t p)
{
	return make_color((int)p.val);
}

}

#endif // CUTE_COLOR_H
