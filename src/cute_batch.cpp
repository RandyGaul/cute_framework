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

#include <cute_batch.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_file_system.h>
#include <cute_lru_cache.h>
#include <cute_defer.h>
//#include <cute_debug_printf.h>

#include <internal/cute_app_internal.h>

#include <shaders/sprite_shader.h>
#include <shaders/sprite_outline_shader.h>
#include <shaders/geom_shader.h>

struct cf_quad_udata_t
{
	float alpha;
};

#include <cute/cute_png.h>

#define SPRITEBATCH_SPRITE_USERDATA cf_quad_udata_t
#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define DEBUG_VERT(v, c) batch_quad(batch, make_aabb(v, 3, 3), c)

struct cf_quad_vertex_t
{
	cf_v2 pos;
	cf_v2 uv;
	float alpha;
};

struct cf_vertex_t
{
	cf_v2 p;
	cf_color_t c;
};


enum cf_batch_sprite_shader_type_t
{
	CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT,
	CF_BATCH_SPRITE_SHADER_TYPE_OUTLINE,
};

struct cf_scissor_t
{
	int x, y, w, h;
};

static const cf_color_t DEFAULT_TINT = cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f);

struct cf_batch_t
{
	::spritebatch_t sb;

	float atlas_width = 1024;
	float atlas_height = 1024;

	sg_pipeline geom_pip;
	cf_triple_buffer_t geom_buffer;
	cf_array<cf_vertex_t> geom_verts;

	cf_array<cf_quad_vertex_t> sprite_verts;
	cf_triple_buffer_t sprite_buffer;
	sg_shader default_shd = { 0 };
	sg_shader outline_shd = { 0 };
	sg_shader active_shd = { 0 };
	cf_batch_sprite_shader_type_t sprite_shd_type = CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT;
	bool pip_dirty = true;
	sg_pipeline pip = { 0 };
	float outline_use_border = 1.0f;
	float outline_use_corners = 0;
	cf_color_t outline_color = cf_color_white();
	sg_wrap wrap_mode = SG_WRAP_REPEAT;
	sg_filter filter = SG_FILTER_NEAREST;
	cf_matrix_t projection;

	cf_m3x2 m = cf_make_identity();
	cf_array<cf_m3x2> m3x2s;
	cf_array<sg_blend_state> blend_states;
	cf_array<sg_depth_state> depth_states;
	cf_array<sg_stencil_state> stencil_states;
	cf_array<cf_scissor_t> scissors;
	cf_array<cf_color_t> tints = { DEFAULT_TINT };

	cf_get_pixels_fn* get_pixels = NULL;
	void* get_pixels_udata = NULL;

	void* mem_ctx = NULL;
};

//--------------------------------------------------------------------------------------------------
// Internal functions.

static sg_shader cf_s_load_shader(cf_batch_t* b, cf_batch_sprite_shader_type_t type)
{
	sg_shader_desc params = { 0 };

	// Default sprite shader.
	switch (type) {
	case CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT:
		params = *sprite_default_shd_shader_desc(sg_query_backend());
		break;

	default:
		params = *sprite_outline_shd_shader_desc(sg_query_backend());
		break;
	}

	sg_shader shader = sg_make_shader(params);
	return shader;
}

static void cf_s_set_shd_type(cf_batch_t* b, cf_batch_sprite_shader_type_t type)
{
	b->sprite_shd_type = type;

	// Load the shader, if needed, and set the active sprite system shader.
	switch (type)
	{
	case CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT:
		if (b->default_shd.id == SG_INVALID_ID) {
			b->default_shd = cf_s_load_shader(b, type);
		}
		b->active_shd = b->default_shd;
		break;

	case CF_BATCH_SPRITE_SHADER_TYPE_OUTLINE:
		if (b->outline_shd.id == SG_INVALID_ID) {
			b->outline_shd = cf_s_load_shader(b, type);
		}
		b->active_shd = b->outline_shd;
		break;
	}

	b->pip_dirty = true;
}

//--------------------------------------------------------------------------------------------------
// spritebatch_t callbacks.

static void cf_s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	cf_batch_t* b = (cf_batch_t*)udata;

	// Build vertex buffer of all quads for each sprite.
	int vert_count = count * 6;
	b->sprite_verts.ensure_count(vert_count);
	cf_quad_vertex_t* verts = b->sprite_verts.data();

	cf_m3x2 m = cf_make_identity();
	if (b->m3x2s.count()) {
		m = b->m3x2s.last();
	}

	for (int i = 0; i < count; ++i)
	{
		spritebatch_sprite_t* s = sprites + i;

		cf_v2 quad[] = {
			{ -0.5f,  0.5f },
			{  0.5f,  0.5f },
			{  0.5f, -0.5f },
			{ -0.5f, -0.5f },
		};

		for (int j = 0; j < 4; ++j)
		{
			float x = quad[j].x;
			float y = quad[j].y;

			// Rotate sprite about origin.
			float x0 = s->c * x - s->s * y;
			float y0 = s->s * x + s->c * y;
			x = x0;
			y = y0;

			// Scale sprite about origin.
			x *= s->sx;
			y *= s->sy;

			// Translate sprite into the world.
			x += s->x;
			y += s->y;

			// Apply final batch transformation.
			cf_v2 p = cf_v2(x, y);
			p = cf_mul_m32_v2(m, p);

			quad[j].x = p.x;
			quad[j].y = p.y;
		}

		// output transformed quad into CPU buffer
		cf_quad_vertex_t* out_verts = verts + i * 6;

		for (int i = 0; i < 6; ++i) {
			out_verts[i].alpha = s->udata.alpha;
		}

		out_verts[0].pos.x = quad[0].x;
		out_verts[0].pos.y = quad[0].y;
		out_verts[0].uv.x = s->minx;
		out_verts[0].uv.y = s->maxy;

		out_verts[1].pos.x = quad[3].x;
		out_verts[1].pos.y = quad[3].y;
		out_verts[1].uv.x = s->minx;
		out_verts[1].uv.y = s->miny;

		out_verts[2].pos.x = quad[1].x;
		out_verts[2].pos.y = quad[1].y;
		out_verts[2].uv.x = s->maxx;
		out_verts[2].uv.y = s->maxy;

		out_verts[3].pos.x = quad[1].x;
		out_verts[3].pos.y = quad[1].y;
		out_verts[3].uv.x = s->maxx;
		out_verts[3].uv.y = s->maxy;

		out_verts[4].pos.x = quad[3].x;
		out_verts[4].pos.y = quad[3].y;
		out_verts[4].uv.x = s->minx;
		out_verts[4].uv.y = s->miny;

		out_verts[5].pos.x = quad[2].x;
		out_verts[5].pos.y = quad[2].y;
		out_verts[5].uv.x = s->maxx;
		out_verts[5].uv.y = s->miny;
	}

	// Map the vertex buffer with sprite vertex data.
	cf_error_t err = cf_triple_buffer_append(&b->sprite_buffer, vert_count, verts, 0, nullptr);
	CUTE_ASSERT(!err.is_error());

	// Setup resource bindings.
	sg_bindings bind = b->sprite_buffer.bind();
	bind.fs_images[0].id = (uint32_t)sprites->texture_id;
	sg_apply_bindings(bind);

	// Apply uniforms.
	// TODO - Move MVP to the spritebatch_flush function as an optimization.

	// Set shader-specific uniforms.
	switch (b->sprite_shd_type)
	{
	case CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT:
	{
		sprite_default_vs_params_t vs_params;
		vs_params.u_mvp = b->projection;
		sprite_default_fs_params_t fs_params;
		fs_params.u_texture_size = cf_v2(b->atlas_width, b->atlas_height);
		fs_params.u_tint = b->tints.last();
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(vs_params));
		sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(fs_params));
	}	break;

	case CF_BATCH_SPRITE_SHADER_TYPE_OUTLINE:
	{
		sprite_outline_vs_params_t vs_params;
		vs_params.u_mvp = b->projection;
		sprite_outline_fs_params_t fs_params;
		fs_params.u_texture_size = cf_v2(b->atlas_width, b->atlas_height);
		fs_params.u_tint = b->tints.last();
		fs_params.u_border_color = b->outline_color;
		fs_params.u_texel_size = cf_v2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
		fs_params.u_use_border = b->outline_use_border;
		fs_params.u_use_corners = b->outline_use_corners;
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(vs_params));
		sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(fs_params));
	}	break;
	}

	// Kick off a draw call.
	sg_draw(0, vert_count, 1);
}

static void cf_s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	cf_batch_t* b = (cf_batch_t*)udata;
	b->get_pixels(image_id, buffer, bytes_to_fill, b->get_pixels_udata);
}

static SPRITEBATCH_U64 cf_s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	cf_batch_t* b = (cf_batch_t*)udata;
	return cf_texture_make2((cf_pixel_t*)pixels, w, h, b->wrap_mode, b->filter);
}

static void cf_s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	cf_batch_t* b = (cf_batch_t*)udata;
	cf_texture_destroy(texture_id);
}

//--------------------------------------------------------------------------------------------------

static void cf_s_sync_pip(cf_batch_t* b)
{
	if (b->pip_dirty) {
		if (b->pip.id != SG_INVALID_ID) sg_destroy_pipeline(b->pip);
		sg_pipeline_desc params = { 0 };
		params.layout.buffers[0].stride = sizeof(cf_quad_vertex_t);
		params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
		params.layout.buffers[0].step_rate = 1;
		params.layout.attrs[0].buffer_index = 0;
		params.layout.attrs[0].offset = CUTE_OFFSET_OF(cf_quad_vertex_t, pos);
		params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[1].buffer_index = 0;
		params.layout.attrs[1].offset = CUTE_OFFSET_OF(cf_quad_vertex_t, uv);
		params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[2].buffer_index = 0;
		params.layout.attrs[2].offset = CUTE_OFFSET_OF(cf_quad_vertex_t, alpha);
		params.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT;
		params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
		params.shader = b->active_shd;
		params.stencil = b->stencil_states.last();
		params.colors[0].blend = b->blend_states.last();
		b->pip = sg_make_pipeline(params);
		b->pip_dirty = false;
	}
}

cf_batch_t* cf_batch_make(cf_get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx)
{
	cf_batch_t* b = CUTE_NEW(cf_batch_t, cf_app->mem_ctx);
	if (!b) return NULL;

	b->projection = cf_matrix_identity();
	b->get_pixels = get_pixels;
	b->get_pixels_udata = get_pixels_udata;
	b->mem_ctx = mem_ctx;

	sg_pipeline_desc params = { 0 };
	params.layout.buffers[0].stride = sizeof(cf_vertex_t);
	params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	params.layout.buffers[0].step_rate = 1;
	params.layout.attrs[0].buffer_index = 0;
	params.layout.attrs[0].offset = CUTE_OFFSET_OF(cf_vertex_t, p);
	params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	params.layout.attrs[1].buffer_index = 0;
	params.layout.attrs[1].offset = CUTE_OFFSET_OF(cf_vertex_t, c);
	params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
	params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	params.shader = sg_make_shader(geom_shd_shader_desc(sg_query_backend()));
	params.colors[0].blend.enabled = true;
	params.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	params.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	params.colors[0].blend.op_rgb = SG_BLENDOP_ADD;
	params.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
	params.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	params.colors[0].blend.op_alpha = SG_BLENDOP_ADD;

	b->geom_pip = sg_make_pipeline(params);
	b->geom_buffer = cf_triple_buffer_make(sizeof(cf_vertex_t) * 1024 * 10, sizeof(cf_vertex_t), 0, 0);

	cf_batch_push_stencil_defaults(b);
	cf_batch_push_blend_defaults(b);
	b->default_shd = b->active_shd = cf_s_load_shader(b, CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT);
	b->outline_shd = cf_s_load_shader(b, CF_BATCH_SPRITE_SHADER_TYPE_OUTLINE);
	b->sprite_buffer = cf_triple_buffer_make(sizeof(cf_quad_vertex_t) * 1024 * 10, sizeof(cf_quad_vertex_t), 0, 0);

	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = cf_s_batch_report;
	config.get_pixels_callback = cf_s_get_pixels;
	config.generate_texture_callback = cf_s_generate_texture_handle;
	config.delete_texture_callback = cf_s_destroy_texture_handle;
	config.allocator_context = b->mem_ctx;
	config.lonely_buffer_count_till_flush = 0;

	if (spritebatch_init(&b->sb, &config, b)) {
		CUTE_FREE(b, cf_app->mem_ctx);
		if (!b) return NULL;
	}

	b->atlas_width = (float)config.atlas_width_in_pixels;
	b->atlas_height = (float)config.atlas_height_in_pixels;
	b->m3x2s.add(cf_make_identity());

	return b;
}

void cf_batch_destroy(cf_batch_t* b)
{
	spritebatch_term(&b->sb);
	b->~cf_batch_t();
	CUTE_FREE(b, b->mem_ctx);
}

void cf_batch_push(cf_batch_t* b, cf_batch_sprite_t q)
{
	spritebatch_sprite_t s;
	s.image_id = q.id;
	s.w = q.w;
	s.h = q.h;
	s.x = q.transform.p.x;
	s.y = q.transform.p.y;
	s.sx = q.scale_x;
	s.sy = q.scale_y;
	s.s = q.transform.r.s;
	s.c = q.transform.r.c;
	s.sort_bits = (uint64_t)q.sort_bits << 32;
	s.udata.alpha = q.alpha;
	spritebatch_push(&b->sb, s);
}

cf_error_t cf_batch_flush(cf_batch_t* b)
{
	// Draw sprites.
	cf_s_sync_pip(b);
	sg_apply_pipeline(b->pip);

	if (b->scissors.count()) {
		cf_scissor_t scissor = b->scissors.last();
		sg_apply_scissor_rect(scissor.x, scissor.y, scissor.w, scissor.h, false);
	}

	spritebatch_flush(&b->sb);

	b->sprite_buffer.advance();

	// Draw geometry.
	if (b->geom_verts.count()) {
		// Issue draw call.
		sg_apply_pipeline(b->geom_pip);
		geom_vs_params_t params;
		params.u_mvp = b->projection;
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(params));
		cf_error_t err = cf_triple_buffer_append(&b->geom_buffer, b->geom_verts.count(), b->geom_verts.data(), 0, nullptr);
		CUTE_ASSERT(!err.is_error());
		sg_apply_bindings(b->geom_buffer.bind());
		sg_draw(0, b->geom_verts.count(), 1);
		b->geom_verts.clear();
		b->geom_buffer.advance();
	}

	return cf_error_success();
}

void cf_batch_update(cf_batch_t* b)
{
	spritebatch_tick(&b->sb);
	spritebatch_defrag(&b->sb);
}

//--------------------------------------------------------------------------------------------------

void cf_batch_set_texture_wrap_mode(cf_batch_t* b, sg_wrap wrap_mode)
{
	b->wrap_mode = wrap_mode;
}

void cf_batch_set_texture_filter(cf_batch_t* b, sg_filter filter)
{
	b->filter = filter;
}

void cf_batch_set_projection(cf_batch_t* b, cf_matrix_t projection)
{
	b->projection = projection;
}

void cf_batch_outlines(cf_batch_t* b, bool use_outlines)
{
	b->outline_use_border = use_outlines ? 1.0f : 0;
	if (use_outlines) {
		cf_s_set_shd_type(b, CF_BATCH_SPRITE_SHADER_TYPE_OUTLINE);
	} else {
		cf_s_set_shd_type(b, CF_BATCH_SPRITE_SHADER_TYPE_DEFAULT);
	}
}

void cf_batch_outlines_use_corners(cf_batch_t* b, bool use_corners)
{
	b->outline_use_corners = use_corners ? 1.0f : 0;
}

void cf_batch_outlines_color(cf_batch_t* b, cf_color_t c)
{
	b->outline_color = c;
}

void cf_batch_push_m3x2(cf_batch_t* b, cf_m3x2 m)
{
	b->m = m;
	b->m3x2s.add(m);
}

void cf_batch_pop_m3x2(cf_batch_t* b)
{
	if (b->m3x2s.count() > 1) {
		b->m3x2s.pop();
		if (b->m3x2s.size()) {
			b->m = b->m3x2s.last();
		} else {
			b->m = cf_make_identity();
		}
	}
}

void cf_batch_push_scissor_box(cf_batch_t* b, int x, int y, int w, int h)
{
	cf_scissor_t scissor = { x, y, w, h };
	b->scissors.add(scissor);
}

void cf_batch_pop_scissor_box(cf_batch_t* b)
{
	if (b->scissors.count()) {
		b->scissors.pop();
	}
}

void cf_batch_push_depth_state(cf_batch_t* b, const sg_depth_state* depth_state)
{
	b->depth_states.add(*depth_state);
	b->pip_dirty = true;
}

void cf_batch_push_depth_defaults(cf_batch_t* b)
{
	sg_depth_state depth_state;
	CUTE_MEMSET(&depth_state, 0, sizeof(depth_state));
	cf_batch_push_depth_state(b, &depth_state);
}

void cf_batch_pop_depth_state(cf_batch_t* b)
{
	if (b->depth_states.count() > 1) {
		b->depth_states.pop();
		b->pip_dirty = true;
	}
}

void cf_batch_push_stencil_state(cf_batch_t* b, const sg_stencil_state* stencil_state)
{
	b->stencil_states.add(*stencil_state);
	b->pip_dirty = true;
}

void cf_batch_push_stencil_defaults(cf_batch_t* b)
{
	sg_stencil_state stencil_state;
	CUTE_MEMSET(&stencil_state, 0, sizeof(stencil_state));
	cf_batch_push_stencil_state(b, &stencil_state);
}

void cf_batch_pop_stencil_state(cf_batch_t* b)
{
	if (b->stencil_states.count() > 1) {
		b->stencil_states.pop();
		b->pip_dirty = true;
	}
}

void cf_batch_push_blend_state(cf_batch_t* b, const sg_blend_state* blend_state)
{
	b->blend_states.add(*blend_state);
	b->pip_dirty = true;
}

void cf_batch_push_blend_defaults(cf_batch_t* b)
{
	sg_blend_state blend_state;
	CUTE_MEMSET(&blend_state, 0, sizeof(blend_state));
	blend_state.enabled = true;
	blend_state.src_factor_rgb = SG_BLENDFACTOR_ONE;
	blend_state.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.op_rgb = SG_BLENDOP_ADD;
	blend_state.src_factor_alpha = SG_BLENDFACTOR_ONE;
	blend_state.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.op_alpha = SG_BLENDOP_ADD;
	cf_batch_push_blend_state(b, &blend_state);
}

void cf_batch_pop_blend_state(cf_batch_t* b)
{
	if (b->blend_states.count() > 1) {
		b->blend_states.pop();
		b->pip_dirty = true;
	}
}

void cf_batch_push_tint(cf_batch_t* b, cf_color_t c)
{
	b->tints.add(c);
}

void cf_batch_pop_tint(cf_batch_t* b)
{
	if (b->tints.count() > 1) {
		b->tints.pop();
	}
}

void cf_batch_quad_aabb(cf_batch_t* b, cf_aabb_t bb, cf_color_t c)
{
	cf_v2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_verts(b, verts[0], verts[1], verts[2], verts[3], c);
}

#define PUSH_VERT(P, C) \
	do { \
		cf_vertex_t v; \
		v.p = cf_mul_m32_v2(b->m, P); \
		v.c = C; \
		b->geom_verts.add(v); \
	} while (0)

#define PUSH_TRI(p0, p1, p2, c0, c1, c2) \
	do { \
		PUSH_VERT(p0, c0); \
		PUSH_VERT(p1, c1); \
		PUSH_VERT(p2, c2); \
	} while (0)

void cf_batch_quad_verts(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
	PUSH_TRI(p2, p3, p0, c, c, c);
}

void cf_batch_quad_verts2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
	PUSH_TRI(p2, p3, p0, c2, c3, c0);
}

void cf_batch_quad_line(cf_batch_t* b, cf_aabb_t bb, float thickness, cf_color_t c, bool antialias)
{
	cf_v2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_line2(b, verts[0], verts[1], verts[2], verts[3], thickness, c, antialias);
}

void cf_internal_batch_quad_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3, bool antialias)
{
	if (antialias) {
		cf_v2 verts[] = { p0, p1, p2, p3 };
		cf_batch_polyline(b, verts, 4, thickness, c0, true, true, 0);
	} else {
		float sqrt_2 = 1.41421356237f;
		cf_v2 n = cf_v2(sqrt_2, sqrt_2) * thickness;
		cf_v2 q0 = p0 + cf_v2(-n.x, -n.y);
		cf_v2 q1 = p1 + cf_v2( n.x, -n.y);
		cf_v2 q2 = p2 + cf_v2( n.x,  n.y);
		cf_v2 q3 = p3 + cf_v2(-n.x,  n.y);
		cf_batch_quad_verts2(b, p0, p1, q1, q0, c0, c1, c2, c3);
		cf_batch_quad_verts2(b, p1, p2, q2, q1, c0, c1, c2, c3);
		cf_batch_quad_verts2(b, p2, p3, q3, q2, c0, c1, c2, c3);
		cf_batch_quad_verts2(b, p3, p0, q0, q3, c0, c1, c2, c3);
	}
}

void cf_batch_quad_line3(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, cf_color_t c3)
{
	cf_internal_batch_quad_line(b, p0, p1, p2, p3, thickness, c0, c1, c2, c3, false);
}

void cf_batch_quad_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, cf_color_t c, bool antialias)
{
	cf_internal_batch_quad_line(b, p0, p1, p2, p3, thickness, c, c, c, c, antialias);
}

void cf_batch_circle(cf_batch_t* b, cf_v2 p, float r, int iters, cf_color_t c)
{
	cf_v2 prev = cf_v2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		cf_v2 next = cf_from_angle(a) * r;
		cf_batch_tri(b, p + prev, p + next, p, c);
		prev = next;
	}
}

void cf_batch_circle_line(cf_batch_t* batch, cf_v2 p, float r, int iters, float thickness, cf_color_t color, bool antialias)
{
	if (antialias) {
		cf_array<cf_v2> verts(iters, NULL);
		cf_v2 p0 = cf_v2(p.x + r, p.y);
		verts.add(p0);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			cf_v2 n = cf_from_angle(a);
			cf_v2 p1 = p + n * r;
			verts.add(p1);
			p0 = p1;
		}

		cf_batch_polyline(batch, verts.data(), verts.size(), thickness, color, true, true, 3);
	} else {
		float half_thickness = thickness * 0.5f;
		cf_v2 p0 = cf_v2(p.x + r - half_thickness, p.y);
		cf_v2 p1 = cf_v2(p.x + r + half_thickness, p.y);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			cf_v2 n = cf_from_angle(a);
			cf_v2 p2 = p + n * (r + half_thickness);
			cf_v2 p3 = p + n * (r - half_thickness);
			cf_batch_quad_verts(batch, p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_circle_arc(cf_batch_t* batch, cf_v2 p, cf_v2 center_of_arc, float range, int iters, cf_color_t color)
{
	float r = cf_len(center_of_arc - p);
	cf_v2 d = cf_norm(center_of_arc - p);
	cf_sincos_t m = cf_sincos_f(range * 0.5f);

	cf_v2 t = cf_mulT_sc_v2(m, d);
	cf_v2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		cf_v2 p1 = p + t * r;
		cf_batch_tri(batch, p, p1, p0, color);
		p0 = p1;
	}
}

static void cf_s_circle_arc_line_aa(cf_array<cf_v2>* verts, cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, cf_color_t color)
{
	float r = cf_len(center_of_arc - p);
	cf_v2 d = cf_norm(center_of_arc - p);
	cf_sincos_t m = cf_sincos_f(range * 0.5f);

	cf_v2 t = cf_mulT_sc_v2(m, d);
	cf_v2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;
	verts->add(p0);

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		cf_v2 p1 = p + t * r;
		verts->add(p1);
		p0 = p1;
	}
}

void cf_batch_circle_arc_line(cf_batch_t* batch, cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, cf_color_t color, bool antialias)
{
	if (antialias) {
		cf_array<cf_v2> verts(iters, NULL);
		cf_s_circle_arc_line_aa(&verts, p, center_of_arc, range, iters, thickness, color);
		cf_batch_polyline(batch, verts.data(), verts.size(), thickness, color, false, true, 3);
	} else {
		float r = cf_len(center_of_arc - p);
		cf_v2 d = cf_norm(center_of_arc - p);
		cf_sincos_t m = cf_sincos_f(range * 0.5f);

		float half_thickness = thickness * 0.5f;
		cf_v2 t = cf_mulT_sc_v2(m, d);
		cf_v2 p0 = p + t * (r + half_thickness);
		cf_v2 p1 = p + t * (r - half_thickness);
		d = cf_norm(p0 - p);
		float inc = range / iters;

		for (int i = 1; i <= iters; i++) {
			m = cf_sincos_f(i * inc);
			t = cf_mul_sc_v2(m, d);
			cf_v2 p2 = p + t * (r + half_thickness);
			cf_v2 p3 = p + t * (r - half_thickness);
			cf_batch_quad_verts(batch, p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_capsule(cf_batch_t* batch, cf_v2 a, cf_v2 b, float r, int iters, cf_color_t c)
{
	cf_batch_circle_arc(batch, a, a + cf_norm(a - b) * r, CUTE_PI, iters, c);
	cf_batch_circle_arc(batch, b, b + cf_norm(b - a) * r, CUTE_PI, iters, c);
	cf_v2 n = cf_skew(cf_norm(b - a)) * r;
	cf_v2 q0 = a + n;
	cf_v2 q1 = b + n;
	cf_v2 q2 = b - n;
	cf_v2 q3 = a - n;
	cf_batch_quad_verts(batch, q0, q1, q2, q3, c);
}

void cf_batch_capsule_line(cf_batch_t* batch, cf_v2 a, cf_v2 b, float r, int iters, float thickness, cf_color_t c, bool antialias)
{
	if (antialias) {
		cf_array<cf_v2> verts(iters * 2 + 2, NULL);
		cf_s_circle_arc_line_aa(&verts, a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c);
		cf_s_circle_arc_line_aa(&verts, b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c);
		cf_batch_polyline(batch, verts.data(), verts.count(), thickness, c, true, true, 0);
	} else {
		cf_batch_circle_arc_line(batch, a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c, false);
		cf_batch_circle_arc_line(batch, b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c, false);
		cf_v2 n = cf_skew(cf_norm(b - a)) * r;
		cf_v2 q0 = a + n;
		cf_v2 q1 = b + n;
		cf_v2 q2 = b - n;
		cf_v2 q3 = a - n;
		cf_batch_line(batch, q0, q1, thickness, c, false);
		cf_batch_line(batch, q2, q3, thickness, c, false);
	}
}

void cf_batch_tri(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
}

void cf_batch_tri2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_color_t c0, cf_color_t c1, cf_color_t c2)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
}

void cf_batch_tri_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_batch_tri_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, cf_color_t c0, cf_color_t c1, cf_color_t c2, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_batch_line(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c, bool antialias)
{
	cf_batch_line2(b, p0, p1, thickness, c, c, antialias);
}

void cf_batch_line2(cf_batch_t* b, cf_v2 p0, cf_v2 p1, float thickness, cf_color_t c0, cf_color_t c1, bool antialias)
{
	float scale = cf_len(b->m.m.x); // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (antialias) {
		cf_color_t c2 = c0;
		cf_color_t c3 = c1;
		c2.a = 0;
		c3.a = 0;
		if (thick_line) {
			// Core center line.
			float core_half_width = (thickness - alias_scale) * 0.5f;
			cf_v2 n0 = cf_norm(p1 - p0);
			cf_v2 n1 = cf_skew(n0) * core_half_width;
			cf_v2 q0 = p0 + n1;
			cf_v2 q1 = p1 + n1;
			cf_v2 q2 = p1 - n1;
			cf_v2 q3 = p0 - n1;
			cf_batch_quad_verts2(b, q0, q1, q2, q3, c0, c0, c1, c1);

			// Zero opacity aliased quads.
			cf_v2 n2 = cf_cw90(n0) * alias_scale;
			cf_v2 q4 = q3 + n2;
			cf_v2 q5 = q2 + n2;
			cf_v2 q6 = q1 - n2;
			cf_v2 q7 = q0 - n2;
			cf_batch_quad_verts2(b, q3, q2, q5, q4, c0, c1, c3, c2);
			cf_batch_quad_verts2(b, q0, q7, q6, q1, c0, c2, c3, c1);

			// End caps.
			n0 = n0 * alias_scale;
			cf_v2 r0 = q5 + n0;
			cf_v2 r1 = q2 + n0;
			cf_v2 r2 = q1 + n0;
			cf_v2 r3 = q6 + n0;
			cf_batch_quad_verts2(b, q2, r1, r0, q5, c1, c3, c3, c3);
			cf_batch_quad_verts2(b, q2, q1, r2, r1, c1, c1, c3, c3);
			cf_batch_quad_verts2(b, q1, q6, r3, r2, c1, c3, c3, c3);

			cf_v2 r4 = q4 - n0;
			cf_v2 r5 = q3 - n0;
			cf_v2 r6 = q0 - n0;
			cf_v2 r7 = q7 - n0;
			cf_batch_quad_verts2(b, q3, r5, r4, q4, c0, c2, c2, c2);
			cf_batch_quad_verts2(b, q3, q0, r6, r5, c0, c0, c2, c2);
			cf_batch_quad_verts2(b, q0, q7, r7, r6, c0, c2, c2, c2);
		} else {
			// Zero opacity aliased quads, without any core line.
			cf_v2 n = cf_skew(cf_norm(p1 - p0)) * alias_scale * 0.5f;
			cf_v2 q0 = p0 + n;
			cf_v2 q1 = p1 + n;
			cf_v2 q2 = p1 - n;
			cf_v2 q3 = p0 - n;
			cf_batch_quad_verts2(b, p0, p1, q1, q0, c0, c1, c3, c2);
			cf_batch_quad_verts2(b, p1, p0, q3, q2, c1, c0, c3, c2);
		}
	} else {
		cf_v2 n = cf_skew(cf_norm(p1 - p0)) * thickness * 0.5f;
		cf_v2 q0 = p0 + n;
		cf_v2 q1 = p1 + n;
		cf_v2 q2 = p1 - n;
		cf_v2 q3 = p0 - n;
		cf_batch_quad_verts2(b, q0, q1, q2, q3, c0, c0, c1, c1);
	}
}

CUTE_INLINE static cf_v2 cf_s_rot_b_about_a(cf_sincos_t r, cf_v2 b, cf_v2 a)
{
	cf_v2 result = cf_mul_sc_v2(r, a - b);
	return result + b;
}

CUTE_INLINE static void cf_s_bevel_arc_feather(cf_batch_t* batch, cf_v2 b, cf_v2 i3, cf_v2 f3, cf_v2 i4, cf_v2 f4, cf_color_t c0, cf_color_t c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	cf_sincos_t r = cf_sincos_f(arc);
	cf_v2 p0 = i3;
	cf_v2 p1 = f3;
	for (int i = 1; i < bevel_count; ++i) {
		cf_v2 p2 = cf_s_rot_b_about_a(r, b, p1);
		cf_v2 p3 = cf_s_rot_b_about_a(r, b, p0);
		cf_batch_tri(batch, b, p0, p3, c0);
		cf_batch_quad_verts2(batch, p3, p2, p1, p0, c0, c1, c1, c0);
		p0 = p3;
		p1 = p2;
	}
	cf_batch_tri(batch, b, i4, p0, c0);
	cf_batch_quad_verts2(batch, p0, i4, f4, p1, c0, c0, c1, c1);
}

CUTE_INLINE static void cf_s_bevel_arc(cf_batch_t* batch, cf_v2 b, cf_v2 i3, cf_v2 i4, cf_color_t c0, cf_color_t c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	cf_sincos_t r = cf_sincos_f(arc);
	cf_v2 p0 = i3;
	for (int i = 1; i < bevel_count; ++i) {
		cf_v2 p3 = cf_s_rot_b_about_a(r, b, p0);
		cf_batch_tri(batch, b, p0, p3, c0);
		p0 = p3;
	}
	cf_batch_tri(batch, b, i4, p0, c0);
}

static void cf_s_polyline(cf_batch_t* batch, cf_v2* points, int count, float thickness, cf_color_t c0, cf_color_t c1, bool loop, bool feather, float alias_scale, int bevel_count)
{
	float inner_half = (thickness - alias_scale) * 0.5f;
	float outer_half = inner_half + alias_scale;
	int iter = 0;
	int i = 2;
	cf_v2 a = points[0];
	cf_v2 b = points[1];
	cf_v2 n0 = cf_skew(cf_norm(b - a)) * inner_half;
	cf_v2 i0 = a + n0;
	cf_v2 i1 = a - n0;
	cf_v2 fn0 = cf_norm(n0) * outer_half;
	cf_v2 f0 = a + fn0;
	cf_v2 f1 = a - fn0;
	int end = count;

	// Optionally emits geometry about each corner of the polyline, and sets up i0, i1, f0, f1, n0 and fn0.
	auto do_polyline_corner = [&](cf_v2 c, bool emit) {
		cf_v2 n1 = cf_skew(cf_norm(c - b)) * inner_half;
		cf_v2 fn1 = cf_norm(n1) * outer_half;
		float ab_x_bc = cf_cross(b - a, c - b);
		float d = cf_dot(cf_cw90(n0), cf_cw90(n1));
		const float k_tol = 1.e-6f;

		if (ab_x_bc < -k_tol) {
			if (d >= 0) {
				cf_v2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b - n0), b - n1, c - n1);
				cf_v2 i3 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				if (feather) {
					cf_v2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b - fn0), b - fn1, c - fn1);
					cf_v2 f3 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					if (emit) {
						cf_batch_quad_verts(batch, a, b, i3, i0, c0);
						cf_batch_quad_verts(batch, i1, i2, b, a, c0);
						cf_batch_quad_verts2(batch, i0, i3, f3, f0, c0, c0, c1, c1);
						cf_batch_quad_verts2(batch, f1, f2, i2, i1, c1, c1, c0, c0);
					}
					f0 = f3;
					f1 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(batch, a, b, i3, i0, c0, c0, c1, c1);
					cf_batch_quad_verts2(batch, i1, i2, b, a, c1, c1, c0, c0);
				}
				i0 = i3;
				i1 = i2;
			} else {
				cf_v2 i2 = cf_intersect_halfspace2(cf_plane2(-n0, b - n0), b - n1, c - n1);
				cf_v2 i3 = b + n0;
				cf_v2 i4 = b + n1;
				if (feather) {
					cf_v2 f2 = cf_intersect_halfspace2(cf_plane2(-fn0, b - fn0), b - fn1, c - fn1);
					cf_v2 n = cf_norm(n0 + n1);
					cf_halfspace_t h = cf_plane2(n, i3 + n * alias_scale);
					cf_v2 f3 = cf_intersect_halfspace2(h, a + fn0, b + fn0);
					cf_v2 f4 = cf_intersect_halfspace2(h, b + fn1, c + fn1);
					if (emit) {
						cf_batch_quad_verts(batch, a, b, i3, i0, c0);
						cf_batch_quad_verts(batch, i1, i2, b, a, c0);
						cf_batch_quad_verts2(batch, i0, i3, f3, f0, c0, c0, c1, c1);
						cf_batch_quad_verts2(batch, i1, f1, f2, i2, c0, c1, c1, c0);
						cf_s_bevel_arc_feather(batch, b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f0 = f4;
					f1 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(batch, a, b, i3, i0, c0, c0, c1, c1);
					cf_batch_quad_verts2(batch, i1, i2, b, a, c1, c1, c0, c0);
					cf_s_bevel_arc(batch, b, i3, i4, c0, c1, bevel_count);
				}
				i0 = i4;
				i1 = i2;
			}
		} else if (ab_x_bc > k_tol) {
			if (d >= 0) {
				cf_v2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				cf_v2 i3 = cf_intersect_halfspace2(cf_plane2(n0, b - n0), b - n1, c - n1);
				if (feather) {
					cf_v2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					cf_v2 f3 = cf_intersect_halfspace2(cf_plane2(fn0, b - fn0), b - fn1, c - fn1);
					if (emit) {
						cf_batch_quad_verts(batch, a, b, i3, i1, c0);
						cf_batch_quad_verts(batch, i0, i2, b, a, c0);
						cf_batch_quad_verts2(batch, i1, i3, f3, f1, c0, c0, c1, c1);
						cf_batch_quad_verts2(batch, f0, f2, i2, i0, c1, c1, c0, c0);
					}
					f1 = f3;
					f0 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(batch, a, b, i3, i1, c0, c0, c1, c1);
					cf_batch_quad_verts2(batch, i0, i2, b, a, c1, c1, c0, c0);
				}
				i1 = i3;
				i0 = i2;
			} else {
				cf_v2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				cf_v2 i3 = b - n0;
				cf_v2 i4 = b - n1;
				if (feather) {
					cf_v2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					cf_v2 n = cf_norm(n0 + n1);
					cf_halfspace_t h = cf_plane2(-n, i3 - n * alias_scale);
					cf_v2 f3 = cf_intersect_halfspace2(h, a - fn0, b - fn0);
					cf_v2 f4 = cf_intersect_halfspace2(h, b - fn1, c - fn1);
					if (emit) {
						cf_batch_quad_verts(batch, a, b, i3, i1, c0);
						cf_batch_quad_verts(batch, i0, i2, b, a, c0);
						cf_batch_quad_verts2(batch, i1, i3, f3, f1, c0, c0, c1, c1);
						cf_batch_quad_verts2(batch, i0, f0, f2, i2, c0, c1, c1, c0);
						cf_s_bevel_arc_feather(batch, b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f1 = f4;
					f0 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(batch, a, b, i3, i1, c0, c0, c1, c1);
					cf_batch_quad_verts2(batch, i0, i2, b, a, c1, c1, c0, c0);
					cf_s_bevel_arc(batch, b, i3, i4, c0, c1, bevel_count);
				}
				i1 = i4;
				i0 = i2;
			}
		} else {
			// Parallel, sin(angle_of_corner) within [-k_tol, k_tol].
			cf_v2 i2 = b + n0;
			cf_v2 i3 = b - n0;
			if (feather) {
				cf_v2 f2 = b + fn0;
				cf_v2 f3 = b - fn0;
				if (emit) {
					cf_batch_quad_verts(batch, a, b, i2, i0, c0);
					cf_batch_quad_verts(batch, i1, i3, b, a, c0);
					cf_batch_quad_verts2(batch, i0, i2, f2, f0, c0, c0, c1, c1);
					cf_batch_quad_verts2(batch, i1, f1, f3, i3, c0, c1, c1, c0);
				}
				f1 = f3;
				f0 = f2;
			} else if (emit) {
				cf_batch_quad_verts2(batch, a, b, i2, i0, c0, c0, c1, c1);
				cf_batch_quad_verts2(batch, i1, i3, b, a, c1, c1, c0, c0);
			}
			i1 = i3;
			i0 = i2;
		}
		n0 = n1;
		fn0 = fn1;
		a = b;
		b = c;
	};

	// Special case for the first iteration -- previous endpoints i0, i1, f0, f1 and accompanying
	// normals n0, fn0 need to be pre-computed, as each loop iteration of do_polyline_corner
	// expects these values to be setup properly. We backup the indices for points a, b, an c by
	// -1 then call do_polyline_corner (without rendering any geometry), which calculates correct
	// initial values for the next call to do_polyline_corner.
	if (loop) {
		a = points[count - 1];
		b = points[0];
		cf_v2 c = points[1];
		n0 = cf_skew(cf_norm(b - a)) * inner_half;
		fn0 = cf_norm(n0) * outer_half;
		do_polyline_corner(c, false);
	} else {
		end -= 2;
	}

	// Main loop, emit geometry about each corner of the polyline.
	do {
		cf_v2 c = points[i];
		do_polyline_corner(c, true);
		iter++;
		i = i + 1 == count ? 0 : i + 1;
	} while (iter < end);

	// End case for non-loops.
	if (!loop) {
		if (feather) {
			cf_batch_quad_verts(batch, a, b, b + n0, i0, c0);
			cf_batch_quad_verts(batch, a, i1, b - n0, b, c0);
			cf_batch_quad_verts2(batch, f0, i0, b + n0, b + fn0, c1, c0, c0, c1);
			cf_batch_quad_verts2(batch, b - fn0, b - n0, i1, f1, c1, c0, c0, c1);

			// End caps.
			cf_v2 n = cf_norm(b - a) * alias_scale;
			cf_batch_quad_verts2(batch, b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_batch_quad_verts2(batch, b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_batch_quad_verts2(batch, b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);

			a = points[1];
			b = points[0];
			n = cf_norm(b - a) * alias_scale;
			n0 = cf_skew(cf_norm(b - a)) * inner_half;
			fn0 = cf_norm(n0) * outer_half;
			cf_batch_quad_verts2(batch, b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_batch_quad_verts2(batch, b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_batch_quad_verts2(batch, b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);
		} else {
			cf_batch_quad_verts2(batch, a, b, b + n0, i0, c0, c0, c1, c1);
			cf_batch_quad_verts2(batch, a, i1, b - n0, b, c0, c1, c1, c0);
		}
	}
}

void cf_batch_polyline(cf_batch_t* batch, cf_v2* points, int count, float thickness, cf_color_t color, bool loop, bool antialias, int bevel_count)
{
	CUTE_ASSERT(count >= 3);
	float scale = cf_len(batch->m.m.x); // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (antialias) {
		cf_color_t no_alpha = color;
		no_alpha.a = 0;
		if (thick_line) {
			cf_s_polyline(batch, points, count, thickness, color, no_alpha, loop, true, alias_scale, bevel_count);
		} else {
			cf_s_polyline(batch, points, count, alias_scale, color, no_alpha, loop, false, 0, bevel_count);
		}
	} else {
		cf_s_polyline(batch, points, count, thickness, color, color, loop, false, 0, bevel_count);
	}
}

cf_temporary_image_t cf_batch_fetch(cf_batch_t* b, cf_batch_sprite_t sprite)
{
	spritebatch_sprite_t s = spritebatch_fetch(&b->sb, sprite.id, sprite.w, sprite.h);
	cf_temporary_image_t image;
	image.texture_id = s.texture_id;
	image.w = s.w;
	image.h = s.h;
	image.u = cf_v2(s.minx, s.miny);
	image.v = cf_v2(s.maxx, s.maxy);
	return image;
}
