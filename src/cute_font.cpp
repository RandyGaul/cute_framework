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

void font_draw(app_t* app, const font_t* font, matrix_t mvp, color_t color)
{
	error_t err = triple_buffer_append(&app->font_buffer, app->font_verts.count(), app->font_verts.data());
	CUTE_ASSERT(!err.is_error());

	sg_apply_pipeline(app->font_pip);
	sg_bindings bind = app->font_buffer.bind();
	bind.fs_images[0].id = (uint32_t)((cute_font_t*)font)->atlas_id;
	sg_apply_bindings(bind);
	app->font_vs_uniforms.mvp = mvp;
	app->font_fs_uniforms.u_text_color = color;
	sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &app->font_vs_uniforms, sizeof(app->font_vs_uniforms));
	sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &app->font_fs_uniforms, sizeof(app->font_fs_uniforms));
	sg_draw(0, app->font_verts.count(), 1);

	app->font_verts.clear();
}

void font_borders(app_t* app, bool use_borders)
{
	app->font_fs_uniforms.use_border = use_borders ? 1.0f : 0.0f;
}

void font_toggle_borders(app_t* app)
{
	if (app->font_fs_uniforms.use_border) {
		app->font_fs_uniforms.use_border = 0;
	} else {
		app->font_fs_uniforms.use_border = 1;
	}
}

bool font_is_borders_on(app_t* app)
{
	return app->font_fs_uniforms.use_border ? true : false;
}

void font_border_color(app_t* app, color_t color)
{
	app->font_fs_uniforms.u_border_color = color;
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

	sg_shader_desc shader_params = { 0 };
	shader_params.attrs[0].name = "pos";
	shader_params.attrs[0].sem_name = "POSITION";
	shader_params.attrs[0].sem_index = 0;
	shader_params.attrs[1].name = "uv";
	shader_params.attrs[1].sem_name = "TEXCOORD";
	shader_params.attrs[1].sem_index = 0;
	shader_params.vs.uniform_blocks[0].size = sizeof(font_vs_uniforms_t);
	shader_params.vs.uniform_blocks[0].uniforms[0].name = "u_mvp";
	shader_params.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_MAT4;
	shader_params.fs.uniform_blocks[0].size = sizeof(font_fs_uniforms_t);
	shader_params.fs.uniform_blocks[0].uniforms[0].name = "u_text_color";
	shader_params.fs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
	shader_params.fs.uniform_blocks[0].uniforms[1].name = "u_border_color";
	shader_params.fs.uniform_blocks[0].uniforms[1].type = SG_UNIFORMTYPE_FLOAT4;
	shader_params.fs.uniform_blocks[0].uniforms[2].name = "u_texel_size";
	shader_params.fs.uniform_blocks[0].uniforms[2].type = SG_UNIFORMTYPE_FLOAT2;
	shader_params.fs.uniform_blocks[0].uniforms[3].name = "u_use_border";
	shader_params.fs.uniform_blocks[0].uniforms[3].type = SG_UNIFORMTYPE_FLOAT;
	shader_params.fs.images[0].name = "u_image";
	shader_params.fs.images[0].type = SG_IMAGETYPE_2D;
	shader_params.vs.source = CUTE_STRINGIZE(
		struct vertex_t
		{
			float2 pos : POSITION;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : SV_Position;
			float2 uv   : TEXCOORD0;
		};

		cbuffer params : register(b0)
		{
			row_major float4x4 u_mvp;
		};

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(ceil(vtx.pos), 0, 1), u_mvp);

			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);
	shader_params.fs.source = CUTE_STRINGIZE(
		struct interp_t
		{
			float4 posH : SV_Position;
			float2 uv   : TEXCOORD0;
		};

		cbuffer params : register(b0)
		{
			float4 u_text_color;
			float4 u_border_color;
			float2 u_texel_size;
			float u_use_border;
		};

		Texture2D<float4> u_image: register(t0);
		sampler smp: register(s0);

		float4 main(interp_t interp) : SV_Target0
		{
			float2 uv = interp.uv;

			// Border detection for pixel outlines.
			float a = u_image.Sample(smp, uv + float2(0,  u_texel_size.y)).r;
			float b = u_image.Sample(smp, uv + float2(0, -u_texel_size.y)).r;
			float c = u_image.Sample(smp, uv + float2(u_texel_size.x,  0)).r;
			float d = u_image.Sample(smp, uv + float2(-u_texel_size.x, 0)).r;
			float e = u_image.Sample(smp, uv + float2(-u_texel_size.x, -u_texel_size.y)).r;
			float f = u_image.Sample(smp, uv + float2(-u_texel_size.x,  u_texel_size.y)).r;
			float g = u_image.Sample(smp, uv + float2( u_texel_size.x, -u_texel_size.y)).r;
			float h = u_image.Sample(smp, uv + float2( u_texel_size.x,  u_texel_size.y)).r;
			float i = u_image.Sample(smp, uv).r;
			float border = max(a, max(b, max(c, max(d, max(e, max(f, max(g, h))))))) * (1 - i);

			border *= u_use_border;

			// Pick black for font and white for border.
			float4 border_color = u_border_color;
			float4 text_color = u_text_color * i;
			float4 color = lerp(text_color, border_color, border);

			return color;
		}
	);
	app->font_shader = sg_make_shader(shader_params);

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
	pip_params.blend.enabled = true;
	pip_params.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	pip_params.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pip_params.blend.op_rgb = SG_BLENDOP_ADD;
	pip_params.blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	pip_params.blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	pip_params.blend.op_alpha = SG_BLENDOP_ADD;
	app->font_pip = sg_make_pipeline(pip_params);

	app->font_buffer = triple_buffer_make(sizeof(font_vertex_t) * 1024 * 2, sizeof(font_vertex_t));

	app->font_fs_uniforms.u_border_color = color_white();
	app->font_fs_uniforms.use_border = 0;
	app->font_fs_uniforms.u_texel_size = v2(1.0f / (float)app->courier_new->atlas_w, 1.0f / (float)app->courier_new->atlas_h);
}

}
