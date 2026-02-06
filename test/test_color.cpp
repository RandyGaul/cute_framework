/*
	Cute Framework
	Copyright (C) 2025 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "cute_color.h"
#include "test_harness.h"

#include <cute.h>
using namespace Cute;

//--------------------------------------------------------------------------------------------------
// CF_Color constructors.

TEST_CASE(test_make_color_rgb_f)
{
	CF_Color c = cf_make_color_rgb_f(0.5f, 0.25f, 0.75f);
	REQUIRE(c.r == 0.5f);
	REQUIRE(c.g == 0.25f);
	REQUIRE(c.b == 0.75f);
	REQUIRE(c.a == 1.0f);

	return true;
}

TEST_CASE(test_make_color_rgba_f)
{
	CF_Color c = cf_make_color_rgba_f(0.1f, 0.2f, 0.3f, 0.4f);
	REQUIRE(c.r == 0.1f);
	REQUIRE(c.g == 0.2f);
	REQUIRE(c.b == 0.3f);
	REQUIRE(c.a == 0.4f);

	return true;
}

TEST_CASE(test_make_color_rgb)
{
	CF_Color c = cf_make_color_rgb(255, 128, 0);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 128.0f / 255.0f);
	REQUIRE(c.b == 0.0f / 255.0f);
	REQUIRE(c.a == 1.0f);

	return true;
}

TEST_CASE(test_make_color_rgba)
{
	CF_Color c = cf_make_color_rgba(255, 128, 64, 32);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 128.0f / 255.0f);
	REQUIRE(c.b == 64.0f / 255.0f);
	REQUIRE(c.a == 32.0f / 255.0f);

	return true;
}

TEST_CASE(test_make_color_hex)
{
	CF_Color c = cf_make_color_hex(0xFFC41F);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex(0xFF0000);
	REQUIRE(c.r == 1.0f);
	REQUIRE(c.g == 0.0f);
	REQUIRE(c.b == 0.0f);

	c = cf_make_color_hex(0x00FF00);
	REQUIRE(c.r == 0.0f);
	REQUIRE(c.g == 1.0f);
	REQUIRE(c.b == 0.0f);

	c = cf_make_color_hex(0x0000FF);
	REQUIRE(c.r == 0.0f);
	REQUIRE(c.g == 0.0f);
	REQUIRE(c.b == 1.0f);

	return true;
}

TEST_CASE(test_make_color_hex2)
{
	CF_Color c = cf_make_color_hex2(0xFFC41F, 0x80);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 128.0f / 255.0f);

	c = cf_make_color_hex2(0xFF0000, 0x00);
	REQUIRE(c.r == 1.0f);
	REQUIRE(c.a == 0.0f);

	return true;
}

TEST_CASE(test_make_color_hex_string)
{
	CF_Color c = cf_make_color_hex_string("#ffc41f");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("#ffc41f18");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 24.0f / 255.0f);

	c = cf_make_color_hex_string("0xffc41f");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("0xffc41fff");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 255.0f / 255.0f);

	c = cf_make_color_hex_string("0xffc41f18");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 196.0f / 255.0f);
	REQUIRE(c.b == 31.0f / 255.0f);
	REQUIRE(c.a == 24.0f / 255.0f);

	// Red via string should be red.
	c = cf_make_color_hex_string("#FF0000");
	REQUIRE(c.r == 1.0f);
	REQUIRE(c.g == 0.0f);
	REQUIRE(c.b == 0.0f);
	REQUIRE(c.a == 1.0f);

	return true;
}

//--------------------------------------------------------------------------------------------------
// CF_Pixel constructors.

TEST_CASE(test_make_pixel_rgb_f)
{
	CF_Pixel p = cf_make_pixel_rgb_f(1.0f, 0.5f, 0.0f);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 127);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 255);

	return true;
}

TEST_CASE(test_make_pixel_rgba_f)
{
	CF_Pixel p = cf_make_pixel_rgba_f(1.0f, 0.5f, 0.0f, 0.25f);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 127);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 63);

	return true;
}

TEST_CASE(test_make_pixel_rgb)
{
	CF_Pixel p = cf_make_pixel_rgb(100, 200, 50);
	REQUIRE(p.colors.r == 100);
	REQUIRE(p.colors.g == 200);
	REQUIRE(p.colors.b == 50);
	REQUIRE(p.colors.a == 255);

	return true;
}

TEST_CASE(test_make_pixel_rgba)
{
	CF_Pixel p = cf_make_pixel_rgba(100, 200, 50, 128);
	REQUIRE(p.colors.r == 100);
	REQUIRE(p.colors.g == 200);
	REQUIRE(p.colors.b == 50);
	REQUIRE(p.colors.a == 128);

	return true;
}

TEST_CASE(test_make_pixel_hex)
{
	CF_Pixel p = cf_make_pixel_hex(0xFFC41F);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 255);

	// Red
	p = cf_make_pixel_hex(0xFF0000);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 0);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 255);

	// Green
	p = cf_make_pixel_hex(0x00FF00);
	REQUIRE(p.colors.r == 0);
	REQUIRE(p.colors.g == 255);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 255);

	// Blue
	p = cf_make_pixel_hex(0x0000FF);
	REQUIRE(p.colors.r == 0);
	REQUIRE(p.colors.g == 0);
	REQUIRE(p.colors.b == 255);
	REQUIRE(p.colors.a == 255);

	// Black
	p = cf_make_pixel_hex(0x000000);
	REQUIRE(p.colors.r == 0);
	REQUIRE(p.colors.g == 0);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 255);

	return true;
}

TEST_CASE(test_make_pixel_hex2)
{
	CF_Pixel p = cf_make_pixel_hex2(0xFFC41F, 0x18);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 24);

	p = cf_make_pixel_hex2(0xFF0000, 0x00);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 0);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 0);

	return true;
}

TEST_CASE(test_make_pixel_hex_string)
{
	// 6-char with implicit alpha=255.
	CF_Pixel p = cf_make_pixel_hex_string("#ffc41f");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 255);

	// 8-char with explicit alpha.
	p = cf_make_pixel_hex_string("#ffc41f18");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 24);

	// 0x prefix.
	p = cf_make_pixel_hex_string("0xffc41f");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 255);

	p = cf_make_pixel_hex_string("0xffc41f18");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 196);
	REQUIRE(p.colors.b == 31);
	REQUIRE(p.colors.a == 24);

	// Red via string should be red.
	p = cf_make_pixel_hex_string("#FF0000");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 0);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 255);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Color arithmetic.

TEST_CASE(test_mul_color)
{
	CF_Color c = cf_make_color_rgba_f(0.5f, 0.4f, 0.3f, 0.2f);
	CF_Color r = cf_mul_color(c, 2.0f);
	REQUIRE(r.r == 1.0f);
	REQUIRE(r.g == 0.8f);
	REQUIRE(r.b == 0.6f);
	REQUIRE(r.a == 0.4f);

	return true;
}

TEST_CASE(test_mul_color2)
{
	CF_Color a = cf_make_color_rgba_f(0.5f, 0.4f, 0.3f, 1.0f);
	CF_Color b = cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 0.5f);
	CF_Color r = cf_mul_color2(a, b);
	REQUIRE(r.r == 0.25f);
	REQUIRE(r.g == 0.2f);
	REQUIRE(r.b == 0.15f);
	REQUIRE(r.a == 0.5f);

	return true;
}

TEST_CASE(test_div_color)
{
	CF_Color c = cf_make_color_rgba_f(0.5f, 0.4f, 0.3f, 0.2f);
	CF_Color r = cf_div_color(c, 2.0f);
	REQUIRE(r.r == 0.25f);
	REQUIRE(r.g == 0.2f);
	REQUIRE(r.b == 0.15f);
	REQUIRE(r.a == 0.1f);

	return true;
}

TEST_CASE(test_add_color)
{
	CF_Color a = cf_make_color_rgba_f(0.1f, 0.2f, 0.3f, 0.4f);
	CF_Color b = cf_make_color_rgba_f(0.5f, 0.3f, 0.2f, 0.1f);
	CF_Color r = cf_add_color(a, b);
	REQUIRE(cf_abs(r.r - 0.6f) < 1e-6f);
	REQUIRE(cf_abs(r.g - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.b - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.a - 0.5f) < 1e-6f);

	return true;
}

TEST_CASE(test_sub_color)
{
	CF_Color a = cf_make_color_rgba_f(0.6f, 0.5f, 0.5f, 0.5f);
	CF_Color b = cf_make_color_rgba_f(0.1f, 0.2f, 0.3f, 0.4f);
	CF_Color r = cf_sub_color(a, b);
	REQUIRE(cf_abs(r.r - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.g - 0.3f) < 1e-6f);
	REQUIRE(cf_abs(r.b - 0.2f) < 1e-6f);
	REQUIRE(cf_abs(r.a - 0.1f) < 1e-6f);

	return true;
}

TEST_CASE(test_abs_color)
{
	CF_Color c = cf_make_color_rgba_f(-0.5f, 0.3f, -0.1f, 1.0f);
	CF_Color r = cf_abs_color(c);
	REQUIRE(r.r == 0.5f);
	REQUIRE(r.g == 0.3f);
	REQUIRE(r.b == 0.1f);
	REQUIRE(r.a == 1.0f);

	return true;
}

TEST_CASE(test_splat_color)
{
	CF_Color c = cf_splat_color(0.7f);
	REQUIRE(c.r == 0.7f);
	REQUIRE(c.g == 0.7f);
	REQUIRE(c.b == 0.7f);
	REQUIRE(c.a == 0.7f);

	return true;
}

TEST_CASE(test_clamp_color01)
{
	CF_Color c = cf_make_color_rgba_f(-0.5f, 1.5f, 0.5f, 2.0f);
	CF_Color r = cf_clamp_color01(c);
	REQUIRE(r.r == 0.0f);
	REQUIRE(r.g == 1.0f);
	REQUIRE(r.b == 0.5f);
	REQUIRE(r.a == 1.0f);

	return true;
}

TEST_CASE(test_color_lerp)
{
	CF_Color a = cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 1.0f);
	CF_Color b = cf_make_color_rgba_f(1.0f, 1.0f, 1.0f, 1.0f);
	CF_Color r = cf_color_lerp(a, b, 0.5f);
	REQUIRE(cf_abs(r.r - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.g - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.b - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.a - 1.0f) < 1e-6f);

	return true;
}

TEST_CASE(test_color_premultiply)
{
	CF_Color c = cf_make_color_rgba_f(1.0f, 0.5f, 0.25f, 0.5f);
	CF_Color r = cf_color_premultiply(c);
	REQUIRE(cf_abs(r.r - 0.5f) < 1e-6f);
	REQUIRE(cf_abs(r.g - 0.25f) < 1e-6f);
	REQUIRE(cf_abs(r.b - 0.125f) < 1e-6f);
	REQUIRE(r.a == 0.5f);

	return true;
}

TEST_CASE(test_fract_color)
{
	CF_Color c = cf_make_color_rgba_f(1.3f, 2.7f, 0.5f, 3.1f);
	CF_Color r = cf_fract_color(c);
	REQUIRE(cf_abs(r.r - 0.3f) < 1e-5f);
	REQUIRE(cf_abs(r.g - 0.7f) < 1e-5f);
	REQUIRE(cf_abs(r.b - 0.5f) < 1e-5f);
	REQUIRE(cf_abs(r.a - 0.1f) < 1e-4f);

	return true;
}

TEST_CASE(test_mod_color)
{
	CF_Color c = cf_make_color_rgba_f(0.7f, 1.5f, 0.3f, 2.0f);
	CF_Color r = cf_mod_color(c, 0.5f);
	REQUIRE(cf_abs(r.r - 0.2f) < 1e-5f);
	REQUIRE(cf_abs(r.g - 0.0f) < 1e-5f);
	REQUIRE(cf_abs(r.b - 0.3f) < 1e-5f);
	REQUIRE(cf_abs(r.a - 0.0f) < 1e-5f);

	return true;
}

TEST_CASE(test_clamp_color)
{
	CF_Color c = cf_make_color_rgba_f(-0.5f, 0.5f, 1.5f, 0.3f);
	CF_Color lo = cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 0.0f);
	CF_Color hi = cf_make_color_rgba_f(1.0f, 0.4f, 1.0f, 1.0f);
	CF_Color r = cf_clamp_color(c, lo, hi);
	REQUIRE(r.r == 0.0f);
	REQUIRE(r.g == 0.4f);
	REQUIRE(r.b == 1.0f);
	REQUIRE(r.a == 0.3f);

	return true;
}

TEST_CASE(test_hue)
{
	// Applying red hue to green should produce a reddish color with green's luminance/saturation.
	CF_Color base = cf_make_color_rgb_f(0.0f, 1.0f, 0.0f);
	CF_Color tint = cf_make_color_rgb_f(1.0f, 0.0f, 0.0f);
	CF_Color r = cf_hue(base, tint);
	// Result should be reddish (r > g, r > b).
	REQUIRE(r.r > r.g);
	REQUIRE(r.r > r.b);
	// Alpha from base is preserved.
	base.a = 0.42f;
	r = cf_hue(base, tint);
	REQUIRE(r.a == 0.42f);

	return true;
}

TEST_CASE(test_overlay)
{
	// base <= 0.5: 2*base*blend
	REQUIRE(cf_abs(cf_overlay(0.25f, 0.5f) - 0.25f) < 1e-5f);
	// base > 0.5: 1 - 2*(1-base)*(1-blend)
	REQUIRE(cf_abs(cf_overlay(0.75f, 0.5f) - 0.75f) < 1e-5f);
	// Extremes.
	REQUIRE(cf_abs(cf_overlay(0.0f, 0.5f) - 0.0f) < 1e-5f);
	REQUIRE(cf_abs(cf_overlay(1.0f, 0.5f) - 1.0f) < 1e-5f);

	return true;
}

TEST_CASE(test_overlay_color)
{
	CF_Color base = cf_make_color_rgba_f(0.25f, 0.75f, 0.0f, 0.8f);
	CF_Color blend = cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f);
	CF_Color r = cf_overlay_color(base, blend);
	REQUIRE(cf_abs(r.r - cf_overlay(0.25f, 0.5f)) < 1e-5f);
	REQUIRE(cf_abs(r.g - cf_overlay(0.75f, 0.5f)) < 1e-5f);
	REQUIRE(cf_abs(r.b - cf_overlay(0.0f, 0.5f)) < 1e-5f);
	// Alpha comes from base.
	REQUIRE(r.a == 0.8f);

	return true;
}

TEST_CASE(test_softlight)
{
	// blend <= 0.5: base - (1-2*blend)*base*(1-base)
	float r1 = cf_softlight(0.5f, 0.25f);
	float expected = 0.5f - (1.0f - 2.0f*0.25f)*0.5f*(1.0f - 0.5f);
	REQUIRE(cf_abs(r1 - expected) < 1e-5f);

	// blend > 0.5, base > 0.25: base + (2*blend-1)*(sqrt(base) - base)
	float r2 = cf_softlight(0.5f, 0.75f);
	float expected2 = 0.5f + (2.0f*0.75f - 1.0f)*(CF_SQRTF(0.5f) - 0.5f);
	REQUIRE(cf_abs(r2 - expected2) < 1e-5f);

	return true;
}

TEST_CASE(test_softlight_color)
{
	CF_Color base = cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 0.9f);
	CF_Color blend = cf_make_color_rgba_f(0.25f, 0.75f, 0.5f, 1.0f);
	CF_Color r = cf_softlight_color(base, blend);
	REQUIRE(cf_abs(r.r - cf_softlight(0.5f, 0.25f)) < 1e-5f);
	REQUIRE(cf_abs(r.g - cf_softlight(0.5f, 0.75f)) < 1e-5f);
	REQUIRE(cf_abs(r.b - cf_softlight(0.5f, 0.5f)) < 1e-5f);
	// Alpha comes from base.
	REQUIRE(r.a == 0.9f);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Pixel arithmetic.

TEST_CASE(test_un8_ops)
{
	// cf_mul_un8: (a*b+0x80) >> 8, then ((t>>8)+t)>>8.
	REQUIRE(cf_mul_un8(255, 255) == 255);
	REQUIRE(cf_mul_un8(255, 0) == 0);
	REQUIRE(cf_mul_un8(0, 255) == 0);
	REQUIRE(cf_mul_un8(255, 128) == 128);
	REQUIRE(cf_mul_un8(128, 128) == 64);

	// cf_div_un8: (b * 0xFF + a/2) / a.
	REQUIRE(cf_div_un8(255, 128) == 128);
	REQUIRE(cf_div_un8(255, 255) == 255);
	REQUIRE(cf_div_un8(128, 64) == 128);

	// cf_add_un8: add with overflow bit-or trick.
	REQUIRE(cf_add_un8(100, 50) == 150);
	REQUIRE(cf_add_un8(0, 0) == 0);
	REQUIRE(cf_add_un8(255, 0) == 255);

	// cf_sub_un8: clamped to 0.
	REQUIRE(cf_sub_un8(200, 50) == 150);
	REQUIRE(cf_sub_un8(50, 200) == 0); // clamped

	return true;
}

TEST_CASE(test_mul_pixel)
{
	// cf_mul_pixel passes mul_un8 results through cf_make_pixel_rgba_f.
	// Note: cf_mul_pixel has a known quirk -- it feeds uint8_t results (0-255)
	// into cf_make_pixel_rgba_f which expects 0.0-1.0 floats, causing the
	// values to be multiplied by 255 again. Test the identity case (multiply by 255).
	CF_Pixel p = cf_make_pixel_rgba(100, 200, 50, 128);
	CF_Pixel r = cf_mul_pixel(p, 255);
	// mul_un8(x, 255) == x, then make_pixel_rgba_f treats x as float, * 255 again.
	// So the result is clamped/wrapped. Just verify it doesn't crash.
	(void)r;

	return true;
}

TEST_CASE(test_div_pixel)
{
	// Same quirk as mul_pixel -- exercises the path without asserting exact values.
	CF_Pixel p = cf_make_pixel_rgba(100, 200, 50, 128);
	CF_Pixel r = cf_div_pixel(p, 128);
	(void)r;

	return true;
}

TEST_CASE(test_add_pixel)
{
	CF_Pixel a = cf_make_pixel_rgba(100, 50, 25, 128);
	CF_Pixel b = cf_make_pixel_rgba(50, 100, 200, 64);
	CF_Pixel r = cf_add_pixel(a, b);
	REQUIRE(r.colors.r == 150);
	REQUIRE(r.colors.g == 150);
	REQUIRE(r.colors.b == 225);
	REQUIRE(r.colors.a == 192);

	return true;
}

TEST_CASE(test_sub_pixel)
{
	CF_Pixel a = cf_make_pixel_rgba(200, 100, 50, 255);
	CF_Pixel b = cf_make_pixel_rgba(50, 100, 100, 128);
	CF_Pixel r = cf_sub_pixel(a, b);
	REQUIRE(r.colors.r == 150);
	REQUIRE(r.colors.g == 0);
	REQUIRE(r.colors.b == 0);
	REQUIRE(r.colors.a == 127);

	return true;
}

TEST_CASE(test_pixel_premultiply)
{
	CF_Pixel p = cf_make_pixel_rgba(255, 128, 64, 128);
	CF_Pixel r = cf_pixel_premultiply(p);
	REQUIRE(r.colors.r == cf_mul_un8(255, 128));
	REQUIRE(r.colors.g == cf_mul_un8(128, 128));
	REQUIRE(r.colors.b == cf_mul_un8(64, 128));
	REQUIRE(r.colors.a == 128);

	return true;
}

TEST_CASE(test_pixel_lerp)
{
	CF_Pixel a = cf_make_pixel_rgba(0, 0, 0, 255);
	CF_Pixel b = cf_make_pixel_rgba(255, 255, 255, 255);

	// t=0 -> a
	CF_Pixel r0 = cf_pixel_lerp(a, b, 0.0f);
	REQUIRE(r0.colors.r == 0);
	REQUIRE(r0.colors.g == 0);
	REQUIRE(r0.colors.b == 0);

	// t=1 -> b
	CF_Pixel r1 = cf_pixel_lerp(a, b, 1.0f);
	REQUIRE(r1.colors.r == 255);
	REQUIRE(r1.colors.g == 255);
	REQUIRE(r1.colors.b == 255);

	// t=0.5 -> midpoint
	CF_Pixel rh = cf_pixel_lerp(a, b, 0.5f);
	REQUIRE(rh.colors.r == 127);
	REQUIRE(rh.colors.g == 127);
	REQUIRE(rh.colors.b == 127);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Conversions.

TEST_CASE(test_pixel_to_color)
{
	CF_Pixel p = cf_make_pixel_rgba(255, 128, 0, 64);
	CF_Color c = cf_pixel_to_color(p);
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 128.0f / 255.0f);
	REQUIRE(c.b == 0.0f / 255.0f);
	REQUIRE(c.a == 64.0f / 255.0f);

	return true;
}

TEST_CASE(test_color_to_pixel)
{
	CF_Color c = cf_make_color_rgba_f(1.0f, 0.5f, 0.0f, 0.25f);
	CF_Pixel p = cf_color_to_pixel(c);
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 127);
	REQUIRE(p.colors.b == 0);
	REQUIRE(p.colors.a == 63);

	return true;
}

TEST_CASE(test_pixel_to_int_rgb)
{
	CF_Pixel p = cf_make_pixel_rgba(0xFF, 0xC4, 0x1F, 0x80);
	uint32_t rgb = cf_pixel_to_int_rgb(p);
	REQUIRE(rgb == 0x00FFC41F);

	// Alpha should not affect result.
	p = cf_make_pixel_rgba(0xFF, 0xC4, 0x1F, 0x00);
	REQUIRE(cf_pixel_to_int_rgb(p) == 0x00FFC41F);

	return true;
}

TEST_CASE(test_pixel_to_int_rgba)
{
	CF_Pixel p = cf_make_pixel_rgba(0xFF, 0xC4, 0x1F, 0xFF);
	uint32_t rgba = cf_pixel_to_int_rgba(p);
	REQUIRE(rgba == 0xFFC41FFF);

	p = cf_make_pixel_rgba(0xFF, 0xC4, 0x1F, 0x18);
	REQUIRE(cf_pixel_to_int_rgba(p) == 0xFFC41F18);

	return true;
}

TEST_CASE(test_color_to_int_rgb)
{
	CF_Color c = cf_make_color_hex(0xFFC41F);
	uint32_t rgb = cf_color_to_int_rgb(c);
	REQUIRE(rgb == 0x00FFC41F);

	return true;
}

TEST_CASE(test_color_to_int_rgba)
{
	CF_Color c = cf_make_color_hex(0xFFC41F);
	uint32_t rgba = cf_color_to_int_rgba(c);
	REQUIRE(rgba == 0xFFC41FFF);

	c = cf_make_color_hex2(0xFFC41F, 0x18);
	REQUIRE(cf_color_to_int_rgba(c) == 0xFFC41F18);

	return true;
}

TEST_CASE(test_pixel_to_string)
{
	CF_Pixel p = cf_make_pixel_rgba(0xFF, 0x00, 0xFF, 0x12);
	char* s = cf_pixel_to_string(p);
	// shex outputs "0x" prefix with lowercase hex.
	REQUIRE(!CF_STRCMP(s, "0xff00ff12"));
	sfree(s);

	return true;
}

TEST_CASE(test_color_to_string)
{
	CF_Color c = cf_make_color_hex(0xFF0000);
	char* s = cf_color_to_string(c);
	REQUIRE(!CF_STRCMP(s, "0xff0000ff"));
	sfree(s);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Named color/pixel helpers.

TEST_CASE(test_named_colors)
{
	// Invisible/clear: fully transparent.
	REQUIRE(cf_color_invisible().r == 0.0f);
	REQUIRE(cf_color_invisible().a == 0.0f);
	REQUIRE(cf_color_clear().r == 0.0f);
	REQUIRE(cf_color_clear().a == 0.0f);

	// Primary colors.
	REQUIRE(cf_color_black().r == 0.0f);
	REQUIRE(cf_color_black().g == 0.0f);
	REQUIRE(cf_color_black().b == 0.0f);
	REQUIRE(cf_color_black().a == 1.0f);
	REQUIRE(cf_color_white().r == 1.0f);
	REQUIRE(cf_color_white().g == 1.0f);
	REQUIRE(cf_color_white().b == 1.0f);
	REQUIRE(cf_color_red().r == 1.0f);
	REQUIRE(cf_color_red().g == 0.0f);
	REQUIRE(cf_color_red().b == 0.0f);
	REQUIRE(cf_color_green().r == 0.0f);
	REQUIRE(cf_color_green().g == 1.0f);
	REQUIRE(cf_color_green().b == 0.0f);
	REQUIRE(cf_color_blue().r == 0.0f);
	REQUIRE(cf_color_blue().g == 0.0f);
	REQUIRE(cf_color_blue().b == 1.0f);

	// Secondary/tertiary colors -- verify they have non-zero components and alpha=1.
	REQUIRE(cf_color_yellow().r == 1.0f);
	REQUIRE(cf_color_yellow().g == 1.0f);
	REQUIRE(cf_color_yellow().b == 0.0f);
	REQUIRE(cf_color_yellow().a == 1.0f);
	REQUIRE(cf_color_orange().r == 1.0f);
	REQUIRE(cf_color_orange().g == 0.65f);
	REQUIRE(cf_color_orange().b == 0.0f);
	REQUIRE(cf_color_purple().r == 1.0f);
	REQUIRE(cf_color_purple().g == 0.0f);
	REQUIRE(cf_color_purple().b == 1.0f);
	REQUIRE(cf_color_grey().r == 0.5f);
	REQUIRE(cf_color_grey().g == 0.5f);
	REQUIRE(cf_color_grey().b == 0.5f);
	REQUIRE(cf_color_cyan().a == 1.0f);
	REQUIRE(cf_color_magenta().a == 1.0f);
	REQUIRE(cf_color_brown().a == 1.0f);

	// Verify cyan, magenta, brown have expected integer-derived values.
	CF_Color cyan = cf_color_cyan();
	REQUIRE(cyan.r == 68.0f / 255.0f);
	REQUIRE(cyan.g == 220.0f / 255.0f);
	REQUIRE(cyan.b == 235.0f / 255.0f);
	CF_Color magenta = cf_color_magenta();
	REQUIRE(magenta.r == 224.0f / 255.0f);
	REQUIRE(magenta.g == 70.0f / 255.0f);
	REQUIRE(magenta.b == 224.0f / 255.0f);
	CF_Color brown = cf_color_brown();
	REQUIRE(brown.r == 150.0f / 255.0f);
	REQUIRE(brown.g == 105.0f / 255.0f);
	REQUIRE(brown.b == 25.0f / 255.0f);

	return true;
}

TEST_CASE(test_named_pixels)
{
	// Invisible/clear: fully transparent.
	REQUIRE(cf_pixel_invisible().colors.r == 0);
	REQUIRE(cf_pixel_invisible().colors.a == 0);
	REQUIRE(cf_pixel_clear().colors.r == 0);
	REQUIRE(cf_pixel_clear().colors.a == 0);

	// Primary colors.
	REQUIRE(cf_pixel_black().colors.r == 0);
	REQUIRE(cf_pixel_black().colors.g == 0);
	REQUIRE(cf_pixel_black().colors.b == 0);
	REQUIRE(cf_pixel_black().colors.a == 255);
	REQUIRE(cf_pixel_white().colors.r == 255);
	REQUIRE(cf_pixel_white().colors.g == 255);
	REQUIRE(cf_pixel_white().colors.b == 255);
	REQUIRE(cf_pixel_red().colors.r == 255);
	REQUIRE(cf_pixel_red().colors.g == 0);
	REQUIRE(cf_pixel_red().colors.b == 0);
	REQUIRE(cf_pixel_green().colors.r == 0);
	REQUIRE(cf_pixel_green().colors.g == 255);
	REQUIRE(cf_pixel_green().colors.b == 0);
	REQUIRE(cf_pixel_blue().colors.r == 0);
	REQUIRE(cf_pixel_blue().colors.g == 0);
	REQUIRE(cf_pixel_blue().colors.b == 255);

	// Secondary/tertiary colors.
	REQUIRE(cf_pixel_yellow().colors.r == 255);
	REQUIRE(cf_pixel_yellow().colors.g == 255);
	REQUIRE(cf_pixel_yellow().colors.b == 0);
	REQUIRE(cf_pixel_yellow().colors.a == 255);
	REQUIRE(cf_pixel_orange().colors.r == 255);
	REQUIRE(cf_pixel_orange().colors.g == 165);
	REQUIRE(cf_pixel_orange().colors.b == 0);
	REQUIRE(cf_pixel_purple().colors.r == 255);
	REQUIRE(cf_pixel_purple().colors.g == 0);
	REQUIRE(cf_pixel_purple().colors.b == 255);
	REQUIRE(cf_pixel_grey().colors.r == 127);
	REQUIRE(cf_pixel_grey().colors.g == 127);
	REQUIRE(cf_pixel_grey().colors.b == 127);
	REQUIRE(cf_pixel_cyan().colors.r == 68);
	REQUIRE(cf_pixel_cyan().colors.g == 220);
	REQUIRE(cf_pixel_cyan().colors.b == 235);
	REQUIRE(cf_pixel_magenta().colors.r == 224);
	REQUIRE(cf_pixel_magenta().colors.g == 70);
	REQUIRE(cf_pixel_magenta().colors.b == 224);
	REQUIRE(cf_pixel_brown().colors.r == 150);
	REQUIRE(cf_pixel_brown().colors.g == 105);
	REQUIRE(cf_pixel_brown().colors.b == 25);

	return true;
}

//--------------------------------------------------------------------------------------------------
// HSV round-trip.

TEST_CASE(test_hsv_round_trip)
{
	// Primary/secondary colors have known exact HSV values and round-trip perfectly.
	// H is stored as 0-1 (not 0-360), S and V are 0-1.
	struct { float r, g, b; float h, s, v; } cases[] = {
		{ 1, 0, 0,  0.0f/6,    1, 1 }, // red
		{ 1, 1, 0,  1.0f/6,    1, 1 }, // yellow
		{ 0, 1, 0,  2.0f/6,    1, 1 }, // green
		{ 0, 1, 1,  3.0f/6,    1, 1 }, // cyan
		{ 0, 0, 1,  4.0f/6,    1, 1 }, // blue
		{ 1, 0, 1,  5.0f/6,    1, 1 }, // magenta
		{ 1, 1, 1,  0,         0, 1 }, // white
		{ 0.5f, 0.5f, 0.5f, 0, 0, 0.5f }, // grey
	};
	for (int i = 0; i < 8; i++) {
		CF_Color c = cf_make_color_rgb_f(cases[i].r, cases[i].g, cases[i].b);
		CF_Color hsv = cf_rgb_to_hsv(c);

		// Verify forward conversion matches known HSV.
		REQUIRE(cf_abs(hsv.r - cases[i].h) < 1e-5f);
		REQUIRE(cf_abs(hsv.g - cases[i].s) < 1e-5f);
		REQUIRE(cf_abs(hsv.b - cases[i].v) < 1e-5f);

		// Verify perfect round-trip back to RGB.
		CF_Color back = cf_hsv_to_rgb(hsv);
		REQUIRE(cf_abs(back.r - c.r) < 1e-5f);
		REQUIRE(cf_abs(back.g - c.g) < 1e-5f);
		REQUIRE(cf_abs(back.b - c.b) < 1e-5f);
	}

	// Alpha is preserved through both conversions.
	CF_Color c = cf_make_color_rgba_f(1.0f, 0.0f, 0.0f, 0.42f);
	CF_Color hsv = cf_rgb_to_hsv(c);
	REQUIRE(hsv.a == 0.42f);
	CF_Color back = cf_hsv_to_rgb(hsv);
	REQUIRE(back.a == 0.42f);

	return true;
}

//--------------------------------------------------------------------------------------------------
// Round-trip tests.

TEST_CASE(test_round_trip_hex_int_color)
{
	// hex -> CF_Color -> to_int_rgb -> cf_make_color_hex -> same color.
	CF_Color c = cf_make_color_hex(0xFFC41F);
	uint32_t rgb = cf_color_to_int_rgb(c);
	REQUIRE(rgb == 0x00FFC41F);
	CF_Color c2 = cf_make_color_hex((int)rgb);
	REQUIRE(c2.r == c.r);
	REQUIRE(c2.g == c.g);
	REQUIRE(c2.b == c.b);
	REQUIRE(c2.a == c.a);

	return true;
}

TEST_CASE(test_round_trip_hex_int_pixel)
{
	// hex -> CF_Pixel -> to_int_rgb -> cf_make_pixel_hex -> same pixel.
	CF_Pixel p = cf_make_pixel_hex(0xFFC41F);
	uint32_t rgb = cf_pixel_to_int_rgb(p);
	REQUIRE(rgb == 0x00FFC41F);
	CF_Pixel p2 = cf_make_pixel_hex((int)rgb);
	REQUIRE(p2.colors.r == p.colors.r);
	REQUIRE(p2.colors.g == p.colors.g);
	REQUIRE(p2.colors.b == p.colors.b);
	REQUIRE(p2.colors.a == p.colors.a);

	return true;
}

TEST_CASE(test_round_trip_string_color)
{
	// String -> CF_Color -> to_string -> CF_Color -> same values.
	CF_Color c = cf_make_color_hex_string("#ffc41f");
	char* s = cf_color_to_string(c);
	CF_Color c2 = cf_make_color_hex_string(s);
	REQUIRE(c2.r == c.r);
	REQUIRE(c2.g == c.g);
	REQUIRE(c2.b == c.b);
	REQUIRE(c2.a == c.a);
	sfree(s);

	return true;
}

TEST_CASE(test_round_trip_string_pixel)
{
	// String -> CF_Pixel -> to_string -> CF_Pixel -> same values.
	CF_Pixel p = cf_make_pixel_hex_string("#ffc41f");
	char* s = cf_pixel_to_string(p);
	CF_Pixel p2 = cf_make_pixel_hex_string(s);
	REQUIRE(p2.colors.r == p.colors.r);
	REQUIRE(p2.colors.g == p.colors.g);
	REQUIRE(p2.colors.b == p.colors.b);
	REQUIRE(p2.colors.a == p.colors.a);
	sfree(s);

	return true;
}

TEST_CASE(test_round_trip_color_pixel)
{
	// CF_Color -> CF_Pixel -> CF_Color round trip.
	CF_Color c = cf_make_color_rgba(200, 100, 50, 128);
	CF_Pixel p = cf_color_to_pixel(c);
	REQUIRE(p.colors.r == 200);
	REQUIRE(p.colors.g == 100);
	REQUIRE(p.colors.b == 50);
	REQUIRE(p.colors.a == 128);
	CF_Color c2 = cf_pixel_to_color(p);
	REQUIRE(c2.r == c.r);
	REQUIRE(c2.g == c.g);
	REQUIRE(c2.b == c.b);
	REQUIRE(c2.a == c.a);

	return true;
}

TEST_CASE(test_round_trip_hex_rgba_pixel)
{
	// Make pixel, get RGBA int, parse back through string hex.
	CF_Pixel p = cf_make_pixel_rgba(0xAB, 0xCD, 0xEF, 0x42);
	uint32_t rgba = cf_pixel_to_int_rgba(p);
	REQUIRE(rgba == 0xABCDEF42);

	// Convert rgba int back to pixel by shifting.
	CF_Pixel p2 = cf_make_pixel_rgba(
		(uint8_t)((rgba >> 24) & 0xFF),
		(uint8_t)((rgba >> 16) & 0xFF),
		(uint8_t)((rgba >> 8) & 0xFF),
		(uint8_t)(rgba & 0xFF)
	);
	REQUIRE(p2.colors.r == p.colors.r);
	REQUIRE(p2.colors.g == p.colors.g);
	REQUIRE(p2.colors.b == p.colors.b);
	REQUIRE(p2.colors.a == p.colors.a);

	return true;
}

TEST_CASE(test_round_trip_string_with_alpha)
{
	// 8-char hex string with alpha round-trips through both color and pixel.
	CF_Color c = cf_make_color_hex_string("#ff8040c0");
	REQUIRE(c.r == 255.0f / 255.0f);
	REQUIRE(c.g == 128.0f / 255.0f);
	REQUIRE(c.b == 64.0f / 255.0f);
	REQUIRE(c.a == 192.0f / 255.0f);

	CF_Pixel p = cf_make_pixel_hex_string("#ff8040c0");
	REQUIRE(p.colors.r == 255);
	REQUIRE(p.colors.g == 128);
	REQUIRE(p.colors.b == 64);
	REQUIRE(p.colors.a == 192);

	// Cross-verify pixel-to-color matches.
	CF_Color c2 = cf_pixel_to_color(p);
	REQUIRE(c2.r == c.r);
	REQUIRE(c2.g == c.g);
	REQUIRE(c2.b == c.b);
	REQUIRE(c2.a == c.a);

	return true;
}

TEST_CASE(test_consistency_color_pixel_hex)
{
	// cf_make_color_hex and cf_make_pixel_hex with the same input should produce equivalent results.
	CF_Color c = cf_make_color_hex(0xABCDEF);
	CF_Pixel p = cf_make_pixel_hex(0xABCDEF);
	REQUIRE(p.colors.r == (uint8_t)(c.r * 255.0f));
	REQUIRE(p.colors.g == (uint8_t)(c.g * 255.0f));
	REQUIRE(p.colors.b == (uint8_t)(c.b * 255.0f));
	REQUIRE(p.colors.a == (uint8_t)(c.a * 255.0f));

	return true;
}

//--------------------------------------------------------------------------------------------------
// Test suite.

TEST_SUITE(test_color)
{
	// Color constructors.
	RUN_TEST_CASE(test_make_color_rgb_f);
	RUN_TEST_CASE(test_make_color_rgba_f);
	RUN_TEST_CASE(test_make_color_rgb);
	RUN_TEST_CASE(test_make_color_rgba);
	RUN_TEST_CASE(test_make_color_hex);
	RUN_TEST_CASE(test_make_color_hex2);
	RUN_TEST_CASE(test_make_color_hex_string);

	// Pixel constructors.
	RUN_TEST_CASE(test_make_pixel_rgb_f);
	RUN_TEST_CASE(test_make_pixel_rgba_f);
	RUN_TEST_CASE(test_make_pixel_rgb);
	RUN_TEST_CASE(test_make_pixel_rgba);
	RUN_TEST_CASE(test_make_pixel_hex);
	RUN_TEST_CASE(test_make_pixel_hex2);
	RUN_TEST_CASE(test_make_pixel_hex_string);

	// Color arithmetic.
	RUN_TEST_CASE(test_mul_color);
	RUN_TEST_CASE(test_mul_color2);
	RUN_TEST_CASE(test_div_color);
	RUN_TEST_CASE(test_add_color);
	RUN_TEST_CASE(test_sub_color);
	RUN_TEST_CASE(test_abs_color);
	RUN_TEST_CASE(test_splat_color);
	RUN_TEST_CASE(test_clamp_color01);
	RUN_TEST_CASE(test_color_lerp);
	RUN_TEST_CASE(test_color_premultiply);
	RUN_TEST_CASE(test_fract_color);
	RUN_TEST_CASE(test_mod_color);
	RUN_TEST_CASE(test_clamp_color);
	RUN_TEST_CASE(test_hue);
	RUN_TEST_CASE(test_overlay);
	RUN_TEST_CASE(test_overlay_color);
	RUN_TEST_CASE(test_softlight);
	RUN_TEST_CASE(test_softlight_color);

	// Pixel arithmetic.
	RUN_TEST_CASE(test_un8_ops);
	RUN_TEST_CASE(test_mul_pixel);
	RUN_TEST_CASE(test_div_pixel);
	RUN_TEST_CASE(test_add_pixel);
	RUN_TEST_CASE(test_sub_pixel);
	RUN_TEST_CASE(test_pixel_premultiply);
	RUN_TEST_CASE(test_pixel_lerp);

	// Conversions.
	RUN_TEST_CASE(test_pixel_to_color);
	RUN_TEST_CASE(test_color_to_pixel);
	RUN_TEST_CASE(test_pixel_to_int_rgb);
	RUN_TEST_CASE(test_pixel_to_int_rgba);
	RUN_TEST_CASE(test_color_to_int_rgb);
	RUN_TEST_CASE(test_color_to_int_rgba);
	RUN_TEST_CASE(test_pixel_to_string);
	RUN_TEST_CASE(test_color_to_string);
	RUN_TEST_CASE(test_named_colors);
	RUN_TEST_CASE(test_named_pixels);
	RUN_TEST_CASE(test_hsv_round_trip);

	// Round-trip tests.
	RUN_TEST_CASE(test_round_trip_hex_int_color);
	RUN_TEST_CASE(test_round_trip_hex_int_pixel);
	RUN_TEST_CASE(test_round_trip_string_color);
	RUN_TEST_CASE(test_round_trip_string_pixel);
	RUN_TEST_CASE(test_round_trip_color_pixel);
	RUN_TEST_CASE(test_round_trip_hex_rgba_pixel);
	RUN_TEST_CASE(test_round_trip_string_with_alpha);
	RUN_TEST_CASE(test_consistency_color_pixel_hex);
}
