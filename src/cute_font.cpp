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
#define CUTE_FNOT_FREE CUTE_FREE
#include <cute/cute_font.h>
#undef CUTE_FONT_H // Super hacks, but whatever.

#include <cute_font.h>
#include <cute_file_system.h>
#include <cute_image.h>
#include <cute_gfx.h>

#include <internal/cute_app_internal.h>

namespace cute
{

using font_t = cute_font_t;

static CUTE_FONT_U64 s_generate_texture_handle(app_t* app, void* pixels, int w, int h)
{
	return (CUTE_FONT_U64)gfx_texture_create(app, w, h, pixels, GFX_PIXEL_FORMAT_R8B8G8A8, GFX_WRAP_MODE_CLAMP_BORDER);
}

static void s_destroy_texture_handle(app_t* app, CUTE_FONT_U64 atlas_id)
{
	gfx_texture_clean_up(app, (gfx_texture_t*)atlas_id);
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

	CUTE_FONT_U64 texture_handle = s_generate_texture_handle(app, img.pix, img.w, img.h);
	font_t* font = cute_font_load_bmfont(texture_handle, font_data, (int)font_size, app->mem_ctx);
	image_free(&img);

	CUTE_FREE(font_data, app->mem_ctx);
	CUTE_FREE(image_data, app->mem_ctx);

	return font;
}

void font_free(font_t* font)
{
	cute_font_free(font);
}

error_t font_fill_vertex_buffer(font_t* font, void* buffer, int buffer_size, const char* text, float x, float y, float wrap_w, aabb_t* clip_box)
{
	int vert_count = 0;

	cute_font_vert_t* font_verts = (cute_font_vert_t*)buffer;
	int font_verts_capacity = buffer_size / sizeof(cute_font_vert_t);

	cute_font_rect_t clip_rect;
	if (clip_box) {
		clip_rect.left = clip_box->min.x;
		clip_rect.right = clip_box->max.x;
		clip_rect.top = clip_box->max.y;
		clip_rect.bottom = clip_box->min.y;
	}
	cute_font_rect_t* clip_rect_ptr = clip_box ? &clip_rect : NULL;
	int no_overflow = cute_font_fill_vertex_buffer(font, text, x, y, wrap_w, 0, clip_rect_ptr, font_verts, font_verts_capacity, &vert_count);

	return no_overflow ? error_success() : error_failure("`buffer` wasn't large enough to store the entire text.");
}

void font_submit_draw_call(app_t* app, font_t* font, void* buffer, int buffer_size, color_t color)
{
	gfx_draw_call_t call;
	gfx_draw_call_add_texture(&call, (gfx_texture_t*)font->atlas_id, "u_image");
	call.buffer = app->font_buffer;
	call.shader = app->font_shader;
	gfx_draw_call_add_verts(app, &call, env->font_verts + env->font_verts_count, vert_count);
	gfx_draw_call_add_uniform(&call, "u_text_color", &color, GFX_UNIFORM_TYPE_FLOAT4);
	gfx_draw_call_set_mvp(&call, camera_get_mvp());
	gfx_push_draw_call(env->gfx, &call);
}

}
