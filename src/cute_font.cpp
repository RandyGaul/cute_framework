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
#include <internal/cute_font_internal.h>

#include <shaders/font_shader.h>

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

static void s_r_splat(image_t* img)
{
	// Shader reads only from the alpha channel to see if there's any visible pixel.
	int pixel_count = img->w * img->h;
	for (int i = 0; i < pixel_count; ++i) {
		pixel_t p = img->pix[i];
		p.colors.g = p.colors.r;
		p.colors.b = p.colors.r;
		p.colors.a = p.colors.r;
		img->pix[i] = p;
	}
}

font_t* font_load_bmfont(const char* font_path, const char* font_image_path)
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
	image_flip_horizontal(&img); // TODO: Is this needed?
	s_r_splat(&img);

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

static void s_load_courier_new()
{
	if (!app->courier_new) {
		image_t img;
		error_t err = image_load_png_mem(courier_new_0_png_data, courier_new_0_png_sz, &img);
		s_r_splat(&img);
		texture_t tex = texture_make(img.pix, img.w, img.h);
		cute_font_t* font = cute_font_load_bmfont(tex, courier_new_fnt_data, courier_new_fnt_sz, app->mem_ctx);
		app->courier_new = font;
		image_free(&img);
	}
}

const font_t* font_get_default()
{
	s_load_courier_new();
	return (font_t*)app->courier_new;
}

void font_push_verts(const font_t* font, const char* text, float x, float y, float wrap_w, const aabb_t* clip_box)
{
	int vert_count = 0;
	cute_font_t* cute_font = (cute_font_t*)font;
	array<font_vertex_t>& font_verts = app->font_verts;
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
		font_vertex_t* verts_ptr = font_verts.data() + font_verts.count();
		int no_overflow = cute_font_fill_vertex_buffer(cute_font, text, x, y, wrap_w, 0, clip_rect_ptr, verts_ptr, font_verts.capacity() - font_verts.count(), &vert_count);

		if (no_overflow) {
			break;
		} else {
			font_verts.ensure_capacity(font_verts.capacity() * 2);
		}
	}

	font_verts.set_count(font_verts.count() + vert_count);
}

void font_draw(const font_t* font, matrix_t mvp, color_t color)
{
	error_t err = triple_buffer_append(&app->font_buffer, app->font_verts.count(), app->font_verts.data());
	CUTE_ASSERT(!err.is_error());

	sg_apply_pipeline(app->font_pip);
	sg_bindings bind = app->font_buffer.bind();
	bind.fs_images[0].id = (uint32_t)((cute_font_t*)font)->atlas_id;
	sg_apply_bindings(bind);
	app->font_vs_uniforms.u_mvp = mvp;
	app->font_fs_uniforms.u_text_color = color;
	sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(app->font_vs_uniforms));
	sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(app->font_fs_uniforms));
	sg_draw(0, app->font_verts.count(), 1);

	app->font_verts.clear();
}

void font_borders(bool use_borders)
{
	app->font_fs_uniforms.u_use_border = use_borders ? 1.0f : 0.0f;
}

void font_toggle_borders()
{
	if (app->font_fs_uniforms.u_use_border) {
		app->font_fs_uniforms.u_use_border = 0;
	} else {
		app->font_fs_uniforms.u_use_border = 1;
	}
}

bool font_is_borders_on()
{
	return app->font_fs_uniforms.u_use_border ? true : false;
}

void font_border_color(color_t color)
{
	app->font_fs_uniforms.u_border_color = color;
}

void font_border_use_corners(bool use_corners)
{
	app->font_fs_uniforms.u_use_corners = use_corners ? 1.0f : 0.0f;
}

int font_height(const font_t* font)
{
	return ((cute_font_t*)font)->font_height;
}

int font_line_height(const font_t* font)
{
	return ((cute_font_t*)font)->line_height;
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

void font_init()
{
	s_load_courier_new();

	app->font_shader = sg_make_shader(font_shd_shader_desc(sg_query_backend()));

	sg_pipeline_desc pip_params = { 0 };
	pip_params.layout.buffers[0].stride = sizeof(font_vertex_t);
	pip_params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	pip_params.layout.buffers[0].step_rate = 1;
	pip_params.layout.attrs[0].buffer_index = 0;
	pip_params.layout.attrs[0].offset = 0;
	pip_params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.layout.attrs[1].buffer_index = 0;
	pip_params.layout.attrs[1].offset = sizeof(float) * 2;
	pip_params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	pip_params.shader = app->font_shader;
	pip_params.colors[0].blend.enabled = true;
	pip_params.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	pip_params.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pip_params.colors[0].blend.op_rgb = SG_BLENDOP_ADD;
	pip_params.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	pip_params.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	pip_params.colors[0].blend.op_alpha = SG_BLENDOP_ADD;
	app->font_pip = sg_make_pipeline(pip_params);

	app->font_buffer = triple_buffer_make(sizeof(font_vertex_t) * 1024 * 2, sizeof(font_vertex_t));

	app->font_fs_uniforms.u_border_color = color_white();
	app->font_fs_uniforms.u_use_border = false;
	app->font_fs_uniforms.u_texel_size = v2(1.0f / (float)app->courier_new->atlas_w, 1.0f / (float)app->courier_new->atlas_h);
	app->font_fs_uniforms.u_use_corners = false;
}

}
