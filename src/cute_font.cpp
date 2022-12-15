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

#include <cute_defines.h>

#include <cute_font.h>
#include <cute_c_runtime.h>
#include <cute_file_system.h>
#include <cute_defer.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_font_internal.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <cute/cute_png.h>

using namespace Cute;

void cf_make_font_mem(void* data, int size, const char* font_name, CF_Result* result_out)
{
	font_name = sintern(font_name);
	CF_Font* font = (CF_Font*)CUTE_NEW(CF_Font);
	font->file_data = (uint8_t*)data;
	if (!stbtt_InitFont(&font->info, font->file_data, stbtt_GetFontOffsetForIndex(font->file_data, 0))) {
		CUTE_FREE(data);
		CUTE_FREE(font);
		if (result_out) *result_out = result_failure("Failed to parse ttf file with stb_truetype.h.");
		return;
	}
	app->fonts.insert(font_name, font);

	// Fetch unscaled vertical metrics for the font.
	int ascent, descent, line_gap;
	stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
	font->ascent = ascent;
	font->descent = descent;
	font->line_gap = line_gap;
	font->line_height = font->ascent - font->descent + font->line_gap;
	font->height = font->ascent - font->descent;

	// Build kerning table.
	Array<stbtt_kerningentry> table_array;
	int table_length = stbtt_GetKerningTableLength(&font->info);
	table_array.ensure_capacity(table_length);
	stbtt_kerningentry* table = table_array.data();
	stbtt_GetKerningTable(&font->info, table, table_length);
	for (int i = 0; i < table_length; ++i) {
		stbtt_kerningentry k = table[i];
		uint64_t key = CF_KERN_KEY(k.glyph1, k.glyph2);
		font->kerning.insert(key, k.advance);
	}

	if (result_out) *result_out = result_success();
}

void cf_make_font(const char* path, const char* font_name, CF_Result* result_out)
{
	void* data;
	size_t size;
	Result err = fs_read_entire_file_to_memory(path, &data, &size);
	if (is_error(err)) {
		CUTE_FREE(data);
		if (result_out) *result_out = err;
		return;
	}
	cf_make_font_mem(data, (int)size, font_name, result_out);
}

void cf_destroy_font(const char* font_name)
{
	font_name = sintern(font_name);
	CF_Font* font = app->fonts.get(font_name);
	if (!font) return;
	app->fonts.remove(font_name);
	CUTE_FREE(font->file_data);
	for (int i = 0; i < font->image_ids.count(); ++i) {
		uint64_t image_id = font->image_ids[i];
		CF_Pixel* pixels = app->font_pixels.get(image_id);
		if (pixels) {
			CUTE_FREE(pixels);
			app->font_pixels.remove(image_id);
		}
	}
	font->~CF_Font();
	CUTE_FREE(font);
}

void cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count)
{
	CF_Font* font = app->fonts.get(sintern(font_name));
	for (int i = 0; i < count; ++i) {
		bool found = false;
		for (int j = 0; j < font->backups.count(); ++j) {
			if (font->backups[j] == codepoints[i]) {
				found = true;
				break;
			}
		}
		if (!found) {
			font->backups.add(codepoints[i]);
		}
	}
}

CF_Font* cf_font_get(const char* font_name)
{
	return app->fonts.get(sintern(font_name));
}

CUTE_INLINE uint64_t cf_glyph_key(int cp, float font_size, int blur)
{
	int k0 = cp;
	int k1 = (int)(font_size * 1000.0f);
	int k2 = blur;
	uint64_t key = ((uint64_t)k0 & 0xFFFFFFFFULL) << 32 | ((uint64_t)k1 & 0xFFFFULL) << 16 | ((uint64_t)k2 & 0xFFFFULL);
	return key;
}

// From fontastash.h, memononen
// Based on Exponential blur, Jani Huhtanen, 2006

#define APREC 16
#define ZPREC 7

static void s_blur_cols(unsigned char* dst, int w, int h, int stride, int alpha)
{
	int x, y;
	for (y = 0; y < h; y++) {
		int z = 0; // force zero border
		for (x = 1; x < w; x++) {
			z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
			dst[x] = (unsigned char)(z >> ZPREC);
		}
		dst[w-1] = 0; // force zero border
		z = 0;
		for (x = w-2; x >= 0; x--) {
			z += (alpha * (((int)(dst[x]) << ZPREC) - z)) >> APREC;
			dst[x] = (unsigned char)(z >> ZPREC);
		}
		dst[0] = 0; // force zero border
		dst += stride;
	}
}

static void s_blur_rows(unsigned char* dst, int w, int h, int stride, int alpha)
{
	int x, y;
	for (x = 0; x < w; x++) {
		int z = 0; // force zero border
		for (y = stride; y < h*stride; y += stride) {
			z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
			dst[y] = (unsigned char)(z >> ZPREC);
		}
		dst[(h-1)*stride] = 0; // force zero border
		z = 0;
		for (y = (h-2)*stride; y >= 0; y -= stride) {
			z += (alpha * (((int)(dst[y]) << ZPREC) - z)) >> APREC;
			dst[y] = (unsigned char)(z >> ZPREC);
		}
		dst[0] = 0; // force zero border
		dst++;
	}
}

static void s_blur(unsigned char* dst, int w, int h, int stride, int blur)
{
	int alpha;
	float sigma;

	if (blur < 1)
		return;

	// Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
	sigma = (float)blur * 0.57735f; // 1 / sqrt(3)
	alpha = (int)((1<<APREC) * (1.0f - expf(-2.3f / (sigma+1.0f))));
	s_blur_rows(dst, w, h, stride, alpha);
	s_blur_cols(dst, w, h, stride, alpha);
	s_blur_rows(dst, w, h, stride, alpha);
	s_blur_cols(dst, w, h, stride, alpha);
}

static void s_save(const char* path, uint8_t* pixels, int w, int h)
{
	cp_image_t img;
	img.w = w;
	img.h = h;
	img.pix = (cp_pixel_t*)CUTE_ALLOC(sizeof(cp_pixel_t) * w * h);
	for (int i = 0; i < w * h; ++i) {
		cp_pixel_t pix;
		pix.r = pix.g = pix.b = pixels[i];
		pix.a = 255;
		img.pix[i] = pix;
	}
	cp_save_png(path, &img);
	CUTE_FREE(img.pix);
}

static void s_render(CF_Font* font, CF_Glyph* glyph, float font_size, int blur)
{
	// Create glyph quad.
	blur = clamp(blur, 0, 20);
	int pad = blur;
	float scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
	int xadvance, lsb, x0, y0, x1, y1;
	stbtt_GetGlyphHMetrics(&font->info, glyph->index, &xadvance, &lsb);
	stbtt_GetGlyphBitmapBox(&font->info, glyph->index, scale, scale, &x0, &y0, &x1, &y1);
	int w = x1 - x0 + pad*2;
	int h = y1 - y0 + pad*2;
	glyph->w = w;
	glyph->h = h;
	glyph->q0 = V2((float)x0, -(float)(y0 + h));
	glyph->q1 = V2((float)(x0 + w), -(float)y0);
	glyph->xadvance = xadvance * scale;
	glyph->visible |= w > 0 && h > 0;

	// Render glyph.
	uint8_t* pixels_1bpp = (uint8_t*)CUTE_CALLOC(w * h);
	CUTE_DEFER(CUTE_FREE(pixels_1bpp));
	stbtt_MakeGlyphBitmap(&font->info, pixels_1bpp + pad * w + pad, w - pad*2, h - pad*2, w, scale, scale, glyph->index);
	//s_save("glyph.png", pixels_1bpp, w, h);

	// Apply blur.
	if (blur) s_blur(pixels_1bpp, w, h, w, blur);
	//s_save("glyph_blur.png", pixels_1bpp, w, h);

	// Convert to premultiplied RGBA8 pixel format.
	CF_Pixel* pixels = (CF_Pixel*)CUTE_ALLOC(w * h * sizeof(CF_Pixel));
	for (int i = 0; i < w * h; ++i) {
		uint8_t v = pixels_1bpp[i];
		CF_Pixel p = { };
		if (v) p = make_pixel(v, v, v, v);
		pixels[i] = p;
	}

	// Allocate an image id for the glyph's sprite.
	glyph->image_id = app->font_image_id_gen++;
	app->font_pixels.insert(glyph->image_id, pixels);
	font->image_ids.add(glyph->image_id);
}

CF_Glyph* cf_font_get_glyph(CF_Font* font, int codepoint, float font_size, int blur)
{
	uint64_t glyph_key = cf_glyph_key(codepoint, font_size, blur);
	CF_Glyph* glyph = font->glyphs.try_get(glyph_key);
	if (!glyph) {
		int glyph_index = stbtt_FindGlyphIndex(&font->info, codepoint);
		if (!glyph_index) {
			// This codepoint doesn't exist in this font.
			// Try and use a backup glyph instead.
			// TODO
			CUTE_ASSERT(false);
		}
		glyph = font->glyphs.insert(glyph_key);
		glyph->index = glyph_index;
		glyph->visible = stbtt_IsGlyphEmpty(&font->info, glyph_index) == 0;
	}
	if (glyph->image_id) return glyph;

	// Render the glyph if it exists in the font, but is not yet rendered.
	s_render(font, glyph, font_size, blur);
	return glyph;
}

float cf_font_get_kern(CF_Font* font, float font_size, int codepoint0, int codepoint1)
{
	uint64_t key = CF_KERN_KEY(codepoint0, codepoint1);
	return font->kerning.get(key) * stbtt_ScaleForPixelHeight(&font->info, font_size);
}
