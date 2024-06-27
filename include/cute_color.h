/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_COLOR_H
#define CF_COLOR_H

#include "cute_defines.h"
#include "cute_string.h"
#include "cute_math.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Color
 * @category graphics
 * @brief    16-byte value with 0.0f to 1.0f components, representing an RGBA (red green blue alpha) color.
 * @related  CF_Color CF_Pixel
 */
typedef struct CF_Color
{
	/* @member The red component, from 0.0f to 1.0f. */
	float r;

	/* @member The green component, from 0.0f to 1.0f. */
	float g;

	/* @member The blue component, from 0.0f to 1.0f. */
	float b;

	/* @member The alpha component (transparency/opaqueness), from 0.0f to 1.0f. */
	float a;
} CF_Color;
// @end

/**
 * @struct   CF_Pixel
 * @category graphics
 * @brief    4-byte value with 0-255 components, representing an RGBA (red green blue alpha) color.
 * @related  CF_Color CF_Pixel
 */
typedef union CF_Pixel
{
	struct
	{
		/* @member The red component, from 0 to 255. */
		uint8_t r;

		/* @member The green component, from 0 to 255. */
		uint8_t g;

		/* @member The blue component, from 0 to 255. */
		uint8_t b;

		/* @member The alpha component (transparency/opaqueness), from 0 to 255. */
		uint8_t a;
	} colors;

	/* @member A 32-bit unsigned integer containing the packed-bytes for all four RGBA components. */
	uint32_t val;
} CF_Pixel;
// @end

/**
 * @function cf_make_color_rgb_f
 * @category graphics
 * @brief    Returns a `CF_Color` from RGB float inputs.
 * @param    r          The red component from 0.0f to 1.0f.
 * @param    g          The green component from 0.0f to 1.0f.
 * @param    b          The blue component from 0.0f to 1.0f.
 * @remarks  The alpha component is set to 1.0f;
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_rgb_f(float r, float g, float b) { CF_Color color; color.r = r; color.g = g; color.b = b; color.a = 1.0f; return color; }

/**
 * @function cf_make_color_rgba_f
 * @category graphics
 * @brief    Returns a `CF_Color` from RGBA float inputs.
 * @param    r          The red component from 0.0f to 1.0f.
 * @param    g          The green component from 0.0f to 1.0f.
 * @param    b          The blue component from 0.0f to 1.0f.
 * @param    a          The alpha component from 0.0f to 1.0f.
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_rgba_f(float r, float g, float b, float a) { CF_Color color; color.r = r; color.g = g; color.b = b; color.a = a; return color; }

/**
 * @function cf_make_color_rgb
 * @category graphics
 * @brief    Returns a `CF_Color` made from RGB char inputs.
 * @param    r          The red component from 0 to 255.
 * @param    g          The green component from 0 to 255.
 * @param    b          The blue component from 0 to 255.
 * @remarks  The alpha component is set to 1.0f;
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_rgb(uint8_t r, uint8_t g, uint8_t b) { CF_Color color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = 1.0f; return color; }

/**
 * @function cf_make_color_rgba
 * @category graphics
 * @brief    Returns a `CF_Color` made from RGB char inputs.
 * @param    r          The red component from 0 to 255.
 * @param    g          The green component from 0 to 255.
 * @param    b          The blue component from 0 to 255.
 * @param    a          The alpha component from 0 to 255.
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { CF_Color color; color.r = (float)r / 255.0f; color.g = (float)g / 255.0f; color.b = (float)b / 255.0f; color.a = (float)a / 255.0f; return color; }

/**
 * @function cf_make_color_hex
 * @category graphics
 * @brief    Returns a `CF_Color` made from integer hex input.
 * @param    hex        An integer value, e.g. 0xFFAACC.
 * @remarks  The opacity of the output color is set to 0xFF (fully opaque).
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_hex(int hex) { return cf_make_color_rgba((uint8_t)((hex & 0xFF0000) >> 16), (uint8_t)((hex & 0x00FF00) >> 8), (uint8_t)(hex & 0x0000FF), 0xFF); }

/**
 * @function cf_make_color_hex2
 * @category graphics
 * @brief    Returns a `CF_Color` made from integer hex input.
 * @param    hex        An integer value, e.g. 0xFFAACC, and alpha e.g. 0xFF.
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_hex2(int hex, int alpha) { return cf_make_color_rgba((uint8_t)((hex & 0xFF0000) >> 16), (uint8_t)((hex & 0x00FF00) >> 8), (uint8_t)(hex & 0x0000FF), (uint8_t)(alpha & 0xFF)); }

/**
 * @function cf_make_color_hex_string
 * @category graphics
 * @brief    Returns a `CF_Color` made from hex-value string, such as "#42f563" or "0x42f563FF".
 * @param    hex        A hex-value string, such as "#42f563" or "0x42f563FF".
 * @related  CF_Color cf_make_color_rgb_f cf_make_color_rgba_f cf_make_color_rgb cf_make_color_rgba cf_make_color_hex cf_make_color_hex_string
 */
CF_INLINE CF_Color cf_make_color_hex_string(const char* hex) { return cf_make_color_hex((int)stohex(hex)); }

/**
 * @function cf_make_pixel_rgb_f
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from RGB float inputs.
 * @param    r          The red component from 0.0f to 1.0f.
 * @param    g          The green component from 0.0f to 1.0f.
 * @param    b          The blue component from 0.0f to 1.0f.
 * @remarks  The alpha component is set to 255.
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_rgb_f(float r, float g, float b) { CF_Pixel p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = 255; return p; }

/**
 * @function cf_make_pixel_rgba_f
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from RGBA float inputs.
 * @param    r          The red component from 0.0f to 1.0f.
 * @param    g          The green component from 0.0f to 1.0f.
 * @param    b          The blue component from 0.0f to 1.0f.
 * @param    b          The alpha component from 0.0f to 1.0f.
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_rgba_f(float r, float g, float b, float a) { CF_Pixel p; p.colors.r = (uint8_t)(r * 255.0f); p.colors.g = (uint8_t)(g * 255.0f); p.colors.b = (uint8_t)(b * 255.0f); p.colors.a = (uint8_t)(a * 255.0f); return p; }

/**
 * @function cf_make_pixel_rgb
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from RGB char inputs.
 * @param    r          The red component from 0 to 255.
 * @param    g          The green component from 0 to 255.
 * @param    b          The blue component from 0 to 255.
 * @remarks  The alpha component is set to 255.
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_rgb(uint8_t r, uint8_t g, uint8_t b) { CF_Pixel p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = 255; return p; }

/**
 * @function cf_make_pixel_rgba
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from RGB char inputs.
 * @param    r          The red component from 0 to 255.
 * @param    g          The green component from 0 to 255.
 * @param    b          The blue component from 0 to 255.
 * @param    a          The alpha component from 0 to 255.
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { CF_Pixel p; p.colors.r = r; p.colors.g = g; p.colors.b = b; p.colors.a = a; return p; }

/**
 * @function cf_make_pixel_hex
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from integer hex input.
 * @param    hex        An integer value, e.g. 0xFFAACC11.
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_hex(int hex) { return cf_make_pixel_rgba((uint8_t)((hex & 0xFF000000) >> 24), (uint8_t)((hex & 0x00FF0000) >> 16), (uint8_t)((hex & 0x0000FF00) >> 8), (uint8_t)(hex & 0x000000FF)); }

/**
 * @function cf_make_pixel_hex_string
 * @category graphics
 * @brief    Returns a `CF_Pixel` made from hex-value string, such as "#42f563" or "0x42f563FF".
 * @param    hex        A hex-value string, such as "#42f563" or "0x42f563FF".
 * @related  CF_Pixel cf_make_pixel_rgb_f cf_make_pixel_rgba_f cf_make_pixel_rgb cf_make_pixel_rgba cf_make_pixel_hex cf_make_pixel_hex_string
 */
CF_INLINE CF_Pixel cf_make_pixel_hex_string(const char* hex) { return cf_make_pixel_hex((int)stohex(hex)); }

/**
 * @function cf_mul_color
 * @category graphics
 * @brief    Multiplies each component of a color by `s`.
 * @param    a          The color.
 * @param    s          A value to multiply with.
 * @return   Returns the multiplied color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_mul_color(CF_Color a, float s) { return cf_make_color_rgba_f(a.r * s, a.g * s, a.b * s, a.a * s); }

/**
 * @function cf_mul_color2
 * @category graphics
 * @brief    Performs component-wise multiplication between two colors.
 * @param    a          The first color.
 * @param    b          The second color.
 * @return   For colors `a` and `a` the color `{ a.r*b.r, a.g*b.g, a.b*b.b, a.a*b.a }` is returned.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_mul_color2(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r*b.r, a.g*b.g, a.b*b.b, a.a*b.a); }

/**
 * @function cf_div_color
 * @category graphics
 * @brief    Divides each component of a color by `s`.
 * @param    a          The color.
 * @param    s          A value to divide with.
 * @return   Returns the divided color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_div_color(CF_Color a, float s) { return cf_make_color_rgba_f(a.r/s, a.g/s, a.b/s, a.a/s); }

/**
 * @function cf_add_color
 * @category graphics
 * @brief    Returns two colors added together.
 * @param    a          The first color.
 * @param    b          The second color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_add_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r+b.r, a.g+b.g, a.b+b.b, a.a+b.a); }

/**
 * @function cf_sub_color
 * @category graphics
 * @brief    Returns two color `b` subtracted from color `a`.
 * @param    a          The first color.
 * @param    b          The second color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_sub_color(CF_Color a, CF_Color b) { return cf_make_color_rgba_f(a.r-b.r, a.g-b.g, a.b-b.b, a.a-b.a); }

/**
 * @function cf_abs_color
 * @category graphics
 * @brief    Returns the component-wise absolute value of a color.
 * @param    a          The color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_abs_color(CF_Color a) { return cf_make_color_rgba_f(cf_abs(a.r), cf_abs(a.g), cf_abs(a.b), cf_abs(a.a)); }

/**
 * @function cf_fract_color
 * @category graphics
 * @brief    Returns the fractional portion of each component of a color.
 * @param    a          The color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_fract_color(CF_Color a) { return cf_make_color_rgba_f(cf_fract(a.r), cf_fract(a.g), cf_fract(a.b), cf_fract(a.a)); }

/**
 * @function cf_splat_color
 * @category graphics
 * @brief    Returns a color where all components are the same value.
 * @param    v          The value to set each color component to.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_splat_color(float v) { CF_Color color; color.r = v; color.g = v; color.b = v; color.a = v; return color; }

/**
 * @function cf_mod_color
 * @category graphics
 * @brief    Returns the component-wise mod of a color.
 * @param    m          The mod value.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_mod_color(CF_Color a, float m) { return cf_make_color_rgba_f(cf_mod(a.r, m), cf_mod(a.g, m), cf_mod(a.b, m), cf_mod(a.a, m)); }

/**
 * @function cf_clamp_color
 * @category graphics
 * @brief    Returns the component-wise clamp of `a` between `lo` and `hi`.
 * @param    a          The color.
 * @param    lo         The min value for clamping.
 * @param    hi         The max value for clamping.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_clamp_color(CF_Color a, CF_Color lo, CF_Color hi) { return cf_make_color_rgba_f(cf_clamp(a.r, lo.r, hi.r), cf_clamp(a.g, lo.g, hi.g), cf_clamp(a.b, lo.b, hi.b), cf_clamp(a.a, lo.a, hi.a)); }

/**
 * @function cf_clamp_color01
 * @category graphics
 * @brief    Returns the component-wise clamp of `a` between 0.0f and 1.0f.
 * @param    a          The color.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_clamp_color01(CF_Color a) { return cf_make_color_rgba_f(cf_clamp(a.r, 0, 1.0f), cf_clamp(a.g, 0, 1.0f), cf_clamp(a.b, 0, 1.0f), cf_clamp(a.a, 0, 1.0f)); }

/**
 * @function cf_color_lerp
 * @category graphics
 * @brief    Returns a linearly interpolated color from `a` to `b`.
 * @param    a          The first color.
 * @param    b          The second color.
 * @param    s          The interpolant from 0.0f to 1.0f.
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_color_lerp(CF_Color a, CF_Color b, float s) { return cf_add_color(a, cf_mul_color(cf_sub_color(b, a), s)); }

/**
 * @function cf_color_premultiply
 * @category graphics
 * @brief    Returns a color in premultiplied alpha form.
 * @param    c          The color.
 * @remarks  Read here for more information about [premultiplied alpha](https://limnu.com/premultiplied-alpha-primer-artists/).
 * @related  CF_Color cf_mul_color cf_mul_color2 cf_div_color cf_add_color cf_sub_color cf_abs_color cf_fract_color cf_splat_color cf_mod_color cf_clamp_color cf_clamp_color01 cf_color_lerp cf_color_premultiply
 */
CF_INLINE CF_Color cf_color_premultiply(CF_Color c) { c.r *= c.a; c.g *= c.a; c.b *= c.a; return c; }

// HSV <-> RGB from : http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// And https://www.shadertoy.com/view/MsS3Wc

/**
 * @function cf_rgb_to_hsv
 * @category graphics
 * @brief    Returns a color converted from rgb form to HSV (Hue Saturation Value) form.
 * @param    c          The color.
 * @remarks  Read here for more information about [HSV](https://en.wikipedia.org/wiki/HSL_and_HSV). Often times colors interpolated (`cf_color_lerp`) in HSV form
 *           look way better than in RGB form. Sometimes it's a good idea to convert to HSV, interpolate, then convert back to RGB in order to interpolate
 *           between two RGB colors. If you interpolate between RGB colors without the intermediate HSV conversion, you may find the middle color to be
 *           some kind of ugly grey color. The intermediate HSV conversion may avoid this grey interpolation artifact.
 * @related  CF_Color cf_rgb_to_hsv cf_hsv_to_rgb
 */
CF_INLINE CF_Color cf_rgb_to_hsv(CF_Color c)
{
	CF_Color K = cf_make_color_rgba_f(0, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
	CF_Color p = c.g < c.b ? cf_make_color_rgba_f(c.b, c.g, K.a, K.b) : cf_make_color_rgba_f(c.g, c.b, K.r, K.g);
	CF_Color q = c.r < p.r ? cf_make_color_rgba_f(p.r, p.g, p.a, c.r) : cf_make_color_rgba_f(c.r, p.g, p.b, p.r);
	float d = q.r - cf_min(q.a, q.g);
	float e = 1.0e-10f;
	return cf_make_color_rgba_f(cf_abs(q.b + (q.a - q.g) / (6.0f * d + e)), d / (q.r + e), q.r, c.a);
}

/**
 * @function cf_hsv_to_rgb
 * @category graphics
 * @brief    Returns a color converted from HSV (Hue Saturation Value) form to rgb form.
 * @param    c          The color.
 * @remarks  Read here for more information about [HSV](https://en.wikipedia.org/wiki/HSL_and_HSV). Often times colors interpolated (`cf_color_lerp`) in HSV form
 *           look way better than in RGB form. Sometimes it's a good idea to convert to HSV, interpolate, then convert back to RGB in order to interpolate
 *           between two RGB colors. If you interpolate between RGB colors without the intermediate HSV conversion, you may find the middle color to be
 *           some kind of ugly grey color. The intermediate HSV conversion may avoid this grey interpolation artifact.
 * @related  CF_Color cf_rgb_to_hsv cf_hsv_to_rgb
 */
CF_INLINE CF_Color cf_hsv_to_rgb(CF_Color c)
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

/**
 * @function cf_hue
 * @category graphics
 * @brief    Returns a result color with the luminance and saturation of the base color and the hue of the blend color.
 * @param    base       The original color.
 * @param    tint       The blend color to apply a hue-tint effect with.
 * @remarks  This function attempts to mimic the Hue [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
 * @related  CF_Color cf_hue cf_overlay_color cf_softlight_color cf_overlay cf_softlight
 */
CF_INLINE CF_Color cf_hue(CF_Color base, CF_Color tint)
{
	float alpha = base.a;
	base = cf_rgb_to_hsv(base);
	tint = cf_rgb_to_hsv(tint);
	return cf_hsv_to_rgb(cf_make_color_rgba_f(tint.r, base.g, base.b, alpha));
}

/**
 * @function cf_overlay
 * @category graphics
 * @brief    Returns an overlay blend, where the colors are multiplied or screen'd depending on the `base` color.
 * @param    base       The original color.
 * @param    blend      The blend color to apply an overlay effect with.
 * @remarks  This function attempts to mimic the Overlay [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
 *           The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.
 * @related  CF_Color cf_hue cf_overlay_color cf_softlight_color cf_overlay cf_softlight
 */
CF_INLINE float cf_overlay(float base, float blend) { return (base <= 0.5f) ? 2*base * blend : 1-2*(1-base) * (1-blend); }

/**
 * @function cf_softlight
 * @category graphics
 * @brief    Returns a softlight blend, where the colors are darkened or lightened depending on the `blend` color.
 * @param    base       The original color.
 * @param    blend      The blend color to apply a softlight effect with.
 * @remarks  This function attempts to mimic the Softlight [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
 *           The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.
 * @related  CF_Color cf_hue cf_overlay_color cf_softlight_color cf_overlay cf_softlight
 */
CF_INLINE float cf_softlight(float base, float blend) { if (blend <= 0.5f) return base - (1-2*blend)*base*(1-base); else return base + (2*blend-1) * (((base <= 0.25f) ? ((16*base-12) * base+4) * base : CF_SQRTF(base)) - base); }

/**
 * @function cf_overlay_color
 * @category graphics
 * @brief    Returns an overlay'd color, where the colors are multiplied or screen'd depending on the `base` color.
 * @param    base       The original color.
 * @param    blend      The blend color to apply an overlay effect with.
 * @remarks  This function attempts to mimic the Overlay [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
 *           The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.
 * @related  CF_Color cf_hue cf_overlay_color cf_softlight_color cf_overlay cf_softlight
 */
CF_INLINE CF_Color cf_overlay_color(CF_Color base, CF_Color blend) { return cf_make_color_rgba_f(cf_overlay(base.r, blend.r), cf_overlay(base.g, blend.g), cf_overlay(base.b, blend.b), base.a); }

/**
 * @function cf_softlight_color
 * @category graphics
 * @brief    Returns a softlight'd color, where the colors are darkened or lightened depending on the `blend` color.
 * @param    base       The original color.
 * @param    blend      The blend color to apply a softlight effect with.
 * @remarks  This function attempts to mimic the Softlight [Photoshop blend-layer](https://helpx.adobe.com/photoshop/using/blending-modes.html) mode.
 *           The `blend` color is used to adjust colors in the `base`, while still preserving shadows and highlights of the `base`.
 * @related  CF_Color cf_hue cf_overlay_color cf_softlight_color cf_overlay cf_softlight
 */
CF_INLINE CF_Color cf_softlight_color(CF_Color base, CF_Color blend) { return cf_make_color_rgba_f(cf_softlight(base.r, blend.r), cf_softlight(base.g, blend.g), cf_softlight(base.b, blend.b), base.a); }

/**
 * @function cf_mul_un8
 * @category graphics
 * @brief    Returns the 8-bit unsigned mutiplication of two input 8-bit numbers, intended for implementing 8-bit color blend operations.
 * @param    a          An 8-bit value promoted to an `int` upon calling.
 * @param    b          An 8-bit value promoted to an `int` upon calling.
 * @remarks  This is a helper-function intended for advanced users.
 *           The core calculation is done with full integers to help avoid intermediate overflows on 8-bit values.
 * @related  cf_mul_un8 cf_div_un8 cf_add_un8 cf_sub_un8
 */
CF_INLINE uint8_t cf_mul_un8(int a, int b) { int t = (a * b) + 0x80; return (uint8_t)(((t >> 8) + t) >> 8); }

/**
 * @function cf_div_un8
 * @category graphics
 * @brief    Returns the 8-bit unsigned division of two input 8-bit numbers, intended for implementing 8-bit color blend operations.
 * @param    a          An 8-bit value promoted to an `int` upon calling.
 * @param    b          An 8-bit value promoted to an `int` upon calling.
 * @remarks  This is a helper-function intended for advanced users.
 *           The core calculation is done with full integers to help avoid intermediate overflows on 8-bit values.
 * @related  cf_mul_un8 cf_div_un8 cf_add_un8 cf_sub_un8
 */
CF_INLINE uint8_t cf_div_un8(int a, int b) { return (uint8_t)(((b) * 0xFF + ((a) / 2)) / (a)); }

/**
 * @function cf_add_un8
 * @category graphics
 * @brief    Returns the 8-bit unsigned addition of two input 8-bit numbers, intended for implementing 8-bit color blend operations.
 * @param    a          An 8-bit value promoted to an `int` upon calling.
 * @param    b          An 8-bit value promoted to an `int` upon calling.
 * @remarks  This is a helper-function intended for advanced users.
 *           The core calculation is done with full integers to help avoid intermediate overflows on 8-bit values.
 * @related  cf_mul_un8 cf_div_un8 cf_add_un8 cf_sub_un8
 */
CF_INLINE uint8_t cf_add_un8(int a, int b) { int t = a + b; return (uint8_t)(t | (t >> 8)); }

/**
 * @function cf_sub_un8
 * @category graphics
 * @brief    Returns the 8-bit unsigned subtraction of two input 8-bit numbers, intended for implementing 8-bit color blend operations.
 * @param    a          An 8-bit value promoted to an `int` upon calling.
 * @param    b          An 8-bit value promoted to an `int` upon calling.
 * @remarks  This is a helper-function intended for advanced users.
 *           The core calculation is done with full integers to help avoid intermediate overflows on 8-bit values.
 * @related  cf_mul_un8 cf_div_un8 cf_add_un8 cf_sub_un8
 */
CF_INLINE uint8_t cf_sub_un8(int a, int b) { int t = a - b; if (t < 0) t = 0; return (uint8_t)(t | (t >> 8)); }

/**
 * @function cf_mul_pixel
 * @category graphics
 * @brief    Multiplies a `CF_Pixel` by an unsigned 8-bit number.
 * @param    a          The pixel.
 * @param    b          An 8-bit value.
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_mul_pixel(CF_Pixel a, uint8_t s) { return cf_make_pixel_rgba_f(cf_mul_un8(a.colors.r, s), cf_mul_un8(a.colors.g, s), cf_mul_un8(a.colors.b, s), cf_mul_un8(a.colors.a, s)); }

/**
 * @function cf_div_pixel
 * @category graphics
 * @brief    Divides a `CF_Pixel` by an unsigned 8-bit number.
 * @param    a          The pixel.
 * @param    b          An 8-bit value.
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_div_pixel(CF_Pixel a, uint8_t s) { return cf_make_pixel_rgba_f(cf_div_un8(a.colors.r, s), cf_div_un8(a.colors.g, s), cf_div_un8(a.colors.b, s), cf_div_un8(a.colors.a, s)); }

/**
 * @function cf_add_pixel
 * @category graphics
 * @brief    Adds two `CF_Pixel`s together.
 * @param    a          The first pixel.
 * @param    b          The second pixel.
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_add_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(cf_add_un8(a.colors.r, b.colors.r), cf_add_un8(a.colors.g, b.colors.g), cf_add_un8(a.colors.b, b.colors.b), cf_add_un8(a.colors.a, b.colors.a)); }

/**
 * @function cf_sub_pixel
 * @category graphics
 * @brief    Subtracts one `CF_Pixel` from another.
 * @param    a          The first pixel.
 * @param    b          The second pixel.
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_sub_pixel(CF_Pixel a, CF_Pixel b) { return cf_make_pixel_rgba(cf_sub_un8(a.colors.r, b.colors.r), cf_sub_un8(a.colors.g, b.colors.g), cf_sub_un8(a.colors.b, b.colors.b), cf_sub_un8(a.colors.a, b.colors.a)); }

CF_INLINE CF_Color cf_pixel_to_color(CF_Pixel p);
CF_INLINE CF_Pixel cf_color_to_pixel(CF_Color c);

/**
 * @function cf_pixel_lerp
 * @category graphics
 * @brief    Lerps from one `CF_Pixel` to another.
 * @param    a          The first pixel.
 * @param    b          The second pixel.
 * @param    s          The interpolant from 0 to 1.
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_pixel_lerp(CF_Pixel a, CF_Pixel b, float s) { CF_Color A = cf_pixel_to_color(a); return cf_color_to_pixel(cf_add_color(A, cf_mul_color(cf_sub_color(cf_pixel_to_color(b), A), s))); }

/**
 * @function cf_pixel_premultiply
 * @category graphics
 * @brief    Returns a premultiplied `CF_Pixel` by its alpha component.
 * @param    a          The pixel.
 * @remarks  Read here for more information about [premultiplied alpha](https://limnu.com/premultiplied-alpha-primer-artists/).
 * @related  cf_mul_pixel cf_div_pixel cf_add_pixel cf_sub_pixel cf_pixel_lerp cf_pixel_premultiply
 */
CF_INLINE CF_Pixel cf_pixel_premultiply(CF_Pixel c) { c.colors.r = cf_mul_un8(c.colors.r, c.colors.a); c.colors.g = cf_mul_un8(c.colors.g, c.colors.a); c.colors.b = cf_mul_un8(c.colors.b, c.colors.a); return c; }

/**
 * @function cf_pixel_to_color
 * @category graphics
 * @brief    Converts a `CF_Pixel` to a color.
 * @param    p          The pixel.
 * @return   Returns a `CF_Color` (0.0f-1.0f) converted from pixel form (0-255).
 * @related  cf_pixel_to_color cf_pixel_to_int_rgba cf_pixel_to_int_rgb cf_pixel_to_string
 */
CF_INLINE CF_Color cf_pixel_to_color(CF_Pixel p) { return cf_make_color_rgba(p.colors.r, p.colors.g, p.colors.b, p.colors.a); }

/**
 * @function cf_pixel_to_int_rgb
 * @category graphics
 * @brief    Converts an RGB `CF_Pixel` to an integer.
 * @param    p          The pixel.
 * @return   Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
 *           the green component, the third byte is the blue component, the fourth byte is 0xFF or full-alpha.
 * @related  cf_pixel_to_color cf_pixel_to_int_rgba cf_pixel_to_int_rgb cf_pixel_to_string
 */
CF_INLINE uint32_t cf_pixel_to_int_rgb(CF_Pixel p) { return p.val | 0xFF000000; }

/**
 * @function cf_pixel_to_int_rgba
 * @category graphics
 * @brief    Converts an RGBA `CF_Pixel` to an integer.
 * @param    p          The pixel.
 * @return   Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
 *           the green component, the third byte is the blue component, the fourth byte is the alpha component.
 * @related  cf_pixel_to_color cf_pixel_to_int_rgba cf_pixel_to_int_rgb cf_pixel_to_string
 */
CF_INLINE uint32_t cf_pixel_to_int_rgba(CF_Pixel p) { return p.val; }

/**
 * @function cf_pixel_to_string
 * @category graphics
 * @brief    Converts a `CF_Pixel` to a dynamic string. Free it with `sfree` when done.
 * @param    p          The pixel.
 * @remarks  Since this function dynamically allocates a Cute Framework C-string, it must be free'd up with `sfree` when you're done with it.
 * @related  cf_pixel_to_color cf_pixel_to_int_rgba cf_pixel_to_int_rgb cf_pixel_to_string
 */
CF_INLINE char* cf_pixel_to_string(CF_Pixel p) { char* s = NULL; return shex(s, p.val); } // Call `sfree` when done.

/**
 * @function cf_color_to_pixel
 * @category graphics
 * @brief    Converts a `CF_Color` (0.0f to 1.0f) to a `CF_Pixel` (0-255).
 * @param    c          The color.
 * @related  cf_color_to_pixel cf_color_to_int_rgb cf_color_to_int_rgba cf_color_to_string
 */
CF_INLINE CF_Pixel cf_color_to_pixel(CF_Color c) { CF_Pixel p; p.colors.r = (int)((uint8_t)(c.r * 255.0f)); p.colors.g = (int)((uint8_t)(c.g * 255.0f)); p.colors.b = (int)((uint8_t)(c.b * 255.0f)); p.colors.a = (int)((uint8_t)(c.a * 255.0f)); return p; }

/**
 * @function cf_color_to_int_rgb
 * @category graphics
 * @brief    Converts an RGBA `CF_Color` to an integer.
 * @param    c          The color.
 * @return   Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
 *           the green component, the third byte is the blue component, the fourth byte is 0xFF or full-alpha.
 * @related  cf_color_to_pixel cf_color_to_int_rgb cf_color_to_int_rgba cf_color_to_string
 */
CF_INLINE uint32_t cf_color_to_int_rgb(CF_Color c) { return cf_color_to_pixel(c).val | 0xFF000000; }

/**
 * @function cf_color_to_int_rgba
 * @category graphics
 * @brief    Converts an RGB `CF_Color` to an integer.
 * @param    c          The color.
 * @return   Returns an unsigned 32-bit integer of the packed pixel components. The first byte is the red component, the second byte is
 *           the green component, the third byte is the blue component, the fourth byte is the alpha component.
 * @related  cf_color_to_pixel cf_color_to_int_rgb cf_color_to_int_rgba cf_color_to_string
 */
CF_INLINE uint32_t cf_color_to_int_rgba(CF_Color c) { return cf_color_to_pixel(c).val; }

/**
 * @function cf_color_to_string
 * @category graphics
 * @brief    Converts a `CF_Color` to a dynamic string. Free it with `sfree` when done.
 * @param    p          The pixel.
 * @remarks  Since this function dynamically allocates a Cute Framework C-string, it must be free'd up with `sfree` when you're done with it.
 * @related  cf_pixel_to_color cf_pixel_to_int_rgba cf_pixel_to_int_rgb cf_pixel_to_string
 */
CF_INLINE char* cf_color_to_string(CF_Color c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); }

/**
 * @function cf_color_invisible
 * @category graphics
 * @brief    Helper function to return an invisible `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_invisible() { return cf_make_color_rgba_f(0.0f, 0.0f, 0.0f, 0.0f); }

/**
 * @function cf_color_black
 * @category graphics
 * @brief    Helper function to return a black `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_black() { return cf_make_color_rgb_f(0.0f, 0.0f, 0.0f); }

/**
 * @function cf_color_white
 * @category graphics
 * @brief    Helper function to return a white `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_white() { return cf_make_color_rgb_f(1.0f, 1.0f, 1.0f); }

/**
 * @function cf_color_red
 * @category graphics
 * @brief    Helper function to return a red `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_red() { return cf_make_color_rgb_f(1.0f, 0.0f, 0.0f); }

/**
 * @function cf_color_green
 * @category graphics
 * @brief    Helper function to return a green `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_green() { return cf_make_color_rgb_f(0.0f, 1.0f, 0.0f); }

/**
 * @function cf_color_blue
 * @category graphics
 * @brief    Helper function to return a blue `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_blue() { return cf_make_color_rgb_f(0.0f, 0.0f, 1.0f); }

/**
 * @function cf_color_yellow
 * @category graphics
 * @brief    Helper function to return a yellow `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_yellow() { return cf_make_color_rgb_f(1.0f, 1.0f, 0.0f); }

/**
 * @function cf_color_orange
 * @category graphics
 * @brief    Helper function to return a orange `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_orange() { return cf_make_color_rgb_f(1.0f, 0.65f, 0.0f); }

/**
 * @function cf_color_purple
 * @category graphics
 * @brief    Helper function to return a purple `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_purple() { return cf_make_color_rgb_f(1.0f, 0.0f, 1.0f); }

/**
 * @function cf_color_grey
 * @category graphics
 * @brief    Helper function to return a grey `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_grey() { return cf_make_color_rgb_f(0.5f, 0.5f, 0.5f); }

/**
 * @function cf_color_cyan
 * @category graphics
 * @brief    Helper function to return a cyan `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_cyan() { return cf_make_color_rgb(68, 220, 235); }

/**
 * @function cf_color_magenta
 * @category graphics
 * @brief    Helper function to return a magenta `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_magenta() { return cf_make_color_rgb(224, 70, 224); }

/**
 * @function cf_color_brown
 * @category graphics
 * @brief    Helper function to return a brown `CF_Color`.
 * @related  cf_color_invisible cf_color_black cf_color_white cf_color_red cf_color_green cf_color_blue cf_color_yellow cf_color_orange cf_color_purple cf_color_grey cf_color_cyan cf_color_magenta
 */
CF_INLINE CF_Color cf_color_brown() { return cf_make_color_rgb(150, 105, 25); }

/**
 * @function cf_pixel_invisible
 * @category graphics
 * @brief    Helper function to return a invisible `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_invisible() { return cf_make_pixel_hex(0); }

/**
 * @function cf_pixel_black
 * @category graphics
 * @brief    Helper function to return a black `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_black() { return cf_make_pixel_rgb(0, 0, 0); }

/**
 * @function cf_pixel_white
 * @category graphics
 * @brief    Helper function to return a invisible `white`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_white() { return cf_make_pixel_rgb(255, 255, 255); }

/**
 * @function cf_pixel_red
 * @category graphics
 * @brief    Helper function to return a red `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_red() { return cf_make_pixel_rgb(255, 0, 0); }

/**
 * @function cf_pixel_green
 * @category graphics
 * @brief    Helper function to return a green `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_green() { return cf_make_pixel_rgb(0, 255, 0); }

/**
 * @function cf_pixel_blue
 * @category graphics
 * @brief    Helper function to return a blue `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_blue() { return cf_make_pixel_rgb(0, 0, 255); }

/**
 * @function cf_pixel_yellow
 * @category graphics
 * @brief    Helper function to return a yellow `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_yellow() { return cf_make_pixel_rgb(255, 255, 0); }

/**
 * @function cf_pixel_orange
 * @category graphics
 * @brief    Helper function to return a orange `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_orange() { return cf_make_pixel_rgb(255, 165, 0); }

/**
 * @function cf_pixel_purple
 * @category graphics
 * @brief    Helper function to return a purple `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_purple() { return cf_make_pixel_rgb(255, 0, 255); }

/**
 * @function cf_pixel_grey
 * @category graphics
 * @brief    Helper function to return a grey `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_grey() { return cf_make_pixel_rgb(127, 127, 127); }

/**
 * @function cf_pixel_cyan
 * @category graphics
 * @brief    Helper function to return a cyan `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_cyan() { return cf_make_pixel_rgb(68, 220, 235); }

/**
 * @function cf_pixel_magenta
 * @category graphics
 * @brief    Helper function to return a magenta `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_magenta() { return cf_make_pixel_rgb(224, 70, 224); }

/**
 * @function cf_pixel_brown
 * @category graphics
 * @brief    Helper function to return a brown `CF_Pixel`.
 * @related  cf_pixel_invisible cf_pixel_black cf_pixel_white cf_pixel_red cf_pixel_green cf_pixel_blue cf_pixel_yellow cf_pixel_orange cf_pixel_purple cf_pixel_grey cf_pixel_cyan cf_pixel_magenta
 */
CF_INLINE CF_Pixel cf_pixel_brown() { return cf_make_pixel_rgb(150, 105, 25); }

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Pixel = CF_Pixel;
using Color = CF_Color;

CF_INLINE Color make_color(float r, float g, float b) { return cf_make_color_rgb_f(r, g, b); }
CF_INLINE Color make_color(float r, float g, float b, float a) { return cf_make_color_rgba_f(r, g, b, a); }
CF_INLINE Color make_color(uint8_t r, uint8_t g, uint8_t b) { return cf_make_color_rgb(r, g, b); }
CF_INLINE Color make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_color_rgba(r, g, b, a); }
CF_INLINE Color make_color(int hex) { return cf_make_color_hex(hex); }
CF_INLINE Color make_color(const char* s) { return make_color((int)stohex(s)); }

CF_INLINE Pixel make_pixel(float r, float g, float b) { return cf_make_pixel_rgb_f(r, g, b); }
CF_INLINE Pixel make_pixel(float r, float g, float b, float a) { return cf_make_pixel_rgba_f(r, g, b, a); }
CF_INLINE Pixel make_pixel(uint8_t r, uint8_t g, uint8_t b) { return cf_make_pixel_rgb(r, g, b); }
CF_INLINE Pixel make_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return cf_make_pixel_rgba(r, g, b, a); }
CF_INLINE Pixel make_pixel(int hex) { return cf_make_pixel_hex(hex); }
CF_INLINE Pixel make_pixel(const char* hex) { return cf_make_pixel_hex((int)stohex(hex)); }

CF_INLINE Color operator*(Color a, float s) { return cf_mul_color(a, s); }
CF_INLINE Color operator/(Color a, float s) { return cf_div_color(a, s); }
CF_INLINE Color operator+(Color a, Color b) { return cf_add_color(a, b); }
CF_INLINE Color operator-(Color a, Color b) { return cf_sub_color(a, b); }
CF_INLINE Color operator*(Color a, Color b) { CF_Color c; c.r = a.r * b.r; c.g = a.g * b.g; c.b = a.b * b.b; c.a = a.a * b.a; return c; }
CF_INLINE bool operator==(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
CF_INLINE bool operator!=(Color a, Color b) { return !(a == b); }
CF_INLINE CF_Color abs(CF_Color a) { return cf_abs_color(a); }
CF_INLINE CF_Color fract(CF_Color a) { return cf_fract_color(a); }
CF_INLINE CF_Color mod(CF_Color a, float m) { return cf_mod_color(a, m); }
CF_INLINE CF_Color splat(float v) { return cf_splat_color(v); }
CF_INLINE CF_Color clamp(CF_Color a, CF_Color lo, CF_Color hi) { return cf_clamp_color(a, lo, hi); }
CF_INLINE CF_Color clamp01(CF_Color a) { return cf_clamp_color01(a); }
CF_INLINE Color lerp(Color a, Color b, float s) { return cf_color_lerp(a, b, s); }
CF_INLINE Color premultiply(Color c) { return cf_color_premultiply(c); }
CF_INLINE CF_Color rgb_to_hsv(CF_Color c) { return cf_rgb_to_hsv(c); }
CF_INLINE CF_Color hsv_to_rgb(CF_Color c) { return cf_hsv_to_rgb(c); }
CF_INLINE CF_Color hue(CF_Color base, CF_Color tint) { return cf_hue(base, tint); }
CF_INLINE float overlay(float base, float blend) { return cf_overlay(base, blend); }
CF_INLINE float softlight(float base, float blend) { return cf_softlight(base, blend); }
CF_INLINE CF_Color overlay(CF_Color base, CF_Color blend) { return cf_overlay_color(base, blend); }
CF_INLINE CF_Color softlight(CF_Color base, CF_Color blend) { return cf_softlight_color(base, blend); }

CF_INLINE Pixel operator*(Pixel a, int s) { return cf_mul_pixel(a, s); }
CF_INLINE Pixel operator/(Pixel a, int s) { return cf_div_pixel(a, s); }
CF_INLINE Pixel operator+(Pixel a, Pixel b) { return cf_add_pixel(a, b); }
CF_INLINE Pixel operator-(Pixel a, Pixel b) { return cf_sub_pixel(a, b); }
CF_INLINE bool operator==(Pixel a, Pixel b) { return a.val == b.val; }
CF_INLINE bool operator!=(Pixel a, Pixel b) { return a.val != b.val; }
CF_INLINE Pixel lerp(Pixel a, Pixel b, uint8_t s) { return cf_pixel_lerp(a, b, s); }
CF_INLINE Pixel premultiply(Pixel p) { return cf_pixel_premultiply(p); }

CF_INLINE Color to_color(Pixel p) { return cf_make_color_rgba(p.colors.r, p.colors.g, p.colors.b, p.colors.a); }
CF_INLINE uint32_t to_int_rgba(Pixel p) { return p.val; }
CF_INLINE uint32_t to_int_rgb(Pixel p) { return p.val | 0xFF000000; }
CF_INLINE String to_string(Pixel p) { char* s = NULL; return shex(s, p.val); }

CF_INLINE Pixel to_pixel(Color c) { return cf_color_to_pixel(c); }
CF_INLINE uint32_t to_int_rgb(Color c) { return cf_color_to_pixel(c).val | 0xFF000000; }
CF_INLINE uint32_t to_int_rgba(Color c) { return cf_color_to_pixel(c).val; }
CF_INLINE String to_string(Color c) { char* s = NULL; return shex(s, cf_color_to_pixel(c).val); }

CF_INLINE Color color_invisible() { return cf_color_invisible(); }
CF_INLINE Color color_black() { return cf_color_black(); }
CF_INLINE Color color_white() { return cf_color_white(); }
CF_INLINE Color color_red() { return cf_color_red(); }
CF_INLINE Color color_green() { return cf_color_green(); }
CF_INLINE Color color_blue() { return cf_color_blue(); }
CF_INLINE Color color_yellow() { return cf_color_yellow(); }
CF_INLINE Color color_orange() { return cf_color_orange(); }
CF_INLINE Color color_purple() { return cf_color_purple(); }
CF_INLINE Color color_grey() { return cf_color_grey(); }
CF_INLINE Color color_cyan() { return cf_color_cyan(); }
CF_INLINE Color color_magenta() { return cf_color_magenta(); }

CF_INLINE Pixel pixel_invisible() { return cf_pixel_invisible(); }
CF_INLINE Pixel pixel_black() { return cf_pixel_black(); }
CF_INLINE Pixel pixel_white() { return cf_pixel_white(); }
CF_INLINE Pixel pixel_red() { return cf_pixel_red(); }
CF_INLINE Pixel pixel_green() { return cf_pixel_green(); }
CF_INLINE Pixel pixel_blue() { return cf_pixel_blue(); }
CF_INLINE Pixel pixel_yellow() { return cf_pixel_yellow(); }
CF_INLINE Pixel pixel_orange() { return cf_pixel_orange(); }
CF_INLINE Pixel pixel_purple() { return cf_pixel_purple(); }
CF_INLINE Pixel pixel_grey() { return cf_pixel_grey(); }
CF_INLINE Pixel pixel_cyan() { return cf_pixel_cyan(); }
CF_INLINE Pixel pixel_magenta() { return cf_pixel_magenta(); }

}

#endif // CF_CPP

#endif // CF_COLOR_H
