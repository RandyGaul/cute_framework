/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpositione,
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
#include <cute_defer.h>
//#include <cute_debug_printf.h>

#include <internal/cute_app_internal.h>

#include <shaders/sprite_shader.h>

static struct CF_Batch* b = NULL;

struct QuadUdata
{
	float alpha;
};

#include <cute/cute_png.h>

#define SPRITEBATCH_SPRITE_USERDATA QuadUdata
#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define DEBUG_VERT(v, c) batch_quad(make_aabb(v, 3, 3), c)

// Initial design of this batcher comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_batch.h

using namespace cute;

struct Scissor
{
	int x, y, w, h;
};

static const CF_Color DEFAULT_TINT = cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f);

struct BatchVertex
{
	v2 position;
	v2 uv;
	CF_Pixel color;
	uint8_t solid;
	uint8_t outline;
	uint8_t alpha;
	uint8_t unused1;
};

typedef struct BatchSprite
{
	uint64_t id;

	cf_transform_t transform; // Position and location rotation of the quad. Default: cf_make_transform() 
	int w; // Width in pixels of the source image.
	int h; // Height in pixels of the source image.
	float scale_x; // Scaling along the quad's local x-axis in pixels.
	float scale_y; // Scaling along the quad's local y-axis in pixels.
	float alpha; // Applies additional alpha to this quad. Default: 1.0f

	int sort_bits; /*= 0;*/
} BatchSprite;

struct CF_Batch
{
	::spritebatch_t sb;

	float atlas_width = 1024;
	float atlas_height = 1024;

	array<BatchVertex> verts;
	CF_Shader shader;
	CF_Mesh mesh;
	CF_Material material;
	float outline_use_border = 1.0f;
	float outline_use_corners = 0;
	CF_Color outline_color = cf_color_white();
	CF_WrapMode wrap_mode = CF_WRAP_MODE_REPEAT;
	CF_Filter filter = CF_FILTER_NEAREST;
	CF_Matrix4x4 projection;

	cf_m3x2 m = cf_make_identity();
	float scale_x = 1.0f;
	float scale_y = 1.0f;
	array<cf_m3x2> m3x2s;
	array<Scissor> scissors;
	array<CF_Color> tints = { DEFAULT_TINT };

	get_pixels_fn* get_pixels = NULL;
	void* get_pixels_udata = NULL;

	void* mem_ctx = NULL;
};

static void s_push(BatchSprite q)
{
	spritebatch_sprite_t s;
	s.image_id = q.id;
	s.w = q.w;
	s.h = q.h;
	q.transform.p = cf_mul_m32_v2(b->m, q.transform.p);
	s.x = q.transform.p.x;
	s.y = q.transform.p.y;
	s.sx = q.scale_x * b->scale_x;
	s.sy = q.scale_y * b->scale_y;
	s.s = q.transform.r.s;
	s.c = q.transform.r.c;
	s.sort_bits = (uint64_t)q.sort_bits << 32;
	s.udata.alpha = q.alpha;
	spritebatch_push(&b->sb, s);
}

void cf_sprite_batch_sprite(CF_Sprite sprite)
{
	BatchSprite q;
	q.id = sprite.animation->frames[sprite.frame_index].id;
	q.transform = sprite.transform;
	q.transform.p = cf_add_v2(q.transform.p, sprite.local_offset);
	q.w = sprite.w;
	q.h = sprite.h;
	q.scale_x = sprite.scale.x * sprite.w;
	q.scale_y = sprite.scale.y * sprite.h;
	q.sort_bits = sprite.layer;
	q.alpha = sprite.opacity;
	s_push(q);
}

void cf_sprite_batch_sprite_tf(CF_Sprite sprite, cf_transform_t transform)
{
	BatchSprite q;
	q.id = sprite.animation->frames[sprite.frame_index].id;
	q.transform = transform;
	q.transform.p = cf_add_v2(q.transform.p, sprite.local_offset);
	q.w = sprite.w;
	q.h = sprite.h;
	q.scale_x = sprite.scale.x * sprite.w;
	q.scale_y = sprite.scale.y * sprite.h;
	q.sort_bits = sprite.layer;
	q.alpha = sprite.opacity;
	s_push(q);
}

CUTE_INLINE BatchSprite cf_batch_sprite_defaults()
{
	BatchSprite result = { 0 };
	result.transform = cf_make_transform();
	result.alpha = 1.0f;
	return result;
}

static void s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	// Build vertex buffer of all quads for each sprite.
	int vert_count = count * 6;
	b->verts.ensure_count(vert_count);
	BatchVertex* verts = b->verts.data();

	for (int i = 0; i < count; ++i) {
		spritebatch_sprite_t* s = sprites + i;

		cf_v2 quad[] = {
			{ -0.5f,  0.5f },
			{  0.5f,  0.5f },
			{  0.5f, -0.5f },
			{ -0.5f, -0.5f },
		};

		for (int j = 0; j < 4; ++j) {
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

			quad[j].x = x;
			quad[j].y = y;
		}

		// output transformed quad into CPU buffer
		BatchVertex* out_verts = verts + i * 6;

		for (int i = 0; i < 6; ++i) {
			out_verts[i].alpha = (uint8_t)(s->udata.alpha * 255.0f);
		}

		out_verts[0].position.x = quad[0].x;
		out_verts[0].position.y = quad[0].y;
		out_verts[0].uv.x = s->minx;
		out_verts[0].uv.y = s->maxy;

		out_verts[1].position.x = quad[3].x;
		out_verts[1].position.y = quad[3].y;
		out_verts[1].uv.x = s->minx;
		out_verts[1].uv.y = s->miny;

		out_verts[2].position.x = quad[1].x;
		out_verts[2].position.y = quad[1].y;
		out_verts[2].uv.x = s->maxx;
		out_verts[2].uv.y = s->maxy;

		out_verts[3].position.x = quad[1].x;
		out_verts[3].position.y = quad[1].y;
		out_verts[3].uv.x = s->maxx;
		out_verts[3].uv.y = s->maxy;

		out_verts[4].position.x = quad[3].x;
		out_verts[4].position.y = quad[3].y;
		out_verts[4].uv.x = s->minx;
		out_verts[4].uv.y = s->miny;

		out_verts[5].position.x = quad[2].x;
		out_verts[5].position.y = quad[2].y;
		out_verts[5].uv.x = s->maxx;
		out_verts[5].uv.y = s->miny;
	}

	// Map the vertex buffer with sprite vertex data.
	cf_mesh_append_vertex_data(b->mesh, verts, vert_count / 2);
	cf_apply_mesh(b->mesh);

	// Apply uniforms.
	cf_material_set_uniform_vs(b->material, "vs_params", "u_mvp", &b->projection, CF_UNIFORM_TYPE_MAT4, 1);
	v2 u_texture_size = cf_V2(b->atlas_width, b->atlas_height);
	CF_Color u_tint = b->tints.last();
	cf_material_set_uniform_fs(b->material, "fs_params", "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(b->material, "fs_params", "u_tint", &u_tint, CF_UNIFORM_TYPE_FLOAT4, 1);

	// Outline shader uniforms.
	CF_Color u_border_color = b->outline_color;
	v2 u_texel_size = cf_V2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
	float u_use_border = b->outline_use_border;
	float u_use_corners = b->outline_use_corners;
	cf_material_set_uniform_fs(b->material, "fs_params", "u_border_color", &u_border_color, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(b->material, "fs_params", "u_texel_size", &u_texel_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(b->material, "fs_params", "u_use_border", &u_use_border, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_fs(b->material, "fs_params", "u_use_corners", &u_use_corners, CF_UNIFORM_TYPE_FLOAT, 1);

	// Kick off a draw call.
	cf_apply_shader(b->shader, b->material);
	cf_draw_elements();
}

static void s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	b->get_pixels(image_id, buffer, bytes_to_fill, b->get_pixels_udata);
}

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CF_TextureParams params = cf_texture_defaults();
	params.width = w;
	params.height = h;
	params.wrap_u = params.wrap_v = b->wrap_mode;
	params.filter = b->filter;
	CF_Texture texture = cf_make_texture(params);
	return texture.id;
}

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CF_Texture tex;
	tex.id = texture_id;
	cf_destroy_texture(tex);
}

//--------------------------------------------------------------------------------------------------

static void s_make_batch(get_pixels_fn* get_pixels, void* get_pixels_udata)
{
	b = CUTE_NEW(CF_Batch);

	b->projection = cf_matrix_identity();
	b->get_pixels = get_pixels;
	b->get_pixels_udata = get_pixels_udata;

	// Mesh + vertex attributes.
	b->mesh = cf_make_mesh(CF_USAGE_TYPE_STREAM, CUTE_MB * 25, 0, 0);
	CF_VertexAttribute attrs[4] = { };
	attrs[0].name = "in_position";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CUTE_OFFSET_OF(BatchVertex, position);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CUTE_OFFSET_OF(BatchVertex, uv);
	attrs[2].name = "in_col";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CUTE_OFFSET_OF(BatchVertex, color);
	attrs[3].name = "in_params";
	attrs[3].format = CF_VERTEX_FORMAT_BYTE4;
	attrs[3].offset = CUTE_OFFSET_OF(BatchVertex, solid);
	cf_mesh_set_attributes(b->mesh, attrs, CUTE_ARRAY_SIZE(attrs), sizeof(BatchVertex), 0);

	// Shaders.
	b->shader = CF_MAKE_SOKOL_SHADER(sprite_shd);

	// Material.
	b->material = cf_make_material();
	CF_RenderState state = cf_render_state_defaults();
	state.blend.enabled = true;
	state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.rgb_op = CF_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.alpha_op = CF_BLEND_OP_ADD;
	cf_material_set_render_state(b->material, state);

	// Spritebatcher.
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
		CUTE_FREE(b);
		b = NULL;
		CUTE_ASSERT(false);
	}

	b->atlas_width = (float)config.atlas_width_in_pixels;
	b->atlas_height = (float)config.atlas_height_in_pixels;
	b->m3x2s.add(cf_make_identity());
}

CF_Result cf_batch_flush()
{
	// Draw sprites.
	spritebatch_flush(&b->sb);
	return cf_result_success();
}

void cf_make_batch()
{
	// TODO
}

void cf_destroy_batch()
{
	spritebatch_term(&b->sb);
	b->~CF_Batch();
	CUTE_FREE(b);
}

void cf_batch_update()
{
	spritebatch_tick(&b->sb);
	spritebatch_defrag(&b->sb);
}

//--------------------------------------------------------------------------------------------------

void cf_batch_set_texture_wrap_mode(CF_WrapMode wrap_mode)
{
	b->wrap_mode = wrap_mode;
}

void cf_batch_set_texture_filter(CF_Filter filter)
{
	b->filter = filter;
}

void cf_batch_set_projection(CF_Matrix4x4 projection)
{
	b->projection = projection;
}

void cf_batch_outlines(bool use_outlines)
{
	b->outline_use_border = use_outlines ? 1.0f : 0;
}

void cf_batch_outlines_use_corners(bool use_corners)
{
	b->outline_use_corners = use_corners ? 1.0f : 0;
}

void cf_batch_outlines_color(CF_Color c)
{
	b->outline_color = c;
}

void cf_batch_push_m3x2(cf_m3x2 m)
{
	b->m = m;
	b->scale_x = cf_len(m.m.x);
	b->scale_y = cf_len(m.m.y);
	b->m3x2s.add(m);
}

void cf_batch_pop_m3x2()
{
	if (b->m3x2s.count() > 1) {
		b->m3x2s.pop();
		if (b->m3x2s.size()) {
			b->m = b->m3x2s.last();
			b->scale_x = cf_len(b->m.m.x);
			b->scale_y = cf_len(b->m.m.y);
		} else {
			b->m = cf_make_identity();
			b->scale_x = 1.0f;
			b->scale_y = 1.0f;
		}
	}
}

void cf_batch_push_scissor_box(int x, int y, int w, int h)
{
	Scissor scissor = { x, y, w, h };
	b->scissors.add(scissor);
}

void cf_batch_pop_scissor_box()
{
	if (b->scissors.count()) {
		b->scissors.pop();
	}
}

void cf_batch_push_tint(CF_Color c)
{
	b->tints.add(c);
}

void cf_batch_pop_tint()
{
	if (b->tints.count() > 1) {
		b->tints.pop();
	}
}

void cf_batch_quad_aabb(cf_aabb_t bb, CF_Color c)
{
	cf_v2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_verts(verts[0], verts[1], verts[2], verts[3], c);
}

#define PUSH_VERT(P, C) \
	do { \
		BatchVertex v; \
		v.position = mul(b->m, P); \
		v.uv = V2(0,0); \
		v.color = to_pixel(C); \
		v.solid = 255; \
		v.outline = 0; \
		v.alpha = 0; \
		v.unused1 = 0; \
		b->verts.add(v); \
	} while (0)

#define PUSH_TRI(p0, p1, p2, c0, c1, c2) \
	do { \
		PUSH_VERT(p0, c0); \
		PUSH_VERT(p1, c1); \
		PUSH_VERT(p2, c2); \
	} while (0)

void cf_batch_quad_verts(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, CF_Color c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
	PUSH_TRI(p2, p3, p0, c, c, c);
}

void cf_batch_quad_verts2(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
	PUSH_TRI(p2, p3, p0, c2, c3, c0);
}

void cf_batch_quad_line(cf_aabb_t bb, float thickness, CF_Color c, bool antialias)
{
	cf_v2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_line2(verts[0], verts[1], verts[2], verts[3], thickness, c, antialias);
}

void cf_internal_batch_quad_line(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3, bool antialias)
{
	if (antialias) {
		cf_v2 verts[] = { p0, p1, p2, p3 };
		cf_batch_polyline(verts, 4, thickness, c0, true, true, 0);
	} else {
		float sqrt_2 = 1.41421356237f;
		cf_v2 n = cf_V2(sqrt_2, sqrt_2) * thickness;
		cf_v2 q0 = p0 + cf_V2(-n.x, -n.y);
		cf_v2 q1 = p1 + cf_V2( n.x, -n.y);
		cf_v2 q2 = p2 + cf_V2( n.x,  n.y);
		cf_v2 q3 = p3 + cf_V2(-n.x,  n.y);
		cf_batch_quad_verts2(p0, p1, q1, q0, c0, c1, c2, c3);
		cf_batch_quad_verts2(p1, p2, q2, q1, c0, c1, c2, c3);
		cf_batch_quad_verts2(p2, p3, q3, q2, c0, c1, c2, c3);
		cf_batch_quad_verts2(p3, p0, q0, q3, c0, c1, c2, c3);
	}
}

void cf_batch_quad_line3(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	cf_internal_batch_quad_line(p0, p1, p2, p3, thickness, c0, c1, c2, c3, false);
}

void cf_batch_quad_line2(cf_v2 p0, cf_v2 p1, cf_v2 p2, cf_v2 p3, float thickness, CF_Color c, bool antialias)
{
	cf_internal_batch_quad_line(p0, p1, p2, p3, thickness, c, c, c, c, antialias);
}

void cf_batch_circle(cf_v2 p, float r, int iters, CF_Color c)
{
	cf_v2 prev = cf_V2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		cf_v2 next = cf_from_angle(a) * r;
		CF_Batchri(p + prev, p + next, p, c);
		prev = next;
	}
}

void cf_batch_circle_line(cf_v2 p, float r, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		array<cf_v2> verts(iters);
		cf_v2 p0 = cf_V2(p.x + r, p.y);
		verts.add(p0);

		for (int i = 1; i < iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			cf_v2 n = cf_from_angle(a);
			cf_v2 p1 = p + n * r;
			verts.add(p1);
			p0 = p1;
		}

		cf_batch_polyline(verts.data(), verts.size(), thickness, color, true, true, 0);
	} else {
		float half_thickness = thickness * 0.5f;
		cf_v2 p0 = cf_V2(p.x + r - half_thickness, p.y);
		cf_v2 p1 = cf_V2(p.x + r + half_thickness, p.y);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			cf_v2 n = cf_from_angle(a);
			cf_v2 p2 = p + n * (r + half_thickness);
			cf_v2 p3 = p + n * (r - half_thickness);
			cf_batch_quad_verts(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_circle_arc(cf_v2 p, cf_v2 center_of_arc, float range, int iters, CF_Color color)
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
		CF_Batchri(p, p1, p0, color);
		p0 = p1;
	}
}

static void s_circle_arc_line_aa(array<cf_v2>* verts, cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, CF_Color color)
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

void cf_batch_circle_arc_line(cf_v2 p, cf_v2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		array<cf_v2> verts(iters);
		s_circle_arc_line_aa(&verts, p, center_of_arc, range, iters, thickness, color);
		cf_batch_polyline(verts.data(), verts.size(), thickness, color, false, true, 3);
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
			cf_batch_quad_verts(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_capsule(cf_v2 a, cf_v2 b, float r, int iters, CF_Color c)
{
	cf_batch_circle_arc(a, a + cf_norm(a - b) * r, CUTE_PI, iters, c);
	cf_batch_circle_arc(b, b + cf_norm(b - a) * r, CUTE_PI, iters, c);
	cf_v2 n = cf_skew(cf_norm(b - a)) * r;
	cf_v2 q0 = a + n;
	cf_v2 q1 = b + n;
	cf_v2 q2 = b - n;
	cf_v2 q3 = a - n;
	cf_batch_quad_verts(q0, q1, q2, q3, c);
}

void cf_batch_capsule_line(cf_v2 a, cf_v2 b, float r, int iters, float thickness, CF_Color c, bool antialias)
{
	if (antialias) {
		array<cf_v2> verts(iters * 2 + 2);
		s_circle_arc_line_aa(&verts, a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c);
		s_circle_arc_line_aa(&verts, b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c);
		cf_batch_polyline(verts.data(), verts.count(), thickness, c, true, true, 0);
	} else {
		cf_batch_circle_arc_line(a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c, false);
		cf_batch_circle_arc_line(b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c, false);
		cf_v2 n = cf_skew(cf_norm(b - a)) * r;
		cf_v2 q0 = a + n;
		cf_v2 q1 = b + n;
		cf_v2 q2 = b - n;
		cf_v2 q3 = a - n;
		cf_batch_line(q0, q1, thickness, c, false);
		cf_batch_line(q2, q3, thickness, c, false);
	}
}

void CF_Batchri(cf_v2 p0, cf_v2 p1, cf_v2 p2, CF_Color c)
{
	PUSH_TRI(p0, p1, p2, c, c, c);
}

void CF_Batchri2(cf_v2 p0, cf_v2 p1, cf_v2 p2, CF_Color c0, CF_Color c1, CF_Color c2)
{
	PUSH_TRI(p0, p1, p2, c0, c1, c2);
}

void CF_Batchri_line(cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, CF_Color c, bool antialias)
{
	CUTE_ASSERT(0);
}

void CF_Batchri_line2(cf_v2 p0, cf_v2 p1, cf_v2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_batch_line(cf_v2 p0, cf_v2 p1, float thickness, CF_Color c, bool antialias)
{
	cf_batch_line2(p0, p1, thickness, c, c, antialias);
}

void cf_batch_line2(cf_v2 p0, cf_v2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias)
{
	float scale = cf_len(b->m.m.x); // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (antialias) {
		CF_Color c2 = c0;
		CF_Color c3 = c1;
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
			cf_batch_quad_verts2(q0, q1, q2, q3, c0, c0, c1, c1);

			// Zero opacity aliased quads.
			cf_v2 n2 = cf_cw90(n0) * alias_scale;
			cf_v2 q4 = q3 + n2;
			cf_v2 q5 = q2 + n2;
			cf_v2 q6 = q1 - n2;
			cf_v2 q7 = q0 - n2;
			cf_batch_quad_verts2(q3, q2, q5, q4, c0, c1, c3, c2);
			cf_batch_quad_verts2(q0, q7, q6, q1, c0, c2, c3, c1);

			// End caps.
			n0 = n0 * alias_scale;
			cf_v2 r0 = q5 + n0;
			cf_v2 r1 = q2 + n0;
			cf_v2 r2 = q1 + n0;
			cf_v2 r3 = q6 + n0;
			cf_batch_quad_verts2(q2, r1, r0, q5, c1, c3, c3, c3);
			cf_batch_quad_verts2(q2, q1, r2, r1, c1, c1, c3, c3);
			cf_batch_quad_verts2(q1, q6, r3, r2, c1, c3, c3, c3);

			cf_v2 r4 = q4 - n0;
			cf_v2 r5 = q3 - n0;
			cf_v2 r6 = q0 - n0;
			cf_v2 r7 = q7 - n0;
			cf_batch_quad_verts2(q3, r5, r4, q4, c0, c2, c2, c2);
			cf_batch_quad_verts2(q3, q0, r6, r5, c0, c0, c2, c2);
			cf_batch_quad_verts2(q0, q7, r7, r6, c0, c2, c2, c2);
		} else {
			// Zero opacity aliased quads, without any core line.
			cf_v2 n = cf_skew(cf_norm(p1 - p0)) * alias_scale * 0.5f;
			cf_v2 q0 = p0 + n;
			cf_v2 q1 = p1 + n;
			cf_v2 q2 = p1 - n;
			cf_v2 q3 = p0 - n;
			cf_batch_quad_verts2(p0, p1, q1, q0, c0, c1, c3, c2);
			cf_batch_quad_verts2(p1, p0, q3, q2, c1, c0, c3, c2);
		}
	} else {
		cf_v2 n = cf_skew(cf_norm(p1 - p0)) * thickness * 0.5f;
		cf_v2 q0 = p0 + n;
		cf_v2 q1 = p1 + n;
		cf_v2 q2 = p1 - n;
		cf_v2 q3 = p0 - n;
		cf_batch_quad_verts2(q0, q1, q2, q3, c0, c0, c1, c1);
	}
}

CUTE_INLINE static cf_v2 s_rot_b_about_a(cf_sincos_t r, cf_v2 b, cf_v2 a)
{
	cf_v2 result = cf_mul_sc_v2(r, a - b);
	return result + b;
}

CUTE_INLINE static void s_bevel_arc_feather(cf_v2 b, cf_v2 i3, cf_v2 f3, cf_v2 i4, cf_v2 f4, CF_Color c0, CF_Color c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	cf_sincos_t r = cf_sincos_f(arc);
	cf_v2 p0 = i3;
	cf_v2 p1 = f3;
	for (int i = 1; i < bevel_count; ++i) {
		cf_v2 p2 = s_rot_b_about_a(r, b, p1);
		cf_v2 p3 = s_rot_b_about_a(r, b, p0);
		CF_Batchri(b, p0, p3, c0);
		cf_batch_quad_verts2(p3, p2, p1, p0, c0, c1, c1, c0);
		p0 = p3;
		p1 = p2;
	}
	CF_Batchri(b, i4, p0, c0);
	cf_batch_quad_verts2(p0, i4, f4, p1, c0, c0, c1, c1);
}

CUTE_INLINE static void s_bevel_arc(cf_v2 b, cf_v2 i3, cf_v2 i4, CF_Color c0, CF_Color c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	cf_sincos_t r = cf_sincos_f(arc);
	cf_v2 p0 = i3;
	for (int i = 1; i < bevel_count; ++i) {
		cf_v2 p3 = s_rot_b_about_a(r, b, p0);
		CF_Batchri(b, p0, p3, c0);
		p0 = p3;
	}
	CF_Batchri(b, i4, p0, c0);
}

static void s_polyline(cf_v2* points, int count, float thickness, CF_Color c0, CF_Color c1, bool loop, bool feather, float alias_scale, int bevel_count)
{
	float inner_half = (thickness - alias_scale);
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
		auto cc = cf_color_white();

		if (ab_x_bc < -k_tol) {
			if (d >= 0) {
				cf_v2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b - n0), b - n1, c - n1);
				cf_v2 i3 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				if (feather) {
					cf_v2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b - fn0), b - fn1, c - fn1);
					cf_v2 f3 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					if (emit) {
						cf_batch_quad_verts(a, b, i3, i0, c0);
						cf_batch_quad_verts(i1, i2, b, a, c0);
						cf_batch_quad_verts2(i0, i3, f3, f0, c0, c0, c1, c1);
						cf_batch_quad_verts2(f1, f2, i2, i1, c1, c1, c0, c0);
					}
					f0 = f3;
					f1 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(a, b, i3, i0, c0, c0, c1, c1);
					cf_batch_quad_verts2(i1, i2, b, a, c1, c1, c0, c0);
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
						cf_batch_quad_verts(a, b, i3, i0, c0);
						cf_batch_quad_verts(i1, i2, b, a, c0);
						cf_batch_quad_verts2(i0, i3, f3, f0, c0, c0, c1, c1);
						cf_batch_quad_verts2(i1, f1, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f0 = f4;
					f1 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(a, b, i3, i0, c0, c0, c1, c1);
					cf_batch_quad_verts2(i1, i2, b, a, c1, c1, c0, c0);
					s_bevel_arc(b, i3, i4, c0, c1, bevel_count);
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
						cf_batch_quad_verts(a, b, i3, i1, c0);
						cf_batch_quad_verts(i0, i2, b, a, c0);
						cf_batch_quad_verts2(i1, i3, f3, f1, c0, c0, c1, c1);
						cf_batch_quad_verts2(f0, f2, i2, i0, c1, c1, c0, c0);
					}
					f1 = f3;
					f0 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(a, b, i3, i1, c0, c0, c1, c1);
					cf_batch_quad_verts2(i0, i2, b, a, c1, c1, c0, c0);
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
						cf_batch_quad_verts(a, b, i3, i1, c0);
						cf_batch_quad_verts(i0, i2, b, a, c0);
						cf_batch_quad_verts2(i1, i3, f3, f1, c0, c0, c1, c1);
						cf_batch_quad_verts2(i0, f0, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f1 = f4;
					f0 = f2;
				} else if (emit) {
					cf_batch_quad_verts2(a, b, i3, i1, c0, c0, c1, c1);
					cf_batch_quad_verts2(i0, i2, b, a, c1, c1, c0, c0);
					s_bevel_arc(b, i3, i4, c0, c1, bevel_count);
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
					cf_batch_quad_verts(a, b, i2, i0, c0);
					cf_batch_quad_verts(i1, i3, b, a, c0);
					cf_batch_quad_verts2(i0, i2, f2, f0, c0, c0, c1, c1);
					cf_batch_quad_verts2(i1, f1, f3, i3, c0, c1, c1, c0);
				}
				f1 = f3;
				f0 = f2;
			} else if (emit) {
				cf_batch_quad_verts2(a, b, i2, i0, c0, c0, c1, c1);
				cf_batch_quad_verts2(i1, i3, b, a, c1, c1, c0, c0);
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
			cf_batch_quad_verts(a, b, b + n0, i0, c0);
			cf_batch_quad_verts(a, i1, b - n0, b, c0);
			cf_batch_quad_verts2(f0, i0, b + n0, b + fn0, c1, c0, c0, c1);
			cf_batch_quad_verts2(b - fn0, b - n0, i1, f1, c1, c0, c0, c1);

			// End caps.
			cf_v2 n = cf_norm(b - a) * alias_scale;
			cf_batch_quad_verts2(b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_batch_quad_verts2(b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_batch_quad_verts2(b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);

			a = points[1];
			b = points[0];
			n = cf_norm(b - a) * alias_scale;
			n0 = cf_skew(cf_norm(b - a)) * inner_half;
			fn0 = cf_norm(n0) * outer_half;
			cf_batch_quad_verts2(b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_batch_quad_verts2(b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_batch_quad_verts2(b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);
		} else {
			cf_batch_quad_verts2(a, b, b + n0, i0, c0, c0, c1, c1);
			cf_batch_quad_verts2(a, i1, b - n0, b, c0, c1, c1, c0);
		}
	}
}

void cf_batch_polyline(cf_v2* points, int count, float thickness, CF_Color color, bool loop, bool antialias, int bevel_count)
{
	CUTE_ASSERT(count >= 3);
	float scale = cf_len(b->m.m.x); // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (antialias) {
		CF_Color no_alpha = color;
		no_alpha.a = 0;
		if (thick_line) {
			s_polyline(points, count, thickness, color, no_alpha, loop, true, alias_scale, bevel_count);
		} else {
			s_polyline(points, count, alias_scale, color, no_alpha, loop, false, 0, bevel_count);
		}
	} else {
		s_polyline(points, count, thickness, color, color, loop, false, 0, bevel_count);
	}
}

CF_TemporaryImage cf_batch_fetch(BatchSprite sprite)
{
	spritebatch_sprite_t s = spritebatch_fetch(&b->sb, sprite.id, sprite.w, sprite.h);
	CF_TemporaryImage image;
	image.tex = { s.texture_id };
	image.w = s.w;
	image.h = s.h;
	image.u = cf_V2(s.minx, s.miny);
	image.v = cf_V2(s.maxx, s.maxy);
	return image;
}
