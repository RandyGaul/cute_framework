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

#include <data/fonts/courier_new_fnt.h>
#include <data/fonts/courier_new_0_png.h>

static CUTE_FONT_U64 cf_s_generate_texture_handle(cf_pixel_t* pixels, int w, int h)
{
	return cf_make_texture(pixels, w, h);
}

static void cf_s_destroy_texture_handle(CUTE_FONT_U64 atlas_id)
{
	cf_destroy_texture(atlas_id);
}

static void cf_s_r_splat(cf_image_t* img)
{
	// Shader reads only from the alpha channel to see if there's any visible pixel.
	int pixel_count = img->w * img->h;
	for (int i = 0; i < pixel_count; ++i) {
		cf_pixel_t p = img->pix[i];
		p.colors.g = p.colors.r;
		p.colors.b = p.colors.r;
		p.colors.a = p.colors.r;
		img->pix[i] = p;
	}
}

cf_font_t* cf_font_load_bmfont(const char* font_path, const char* font_image_path)
{
	void* font_data;
	size_t font_size;
	cf_result_t err = cf_file_system_read_entire_file_to_memory(font_path, &font_data, &font_size, cf_app->mem_ctx);
	if (cf_is_error(err)) return NULL;

	void* image_data;
	size_t image_size;
	err = cf_file_system_read_entire_file_to_memory(font_image_path, &image_data, &image_size, cf_app->mem_ctx);
	if (cf_is_error(err)) return NULL;

	cf_image_t img;
	err = cf_image_load_png_mem(image_data, (int)image_size, &img, NULL);
	if (cf_is_error(err)) return NULL;
	cf_image_flip_horizontal(&img); // TODO: Is this needed?
	cf_s_r_splat(&img);

	CUTE_FONT_U64 texture_handle = cf_s_generate_texture_handle(img.pix, img.w, img.h);
	cf_font_t* font = (cf_font_t*)cute_font_load_bmfont(texture_handle, font_data, (int)font_size, cf_app->mem_ctx);
	cf_image_free(&img);

	CUTE_FREE(font_data, cf_app->mem_ctx);
	CUTE_FREE(image_data, cf_app->mem_ctx);

	return font;
}

void cf_font_free(cf_font_t* font)
{
	cute_font_free((cute_font_t*)font);
}

static void cf_s_load_courier_new()
{
	if (!cf_app->courier_new) {
		cf_image_t img;
		cf_result_t err = cf_image_load_png_mem(courier_new_0_png_data, courier_new_0_png_sz, &img, NULL);
		cf_s_r_splat(&img);
		cf_texture_t tex = cf_make_texture(img.pix, img.w, img.h);
		cute_font_t* font = cute_font_load_bmfont(tex, courier_new_fnt_data, courier_new_fnt_sz, cf_app->mem_ctx);
		cf_app->courier_new = font;
		cf_image_free(&img);
	}
}

const cf_font_t* cf_font_get_default()
{
	cf_s_load_courier_new();
	return (cf_font_t*)cf_app->courier_new;
}

void cf_font_push_verts(const cf_font_t* font, const char* text, float x, float y, float wrap_w, const cf_aabb_t* clip_box)
{
	int vert_count = 0;
	cute_font_t* cute_font = (cute_font_t*)font;
	cf_array<cf_font_vertex_t>& font_verts = cf_app->font_verts;
	font_verts.ensure_capacity(256);

	while (1) {
		cute_font_rect_t clip_rect;
		if (clip_box) {
			clip_rect.left = clip_box->min.x;
			clip_rect.right = clip_box->max.x;
			clip_rect.top = clip_box->max.y;
			clip_rect.bottom = clip_box->min.y;
		}
		cute_font_rect_t* clip_rect_ptr = clip_box ? &clip_rect : NULL;
		cf_font_vertex_t* verts_ptr = font_verts.data() + font_verts.count();
		int no_overflow = cute_font_fill_vertex_buffer(cute_font, text, x, y, wrap_w, 0, clip_rect_ptr, verts_ptr, font_verts.capacity() - font_verts.count(), &vert_count);

		if (no_overflow) {
			break;
		} else {
			font_verts.ensure_capacity(font_verts.capacity() * 2);
		}
	}

	font_verts.set_count(font_verts.count() + vert_count);
}

void cf_font_draw(const cf_font_t* font, cf_matrix_t mvp, cf_color_t color)
{
	cf_result_t err = cf_app->font_buffer.append(cf_app->font_verts.count(), cf_app->font_verts.data());
	if (cf_is_error(err)) {
		CUTE_WARN("Overflow in `font_draw`, dropping draw call.");
		return;
	}

	sg_apply_pipeline(cf_app->font_pip);
	sg_bindings bind = cf_app->font_buffer.bind();
	bind.fs_images[0].id = (uint32_t)((cute_font_t*)font)->atlas_id;
	sg_apply_bindings(bind);
	cf_app->font_vs_uniforms.u_mvp = mvp;
	cf_app->font_fs_uniforms.u_text_color = color;
	// Need to apply more things here.
	sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(cf_app->font_vs_uniforms));
	sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(cf_app->font_fs_uniforms));
	sg_draw(0, cf_app->font_verts.count(), 1);

	cf_app->font_verts.clear();
}

void cf_font_borders(bool use_borders)
{
	cf_app->font_fs_uniforms.u_use_border = use_borders ? 1.0f : 0.0f;
}

void cf_font_toggle_borders()
{
	if (cf_app->font_fs_uniforms.u_use_border) {
		cf_app->font_fs_uniforms.u_use_border = 0;
	} else {
		cf_app->font_fs_uniforms.u_use_border = 1;
	}
}

bool cf_font_is_borders_on()
{
	return cf_app->font_fs_uniforms.u_use_border ? true : false;
}

void cf_font_border_color(cf_color_t color)
{
	cf_app->font_fs_uniforms.u_border_color = color;
}

void cf_font_border_use_corners(bool use_corners)
{
	cf_app->font_fs_uniforms.u_use_corners = use_corners ? 1.0f : 0.0f;
}

int cf_font_height(const cf_font_t* font)
{
	return ((cute_font_t*)font)->font_height;
}

int cf_font_line_height(const cf_font_t* font)
{
	return ((cute_font_t*)font)->line_height;
}

int cf_font_text_width(const cf_font_t* font, const char* text)
{
	return cute_font_text_width((cute_font_t*)font, text);
}

int cf_font_text_height(const cf_font_t* font, const char* text)
{
	return cute_font_text_height((cute_font_t*)font, text);
}

// -------------------------------------------------------------------------------------------------
// Internal.

void cf_font_init()
{
	cf_s_load_courier_new();

	cf_app->font_shader = sg_make_shader(font_shd_shader_desc(sg_query_backend()));

	sg_pipeline_desc pip_params = { 0 };
	pip_params.layout.buffers[0].stride = sizeof(cf_font_vertex_t);
	pip_params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	pip_params.layout.buffers[0].step_rate = 1;
	pip_params.layout.attrs[0].buffer_index = 0;
	pip_params.layout.attrs[0].offset = 0;
	pip_params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.layout.attrs[1].buffer_index = 0;
	pip_params.layout.attrs[1].offset = sizeof(float) * 2;
	pip_params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	pip_params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	pip_params.shader = cf_app->font_shader;
	pip_params.colors[0].blend.enabled = true;
	pip_params.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	pip_params.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pip_params.colors[0].blend.op_rgb = SG_BLENDOP_ADD;
	pip_params.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	pip_params.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	pip_params.colors[0].blend.op_alpha = SG_BLENDOP_ADD;
	cf_app->font_pip = sg_make_pipeline(pip_params);

	cf_app->font_buffer.init(CUTE_MB * 25, sizeof(cf_font_vertex_t));

	cf_app->font_fs_uniforms.u_border_color = cf_color_white();
	cf_app->font_fs_uniforms.u_use_border = false;
	cf_app->font_fs_uniforms.u_texel_size = cf_V2(1.0f / (float)cf_app->courier_new->atlas_w, 1.0f / (float)cf_app->courier_new->atlas_h);
	cf_app->font_fs_uniforms.u_use_corners = false;
}

namespace cute
{
void font_draw(const cf_font_t* font, matrix_t mvp, color_t color) { cf_font_draw(font, mvp, color); }
}
