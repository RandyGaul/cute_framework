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

struct quad_udata_t
{
	float alpha;
};

#include <cute/cute_png.h>

#define SPRITEBATCH_SPRITE_USERDATA quad_udata_t
#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

namespace cute
{

struct quad_vertex_t
{
	v2 pos;
	v2 uv;
	float alpha;
};

struct vertex_t
{
	v2 p;
	color_t c;
};


enum batch_sprite_shader_type_t
{
	BATCH_SPRITE_SHADER_TYPE_DEFAULT,
	BATCH_SPRITE_SHADER_TYPE_OUTLINE,
};

struct scissor_t
{
	int x, y, w, h;
};

static const color_t DEFAULT_TINT = make_color(0.5f, 0.5f, 0.5f, 1.0f);

struct batch_t
{
	::spritebatch_t sb;

	sg_pipeline geom_pip;
	triple_buffer_t geom_buffer;
	array<vertex_t> geom_verts;

	array<quad_vertex_t> sprite_verts;
	triple_buffer_t sprite_buffer;
	sg_shader default_shd = { 0 };
	sg_shader outline_shd = { 0 };
	sg_shader active_shd = { 0 };
	batch_sprite_shader_type_t sprite_shd_type = BATCH_SPRITE_SHADER_TYPE_DEFAULT;
	bool pip_dirty = true;
	sg_pipeline pip = { 0 };
	float outline_use_border = 1.0f;
	float outline_use_corners = 0;
	color_t outline_color = color_white();
	matrix_t projection;

	array<m3x2> m3x2s;
	array<sg_blend_state> blend_states;
	array<sg_depth_state> depth_states;
	array<sg_stencil_state> stencil_states;
	array<scissor_t> scissors;
	array<color_t> tints = { DEFAULT_TINT };

	get_pixels_fn* get_pixels = NULL;
	void* get_pixels_udata = NULL;

	void* mem_ctx = NULL;
};

//--------------------------------------------------------------------------------------------------
// Internal functions.

static sg_shader s_load_shader(batch_t* b, batch_sprite_shader_type_t type)
{
	sg_shader_desc params = { 0 };

	// Default sprite shader.
	switch (type) {
	case BATCH_SPRITE_SHADER_TYPE_DEFAULT:
		params = *sprite_default_shd_shader_desc(sg_query_backend());
		break;

	default:
		params = *sprite_outline_shd_shader_desc(sg_query_backend());
		break;
	}

	sg_shader shader = sg_make_shader(params);
	return shader;
}

static void s_set_shd_type(batch_t* b, batch_sprite_shader_type_t type)
{
	b->sprite_shd_type = type;

	// Load the shader, if needed, and set the active sprite system shader.
	switch (type)
	{
	case BATCH_SPRITE_SHADER_TYPE_DEFAULT:
		if (b->default_shd.id == SG_INVALID_ID) {
			b->default_shd = s_load_shader(b, type);
		}
		b->active_shd = b->default_shd;
		break;

	case BATCH_SPRITE_SHADER_TYPE_OUTLINE:
		if (b->outline_shd.id == SG_INVALID_ID) {
			b->outline_shd = s_load_shader(b, type);
		}
		b->active_shd = b->outline_shd;
		break;
	}

	b->pip_dirty = true;
}

//--------------------------------------------------------------------------------------------------
// spritebatch_t callbacks.

static void s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	batch_t* b = (batch_t*)udata;

	// Build vertex buffer of all quads for each sprite.
	int vert_count = count * 6;
	b->sprite_verts.ensure_count(vert_count);
	quad_vertex_t* verts = b->sprite_verts.data();

	m3x2 m = make_identity();
	if (b->m3x2s.count()) {
		m = b->m3x2s.last();
	}

	for (int i = 0; i < count; ++i)
	{
		spritebatch_sprite_t* s = sprites + i;

		v2 quad[] = {
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
			// TODO - This in shaders.
			v2 p = v2(x, y);
			p = mul(m, p);

			quad[j].x = p.x;
			quad[j].y = p.y;
		}

		// output transformed quad into CPU buffer
		quad_vertex_t* out_verts = verts + i * 6;

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
	error_t err = triple_buffer_append(&b->sprite_buffer, vert_count, verts);
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
	case BATCH_SPRITE_SHADER_TYPE_DEFAULT:
	{
		sprite_default_vs_params_t vs_params = {
			b->projection
		};
		sprite_default_fs_params_t fs_params = {
			b->tints.last()
		};
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(vs_params));
		sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(fs_params));
	}	break;

	case BATCH_SPRITE_SHADER_TYPE_OUTLINE:
	{
		sprite_outline_vs_params_t vs_params = {
			b->projection,
		};
		sprite_outline_fs_params_t fs_params = {
			b->tints.last(),
			b->outline_color,
			v2(1.0f / (float)texture_w, 1.0f / (float)texture_h),
			b->outline_use_border,
			b->outline_use_corners
		};
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(vs_params));
		sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(fs_params));
	}	break;
	}

	// Kick off a draw call.
	sg_draw(0, vert_count, 1);
}

static void s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	batch_t* b = (batch_t*)udata;
	b->get_pixels(image_id, buffer, bytes_to_fill, b->get_pixels_udata);
}

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	batch_t* b = (batch_t*)udata;
	return texture_make((pixel_t*)pixels, w, h);
}

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	batch_t* b = (batch_t*)udata;
	texture_destroy(texture_id);
}

//--------------------------------------------------------------------------------------------------

static void s_sync_pip(batch_t* b)
{
	if (b->pip_dirty) {
		if (b->pip.id != SG_INVALID_ID) sg_destroy_pipeline(b->pip);
		sg_pipeline_desc params = { 0 };
		params.layout.buffers[0].stride = sizeof(quad_vertex_t);
		params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
		params.layout.buffers[0].step_rate = 1;
		params.layout.attrs[0].buffer_index = 0;
		params.layout.attrs[0].offset = CUTE_OFFSET_OF(quad_vertex_t, pos);
		params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[1].buffer_index = 0;
		params.layout.attrs[1].offset = CUTE_OFFSET_OF(quad_vertex_t, uv);
		params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
		params.layout.attrs[2].buffer_index = 0;
		params.layout.attrs[2].offset = CUTE_OFFSET_OF(quad_vertex_t, alpha);
		params.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT;
		params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
		params.shader = b->active_shd;
		params.stencil = b->stencil_states.last();
		params.colors[0].blend = b->blend_states.last();
		b->pip = sg_make_pipeline(params);
		b->pip_dirty = false;
	}
}

batch_t* batch_make(get_pixels_fn* get_pixels, void* get_pixels_udata, void* mem_ctx)
{
	batch_t* b = CUTE_NEW(batch_t, app->mem_ctx);
	if (!b) return NULL;

	b->projection = matrix_identity();
	b->get_pixels = get_pixels;
	b->get_pixels_udata = get_pixels_udata;
	b->mem_ctx = mem_ctx;

	sg_pipeline_desc params = { 0 };
	params.layout.buffers[0].stride = sizeof(vertex_t);
	params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	params.layout.buffers[0].step_rate = 1;
	params.layout.attrs[0].buffer_index = 0;
	params.layout.attrs[0].offset = CUTE_OFFSET_OF(vertex_t, p);
	params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	params.layout.attrs[1].buffer_index = 0;
	params.layout.attrs[1].offset = CUTE_OFFSET_OF(vertex_t, c);
	params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
	params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	params.shader = sg_make_shader(geom_shd_shader_desc(sg_query_backend()));
	params.colors[0].blend.enabled = true;
	params.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	params.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	params.colors[0].blend.op_rgb = SG_BLENDOP_ADD;
	params.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	params.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	params.colors[0].blend.op_alpha = SG_BLENDOP_ADD;
	b->geom_pip = sg_make_pipeline(params);
	b->geom_buffer = triple_buffer_make(sizeof(vertex_t) * 1024 * 10, sizeof(vertex_t));

	batch_push_stencil_defaults(b);
	batch_push_blend_defaults(b);
	b->default_shd = b->active_shd = s_load_shader(b, BATCH_SPRITE_SHADER_TYPE_DEFAULT);
	b->outline_shd = s_load_shader(b, BATCH_SPRITE_SHADER_TYPE_OUTLINE);
	b->sprite_buffer = triple_buffer_make(sizeof(quad_vertex_t) * 1024 * 10, sizeof(quad_vertex_t));

	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_batch_report;
	config.get_pixels_callback = s_get_pixels;
	config.generate_texture_callback = s_generate_texture_handle;
	config.delete_texture_callback = s_destroy_texture_handle;
	config.allocator_context = b->mem_ctx;
	config.lonely_buffer_count_till_flush = 0;

	if (spritebatch_init(&b->sb, &config, b)) {
		CUTE_FREE(b, app->mem_ctx);
		if (!b) return NULL;
	}

	return b;
}

void batch_destroy(batch_t* b)
{
	spritebatch_term(&b->sb);
	b->~batch_t();
	CUTE_FREE(b, b->mem_ctx);
}

void batch_push(batch_t* b, batch_sprite_t q)
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

error_t batch_flush(batch_t* b)
{
	// Draw sprites.
	s_sync_pip(b);
	sg_apply_pipeline(b->pip);

	if (b->scissors.count()) {
		scissor_t scissor = b->scissors.last();
		sg_apply_scissor_rect(scissor.x, scissor.y, scissor.w, scissor.h, false);
	}

	spritebatch_tick(&b->sb);
	if (!spritebatch_defrag(&b->sb)) {
		return error_failure("`spritebatch_defrag` failed.");
	}
	spritebatch_flush(&b->sb);

	b->sprite_buffer.advance();

	// Draw geometry.
	if (b->geom_verts.count()) {
		// Transform verts.
		m3x2 m = make_identity();
		if (b->m3x2s.count()) {
			m = b->m3x2s.last();
		}
		for (int i = 0; i < b->geom_verts.count(); ++i) {
			b->geom_verts[i].p = mul(m, b->geom_verts[i].p);
		}

		// Issue draw call.
		sg_apply_pipeline(b->geom_pip);
		geom_vs_params_t params;
		params.u_mvp = b->projection;
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(params));
		error_t err = triple_buffer_append(&b->geom_buffer, b->geom_verts.count(), b->geom_verts.data());
		CUTE_ASSERT(!err.is_error());
		sg_apply_bindings(b->geom_buffer.bind());
		sg_draw(0, b->geom_verts.count(), 1);
		b->geom_verts.clear();
		b->geom_buffer.advance();
	}

	return error_success();
}

//--------------------------------------------------------------------------------------------------

void batch_set_projection(batch_t* b, matrix_t projection)
{
	b->projection = projection;
}

void batch_outlines(batch_t* b, bool use_outlines)
{
	b->outline_use_border = use_outlines ? 1.0f : 0;
	if (use_outlines) {
		s_set_shd_type(b, BATCH_SPRITE_SHADER_TYPE_OUTLINE);
	} else {
		s_set_shd_type(b, BATCH_SPRITE_SHADER_TYPE_DEFAULT);
	}
}

void batch_outlines_use_corners(batch_t* b, bool use_corners)
{
	b->outline_use_corners = use_corners ? 1.0f : 0;
}

void batch_outlines_color(batch_t* b, color_t c)
{
	b->outline_color = c;
}

void batch_push_m3x2(batch_t* b, m3x2 m)
{
	b->m3x2s.add(m);
}

void batch_pop_m3x2(batch_t* b)
{
	if (b->m3x2s.count()) {
		b->m3x2s.pop();
	}
}

void batch_push_scissor_box(batch_t* b, int x, int y, int w, int h)
{
	scissor_t scissor = { x, y, w, h };
	b->scissors.add(scissor);
}

void batch_pop_scissor_box(batch_t* b)
{
	if (b->scissors.count()) {
		b->scissors.pop();
	}
}

void batch_push_depth_state(batch_t* b, const sg_depth_state& depth_state)
{
	b->depth_states.add(depth_state);
	b->pip_dirty = true;
}

void batch_push_depth_defaults(batch_t* b)
{
	sg_depth_state depth_state;
	CUTE_MEMSET(&depth_state, 0, sizeof(depth_state));
	batch_push_depth_state(b, depth_state);
}

void batch_pop_depth_state(batch_t* b)
{
	if (b->depth_states.count() > 1) {
		b->depth_states.pop();
		b->pip_dirty = true;
	}
}

void batch_push_stencil_state(batch_t* b, const sg_stencil_state& stencil_state)
{
	b->stencil_states.add(stencil_state);
	b->pip_dirty = true;
}

void batch_push_stencil_defaults(batch_t* b)
{
	sg_stencil_state stencil_state;
	CUTE_MEMSET(&stencil_state, 0, sizeof(stencil_state));
	batch_push_stencil_state(b, stencil_state);
}

void batch_pop_stencil_state(batch_t* b)
{
	if (b->stencil_states.count() > 1) {
		b->stencil_states.pop();
		b->pip_dirty = true;
	}
}

void batch_push_blend_state(batch_t* b, const sg_blend_state& blend_state)
{
	b->blend_states.add(blend_state);
	b->pip_dirty = true;
}

void batch_push_blend_defaults(batch_t* b)
{
	sg_blend_state blend_state;
	CUTE_MEMSET(&blend_state, 0, sizeof(blend_state));
	blend_state.enabled = true;
	blend_state.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	blend_state.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	blend_state.op_rgb = SG_BLENDOP_ADD;
	blend_state.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	blend_state.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	blend_state.op_alpha = SG_BLENDOP_ADD;
	batch_push_blend_state(b, blend_state);
}

void batch_pop_blend_state(batch_t* b)
{
	if (b->blend_states.count() > 1) {
		b->blend_states.pop();
		b->pip_dirty = true;
	}
}

void batch_push_tint(batch_t* b, color_t c)
{
	b->tints.add(c);
}

void batch_pop_tint(batch_t* b)
{
	if (b->tints.count() > 1) {
		b->tints.pop();
	}
}

void batch_quad(batch_t* b, aabb_t bb, color_t c)
{
	v2 verts[4];
	aabb_verts(verts, bb);
	batch_quad(b, verts[0], verts[1], verts[2], verts[3], c);
}

#define PUSH_VERT(P, C) \
	do { \
		vertex_t v; \
		v.p = P; \
		v.c = C; \
		b->geom_verts.add(v); \
	} while (0)

#define PUSH_TRI(p0, p1, p2, c0, c1, c2) \
	do { \
		PUSH_VERT(p0, c0); \
		PUSH_VERT(p1, c1); \
		PUSH_VERT(p2, c2); \
	} while (0)

void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
	PUSH_TRI(p2, p3, p0, c, c, c);
}

void batch_quad(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, color_t c0, color_t c1, color_t c2, color_t c3)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
	PUSH_TRI(p2, p3, p0, c0, c1, c2);
}

void batch_quad_line(batch_t* b, aabb_t bb, float thickness, color_t c)
{
	v2 verts[4];
	aabb_verts(verts, bb);
	batch_quad_line(b, verts[0], verts[1], verts[2], verts[3], thickness, c);
}

void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c)
{
	batch_quad_line(b, p0, p1, p2, p3, thickness, c, c, c, c);
}

void batch_quad_line(batch_t* b, v2 p0, v2 p1, v2 p2, v2 p3, float thickness, color_t c0, color_t c1, color_t c2, color_t c3)
{
	float sqrt_2 = 1.41421356237f;
	v2 n = v2(sqrt_2, sqrt_2) * thickness;
	v2 q0 = p0 + v2(-n.x, -n.y);
	v2 q1 = p1 + v2( n.x, -n.y);
	v2 q2 = p2 + v2( n.x,  n.y);
	v2 q3 = p3 + v2(-n.x,  n.y);
	batch_quad(b, p0, p1, q1, q0, c0, c1, c2, c3);
	batch_quad(b, p1, p2, q2, q1, c0, c1, c2, c3);
	batch_quad(b, p2, p3, q3, q2, c0, c1, c2, c3);
	batch_quad(b, p3, p0, q0, q3, c0, c1, c2, c3);
}

void batch_circle(batch_t* b, v2 p, float r, int iters, color_t c)
{
	v2 prev = v2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		v2 next = from_angle(a) * r;
		batch_tri(b, p + prev, p + next, p, c);
		prev = next;
	}
}

void batch_circle_line(batch_t* batch, v2 p, float r, int iters, float thickness, color_t color)
{
	v2 p0 = v2(p.x + r - thickness, p.y);
	v2 p1 = v2(p.x + r, p.y);
	float half_thickness = thickness * 0.5f;

	for (int i = 1; i <= iters; i++) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		v2 n = from_angle(a);
		v2 p2 = p + n * (r + half_thickness);
		v2 p3 = p + n * (r - half_thickness);
		batch_quad(batch, p0, p1, p2, p3, color);
		p1 = p2;
		p0 = p3;
	}
}

void batch_circle_arc(batch_t* batch, v2 p, v2 center_of_arc, float range, int iters, color_t color)
{
	float r = len(center_of_arc - p);
	v2 d = norm(center_of_arc - p);
	sincos_t m = sincos(range * 0.5f);

	v2 t = mulT(m, d);
	v2 p0 = p + t * r;
	d = norm(p0 - p);
	float inc = range / iters;

	for (int i = 1; i <= iters; i++) {
		m = sincos(i * inc);
		t = mul(m, d);
		v2 p1 = p + t * r;
		batch_tri(batch, p, p1, p0, color);
		p0 = p1;
	}
}

void batch_circle_arc_line(batch_t* batch, v2 p, v2 center_of_arc, float range, int iters, float thickness, color_t color)
{
	float r = len(center_of_arc - p);
	v2 d = norm(center_of_arc - p);
	sincos_t m = sincos(range * 0.5f);

	float half_thickness = thickness * 0.5f;
	v2 t = mulT(m, d);
	v2 p0 = p + t * (r + half_thickness);
	v2 p1 = p + t * (r - half_thickness);
	d = norm(p0 - p);
	float inc = range / iters;

	for (int i = 1; i <= iters; i++) {
		m = sincos(i * inc);
		t = mul(m, d);
		v2 p2 = p + t * (r + half_thickness);
		v2 p3 = p + t * (r - half_thickness);
		batch_quad(batch, p0, p1, p2, p3, color);
		p1 = p2;
		p0 = p3;
	}
}

void batch_capsule(batch_t* batch, v2 a, v2 b, float r, int iters, color_t c)
{
	batch_circle_arc(batch, a, a + norm(a - b) * r, CUTE_PI, iters, c);
	batch_circle_arc(batch, b, b + norm(b - a) * r, CUTE_PI, iters, c);
	v2 n = skew(norm(b - a)) * r;
	v2 q0 = a + n;
	v2 q1 = b + n;
	v2 q2 = b - n;
	v2 q3 = a - n;
	batch_quad(batch, q0, q1, q2, q3, c);
}

void batch_capsule_line(batch_t* batch, v2 a, v2 b, float r, int iters, float thickness, color_t c)
{
	batch_circle_arc_line(batch, a, a + norm(a - b) * r, CUTE_PI, iters, thickness, c);
	batch_circle_arc_line(batch, b, b + norm(b - a) * r, CUTE_PI, iters, thickness, c);
	v2 n = skew(norm(b - a)) * r * thickness * 0.5f;
	v2 q0 = a + n;
	v2 q1 = b + n;
	v2 q2 = b - n;
	v2 q3 = a - n;
	batch_line(batch, q0, q1, thickness, c);
	batch_line(batch, q2, q3, thickness, c);
}

void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
}

void batch_tri(batch_t* b, v2 p0, v2 p1, v2 p2, color_t c0, color_t c1, color_t c2)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
}

void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, float thickness, color_t c)
{
	CUTE_ASSERT(0);
}

void batch_tri_line(batch_t* b, v2 p0, v2 p1, v2 p2, float thickness, color_t c0, color_t c1, color_t c2)
{
	CUTE_ASSERT(0);
}

void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c)
{
	batch_line(b, p0, p1, thickness, c, c);
}

void batch_line(batch_t* b, v2 p0, v2 p1, float thickness, color_t c0, color_t c1)
{
	v2 n = skew(norm(p1 - p0)) * thickness * 0.5f;
	v2 q0 = p0 + n;
	v2 q1 = p1 + n;
	v2 q2 = p1 - n;
	v2 q3 = p0 - n;
	batch_quad(b, q0, q1, q2, q3, c0, c0, c1, c1);
}

}
