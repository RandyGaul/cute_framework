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

#include <cute_alloc.h>

#define CUTE_FONT_IMPLEMENTATION
#define CUTE_FONT_ALLOC CUTE_ALLOC
#define CUTE_FONT_FREE CUTE_FREE
#include <cute/cute_font.h>

#include <cute_font.h>
#include <cute_file_system.h>
#include <cute_image.h>
#include <cute_gfx.h>

#include <internal/cute_app_internal.h>

namespace cute
{

#include <data/fonts/courier_new_fnt.h>
#include <data/fonts/courier_new_0_png.h>

static CUTE_FONT_U64 s_generate_texture_handle(pixel_t* pixels, int w, int h)
{
	return texture_make(pixels, w, h);
}

static void s_destroy_texture_handle(CUTE_FONT_U64 atlas_id)
{
	texture_destroy(atlas_id);
}

font_t* font_load_bmfont(app_t* app, const char* font_path, const char* font_image_path)
{
	void* font_data;
	size_t font_size;
	error_t err = file_system_read_entire_file_to_memory(font_path, &font_data, &font_size, app->mem_ctx);
	if (err.is_error()) return NULL;

	void* image_data;
	size_t image_size;
	err = file_system_read_entire_file_to_memory(font_image_path, &image_data, &image_size, app->mem_ctx);
	if (err.is_error()) return NULL;

	image_t img;
	err = image_load_png_mem(image_data, (int)image_size, &img);
	if (err.is_error()) return NULL;
	image_flip_horizontal(&img);

	CUTE_FONT_U64 texture_handle = s_generate_texture_handle(img.pix, img.w, img.h);
	font_t* font = (font_t*)cute_font_load_bmfont(texture_handle, font_data, (int)font_size, app->mem_ctx);
	image_free(&img);

	CUTE_FREE(font_data, app->mem_ctx);
	CUTE_FREE(image_data, app->mem_ctx);

	return font;
}

void font_free(font_t* font)
{
	cute_font_free((cute_font_t*)font);
}

static void s_load_courier_new(app_t* app)
{
	if (!app->courier_new) {
		image_t img;
		error_t err = image_load_png_mem(courier_new_0_png_data, courier_new_0_png_sz, &img);
		image_flip_horizontal(&img);
		texture_t tex = texture_make(img.pix, img.w, img.h);
		cute_font_t* font = cute_font_load_bmfont(tex, courier_new_fnt_data, courier_new_fnt_sz, app->mem_ctx);
		app->courier_new = font;
		image_free(&img);
	}
}

const font_t* font_get_default(app_t* app)
{
	s_load_courier_new(app);
	return (font_t*)app->courier_new;
}

void font_push_verts(app_t* app, const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box)
{
	int vert_count = 0;
	cute_font_t* cute_font = (cute_font_t*)font;
	array<cute_font_vert_t>& font_verts = app->font_verts;
	font_verts.ensure_capacity(256);

	while (1)
	{
		cute_font_rect_t clip_rect;
		if (clip_box) {
			clip_rect.left = clip_box->min.x;
			clip_rect.right = clip_box->max.x;
			clip_rect.top = clip_box->max.y;
			clip_rect.bottom = clip_box->min.y;
		}
		cute_font_rect_t* clip_rect_ptr = clip_box ? &clip_rect : NULL;
		cute_font_vert_t* verts_ptr = font_verts.data() + font_verts.count();
		int no_overflow = cute_font_fill_vertex_buffer(cute_font, text, x, y, wrap_w, 0, clip_rect_ptr, verts_ptr, font_verts.capacity() - font_verts.count(), &vert_count);

		if (no_overflow) {
			break;
		} else {
			font_verts.ensure_capacity(font_verts.capacity() * 2);
		}
	}

	font_verts.set_count(font_verts.count() + vert_count);
}

void font_submit_draw_call(app_t* app, const font_t* font, gfx_matrix_t mvp, color_t color)
{
#if 0
	gfx_draw_call_t call;
	gfx_draw_call_add_texture(&call, (gfx_texture_t*)((cute_font_t*)font)->atlas_id, "u_image");
	call.buffer = app->font_buffer;
	call.shader = app->font_shader;
	gfx_draw_call_add_verts(app, &call, app->font_verts.data(), app->font_verts.count());
	gfx_draw_call_add_uniform(&call, "u_text_color", &color, GFX_UNIFORM_TYPE_FLOAT4);
	gfx_draw_call_set_mvp(&call, mvp);
	gfx_push_draw_call(app, &call);
#endif

	app->font_verts.clear();
}

int font_text_width(const font_t* font, const char* text)
{
	return cute_font_text_width((cute_font_t*)font, text);
}

int font_text_height(const font_t* font, const char* text)
{
	return cute_font_text_height((cute_font_t*)font, text);
}

// -------------------------------------------------------------------------------------------------
// Internal.

void font_init(app_t* app)
{
	s_load_courier_new(app);

#if 0
	// Create the dedicated font shader data.
	gfx_vertex_buffer_params_t vertex_params;
	vertex_params.type = GFX_VERTEX_BUFFER_TYPE_DYNAMIC;
	vertex_params.stride = sizeof(cute_font_vert_t);
	gfx_vertex_buffer_params_add_attribute(&vertex_params, 2, CUTE_OFFSET_OF(cute_font_vert_t, x));
	gfx_vertex_buffer_params_add_attribute(&vertex_params, 2, CUTE_OFFSET_OF(cute_font_vert_t, u));
	vertex_params.vertex_count = 1024 * 10;
	app->font_buffer = gfx_vertex_buffer_new(app, &vertex_params);

	const char* vs = CUTE_STRINGIZE(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		float4x4 u_mvp;
		float2 u_inv_screen_wh;

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);

	const char* ps = CUTE_STRINGIZE(
		struct interp_t
		{
			float4 posH : POSITION;
			float2 uv   : TEXCOORD0;
		};

		sampler2D u_image;
		float2 u_texel_size;
		float u_use_border;
		float4 u_text_color;

		float4 main(interp_t interp) : COLOR
		{
			float2 uv = interp.uv;

			// Border detection for pixel outlines.
			float a = tex2D(u_image, uv + float2(0,  u_texel_size.y)).r;
			float b = tex2D(u_image, uv + float2(0, -u_texel_size.y)).r;
			float c = tex2D(u_image, uv + float2(u_texel_size.x,  0)).r;
			float d = tex2D(u_image, uv + float2(-u_texel_size.x, 0)).r;
			float e = tex2D(u_image, uv + float2(-u_texel_size.x, -u_texel_size.y)).r;
			float f = tex2D(u_image, uv + float2(-u_texel_size.x,  u_texel_size.y)).r;
			float g = tex2D(u_image, uv + float2( u_texel_size.x, -u_texel_size.y)).r;
			float h = tex2D(u_image, uv + float2( u_texel_size.x,  u_texel_size.y)).r;
			float i = tex2D(u_image, uv).r;
			float border = max(a, max(b, max(c, max(d, max(e, max(f, max(g, h))))))) * (1 - i);

			border *= u_use_border;

			// Pick black for font and white for border.
			float4 border_color = float4(1, 1, 1, 1);
			float4 text_color = u_text_color * i;
			float4 color = lerp(text_color, border_color, border);

			return color;
		}
	);

	gfx_matrix_t projection = matrix_ortho_2d((float)app->w, (float)app->h, 0, 0);
	app->font_shader = gfx_shader_new(app, app->font_buffer, vs, ps);
	gfx_shader_set_screen_wh(app, app->font_shader, (float)app->w, (float)app->h);

	v2 texel_size = v2(1.0f / (float)app->courier_new->atlas_w, 1.0f / (float)app->courier_new->atlas_h);
	gfx_shader_set_uniform(app, app->font_shader, "u_texel_size", &texel_size, GFX_UNIFORM_TYPE_FLOAT2);

	float use_border = 0;
	gfx_shader_set_uniform(app, app->font_shader, "u_use_border", &use_border, GFX_UNIFORM_TYPE_FLOAT);
#endif
}

}
