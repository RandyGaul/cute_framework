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
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

#include <shaders/sprite_shader.h>

static struct CF_Batch* b;

struct BatchGeometry
{
	CF_V2 position;
	CF_V2 scale;
	CF_SinCos rotation;
	CF_V2 a, b, c;
	float alpha;
	bool tri;
};

#include <cute/cute_png.h>

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry
#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define DEBUG_VERT(v, c) batch_quad(make_aabb(v, 3, 3), c)

// Initial design of this batcher comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_batch.h

using namespace cute;

void s_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	CUTE_UNUSED(udata);
	if (image_id >= CUTE_ASEPRITE_ID_RANGE_LO && image_id <= CUTE_ASEPRITE_ID_RANGE_HI) {
		cf_aseprite_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CUTE_PNG_ID_RANGE_LO && image_id <= CUTE_PNG_ID_RANGE_HI) {
		cf_png_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CUTE_FONT_ID_RANGE_LO && image_id <= CUTE_FONT_ID_RANGE_HI) {
	} else {
		CUTE_ASSERT(false);
		CUTE_MEMSET(buffer, 0, sizeof(bytes_to_fill));
	}
}

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

struct CF_Batch
{
	::spritebatch_t sb;

	float atlas_width = 1024;
	float atlas_height = 1024;
	
	Array<BatchVertex> verts;
	CF_Pass pass;
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
	Array<cf_m3x2> m3x2s;
	Array<Scissor> scissors;
	Array<CF_Color> tints = { DEFAULT_TINT };
	Array<int> layers = { 0 };
};

void cf_batch_sprite_tf(const CF_Sprite* sprite)
{
	v2 p = cf_mul_m32_v2(b->m, cf_add_v2(sprite->transform.p, sprite->local_offset));
	spritebatch_sprite_t s;
	s.image_id = sprite->animation->frames[sprite->frame_index].id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.position = p;
	s.geom.scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	s.geom.rotation = sprite->transform.r;
	s.geom.alpha = sprite->opacity;
	s.sort_bits = sprite->layer;
	spritebatch_push(&b->sb, s);
}

void cf_batch_sprite_tf(const CF_Sprite* sprite, CF_Transform transform)
{
	transform = mul(transform, sprite->transform);
	v2 p = cf_mul_m32_v2(b->m, cf_add_v2(transform.p, sprite->local_offset));
	spritebatch_sprite_t s;
	s.image_id = sprite->animation->frames[sprite->frame_index].id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.position = p;
	s.geom.scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	s.geom.rotation = sprite->transform.r;
	s.geom.alpha = sprite->opacity;
	s.sort_bits = sprite->layer;
	spritebatch_push(&b->sb, s);
}

static void s_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	CUTE_UNUSED(udata);
	int vert_count = count * 6;
	b->verts.ensure_count(vert_count);
	BatchVertex* verts = b->verts.data();

	for (int i = 0; i < count; ++i) {
		spritebatch_sprite_t* s = sprites + i;

		// Expand sprite's scale to account for border pixels in the atlas.
		s->geom.scale.x = s->geom.scale.x + (s->geom.scale.x / (float)s->w) * 2.0f;
		s->geom.scale.y = s->geom.scale.y + (s->geom.scale.y / (float)s->h) * 2.0f;

		if (s->geom.tri) {
			// TODO.
		} else {
			CF_V2 quad[] = {
				{ -0.5f,  0.5f },
				{  0.5f,  0.5f },
				{  0.5f, -0.5f },
				{ -0.5f, -0.5f },
			};

			for (int j = 0; j < 4; ++j) {
				float x = quad[j].x;
				float y = quad[j].y;

				// Rotate sprite about origin.
				float x0 = s->geom.rotation.c * x - s->geom.rotation.s * y;
				float y0 = s->geom.rotation.s * x + s->geom.rotation.c * y;
				x = x0;
				y = y0;

				// Scale sprite about origin.
				x *= s->geom.scale.x;
				y *= s->geom.scale.y;

				// Translate sprite into the world.
				x += s->geom.position.x;
				y += s->geom.position.y;

				quad[j].x = x;
				quad[j].y = y;
			}

			// output transformed quad into CPU buffer
			BatchVertex* out_verts = verts + i * 6;

			for (int i = 0; i < 6; ++i) {
				out_verts[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
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
	}

	// Map the vertex buffer with sprite vertex data.
	cf_mesh_append_vertex_data(b->mesh, verts, vert_count);
	cf_apply_mesh(b->mesh);

	// Apply the atlas texture.
	CF_Texture atlas = { sprites->texture_id };
	cf_material_set_texture_fs(b->material, "u_image", atlas);

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

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CUTE_UNUSED(udata);
	CF_TextureParams params = cf_texture_defaults();
	params.width = w;
	params.height = h;
	params.wrap_u = params.wrap_v = b->wrap_mode;
	params.filter = b->filter;
	params.initial_data = pixels;
	params.initial_data_size = w * h * sizeof(CF_Pixel);
	CF_Texture texture = cf_make_texture(params);
	return texture.id;
}

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CUTE_UNUSED(udata);
	CF_Texture tex;
	tex.id = texture_id;
	cf_destroy_texture(tex);
}

//--------------------------------------------------------------------------------------------------

CF_Result cf_batch_flush()
{
	// Draw sprites.
	spritebatch_flush(&b->sb);
	return cf_result_success();
}

void cf_make_batch()
{
	b = CUTE_NEW(CF_Batch);
	b->projection = cf_matrix_identity();

	// Pass.
	CF_PassParams params = cf_pass_defaults();
	params.name = "Batch";
	params.target = cf_app_get_backbuffer();
	params.depth_stencil = cf_app_get_backbuffer_depth_stencil();
	b->pass = cf_make_pass(params);

	// Mesh + vertex attributes.
	b->mesh = cf_make_mesh(CF_USAGE_TYPE_STREAM, CUTE_MB * 25, 0, 0);
	CF_VertexAttribute attrs[4] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CUTE_OFFSET_OF(BatchVertex, position);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CUTE_OFFSET_OF(BatchVertex, uv);
	attrs[2].name = "in_col";
	attrs[2].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[2].offset = CUTE_OFFSET_OF(BatchVertex, color);
	attrs[3].name = "in_params";
	attrs[3].format = CF_VERTEX_FORMAT_UBYTE4N;
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
	config.allocator_context = NULL;
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

void cf_destroy_batch()
{
	spritebatch_term(&b->sb);
	cf_destroy_pass(b->pass);
	cf_destroy_mesh(b->mesh);
	cf_destroy_material(b->material);
	cf_destroy_shader(b->shader);
	b->~CF_Batch();
	CUTE_FREE(b);
}

void cf_batch_render()
{
	b->verts.clear();
	cf_begin_pass(b->pass);
	spritebatch_tick(&b->sb);
	spritebatch_defrag(&b->sb);
	spritebatch_flush(&b->sb);
	cf_end_pass();
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

cf_m3x2 cf_batch_pop_m3x2()
{
	cf_m3x2 result = cf_make_identity();
	if (b->m3x2s.count() > 1) {
		result = b->m3x2s.pop();
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
	return result;
}

cf_m3x2 cf_batch_peek_m3x2()
{
	return b->m3x2s.last();
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

void cf_batch_peek_scissor_box(int* x, int* y, int* w, int* h)
{
	Scissor scissor = b->scissors.last();
	*x = scissor.x;
	*y = scissor.y;
	*w = scissor.w;
	*h = scissor.h;
}

void cf_batch_push_tint(CF_Color c)
{
	b->tints.add(c);
}

CF_Color cf_batch_pop_tint()
{
	if (b->tints.count() > 1) {
		return b->tints.pop();
	}
	return b->tints.last();
}

CF_Color cf_batch_peek_tint()
{
	return b->tints.last();
}

void cf_batch_push_layer(int layer)
{
	b->layers.add(layer);
}

int cf_batch_pop_layer()
{
	if (b->layers.count() > 1) {
		return b->layers.pop();
	}
	return b->layers.last();
}

int cf_batch_peek_layer()
{
	return b->layers.last();
}

void cf_batch_quad_aabb(CF_Aabb bb, CF_Color c)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_verts(verts[0], verts[1], verts[2], verts[3], c);
}

void cf_batch_quad_verts(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c)
{
}

void cf_batch_quad_verts2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
}

void cf_batch_quad_line(CF_Aabb bb, float thickness, CF_Color c, bool antialias)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_batch_quad_line2(verts[0], verts[1], verts[2], verts[3], thickness, c, antialias);
}

void cf_internal_batch_quad_line(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3, bool antialias)
{
	if (antialias) {
		CF_V2 verts[] = { p0, p1, p2, p3 };
		cf_batch_polyline(verts, 4, thickness, c0, true, true, 0);
	} else {
		float sqrt_2 = 1.41421356237f;
		CF_V2 n = cf_V2(sqrt_2, sqrt_2) * thickness;
		CF_V2 q0 = p0 + cf_V2(-n.x, -n.y);
		CF_V2 q1 = p1 + cf_V2( n.x, -n.y);
		CF_V2 q2 = p2 + cf_V2( n.x,  n.y);
		CF_V2 q3 = p3 + cf_V2(-n.x,  n.y);
		cf_batch_quad_verts2(p0, p1, q1, q0, c0, c1, c2, c3);
		cf_batch_quad_verts2(p1, p2, q2, q1, c0, c1, c2, c3);
		cf_batch_quad_verts2(p2, p3, q3, q2, c0, c1, c2, c3);
		cf_batch_quad_verts2(p3, p0, q0, q3, c0, c1, c2, c3);
	}
}

void cf_batch_quad_line3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	cf_internal_batch_quad_line(p0, p1, p2, p3, thickness, c0, c1, c2, c3, false);
}

void cf_batch_quad_line2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c, bool antialias)
{
	cf_internal_batch_quad_line(p0, p1, p2, p3, thickness, c, c, c, c, antialias);
}

void cf_batch_circle(CF_V2 p, float r, int iters, CF_Color c)
{
	CF_V2 prev = cf_V2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		CF_V2 next = cf_from_angle(a) * r;
		CF_Batchri(p + prev, p + next, p, c);
		prev = next;
	}
}

void cf_batch_circle_line(CF_V2 p, float r, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters);
		CF_V2 p0 = cf_V2(p.x + r, p.y);
		verts.add(p0);

		for (int i = 1; i < iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p1 = p + n * r;
			verts.add(p1);
			p0 = p1;
		}

		cf_batch_polyline(verts.data(), verts.size(), thickness, color, true, true, 0);
	} else {
		float half_thickness = thickness * 0.5f;
		CF_V2 p0 = cf_V2(p.x + r - half_thickness, p.y);
		CF_V2 p1 = cf_V2(p.x + r + half_thickness, p.y);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p2 = p + n * (r + half_thickness);
			CF_V2 p3 = p + n * (r - half_thickness);
			cf_batch_quad_verts(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, CF_Color color)
{
	float r = cf_len(center_of_arc - p);
	CF_V2 d = cf_norm(center_of_arc - p);
	CF_SinCos m = cf_sincos_f(range * 0.5f);

	CF_V2 t = cf_mulT_sc_v2(m, d);
	CF_V2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		CF_V2 p1 = p + t * r;
		CF_Batchri(p, p1, p0, color);
		p0 = p1;
	}
}

static void s_circle_arc_line_aa(Array<CF_V2>* verts, CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color)
{
	float r = cf_len(center_of_arc - p);
	CF_V2 d = cf_norm(center_of_arc - p);
	CF_SinCos m = cf_sincos_f(range * 0.5f);

	CF_V2 t = cf_mulT_sc_v2(m, d);
	CF_V2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;
	verts->add(p0);

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		CF_V2 p1 = p + t * r;
		verts->add(p1);
		p0 = p1;
	}
}

void cf_batch_circle_arc_line(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters);
		s_circle_arc_line_aa(&verts, p, center_of_arc, range, iters, thickness, color);
		cf_batch_polyline(verts.data(), verts.size(), thickness, color, false, true, 3);
	} else {
		float r = cf_len(center_of_arc - p);
		CF_V2 d = cf_norm(center_of_arc - p);
		CF_SinCos m = cf_sincos_f(range * 0.5f);

		float half_thickness = thickness * 0.5f;
		CF_V2 t = cf_mulT_sc_v2(m, d);
		CF_V2 p0 = p + t * (r + half_thickness);
		CF_V2 p1 = p + t * (r - half_thickness);
		d = cf_norm(p0 - p);
		float inc = range / iters;

		for (int i = 1; i <= iters; i++) {
			m = cf_sincos_f(i * inc);
			t = cf_mul_sc_v2(m, d);
			CF_V2 p2 = p + t * (r + half_thickness);
			CF_V2 p3 = p + t * (r - half_thickness);
			cf_batch_quad_verts(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_batch_capsule(CF_V2 a, CF_V2 b, float r, int iters, CF_Color c)
{
	cf_batch_circle_arc(a, a + cf_norm(a - b) * r, CUTE_PI, iters, c);
	cf_batch_circle_arc(b, b + cf_norm(b - a) * r, CUTE_PI, iters, c);
	CF_V2 n = cf_skew(cf_norm(b - a)) * r;
	CF_V2 q0 = a + n;
	CF_V2 q1 = b + n;
	CF_V2 q2 = b - n;
	CF_V2 q3 = a - n;
	cf_batch_quad_verts(q0, q1, q2, q3, c);
}

void cf_batch_capsule_line(CF_V2 a, CF_V2 b, float r, int iters, float thickness, CF_Color c, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters * 2 + 2);
		s_circle_arc_line_aa(&verts, a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c);
		s_circle_arc_line_aa(&verts, b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c);
		cf_batch_polyline(verts.data(), verts.count(), thickness, c, true, true, 0);
	} else {
		cf_batch_circle_arc_line(a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c, false);
		cf_batch_circle_arc_line(b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c, false);
		CF_V2 n = cf_skew(cf_norm(b - a)) * r;
		CF_V2 q0 = a + n;
		CF_V2 q1 = b + n;
		CF_V2 q2 = b - n;
		CF_V2 q3 = a - n;
		cf_batch_line(q0, q1, thickness, c, false);
		cf_batch_line(q2, q3, thickness, c, false);
	}
}

void CF_Batchri(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c)
{
}

void CF_Batchri2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2)
{
}

void CF_Batchri_line(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c, bool antialias)
{
	CUTE_ASSERT(0);
}

void CF_Batchri_line2(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_batch_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c, bool antialias)
{
	cf_batch_line2(p0, p1, thickness, c, c, antialias);
}

void cf_batch_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias)
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
			CF_V2 n0 = cf_norm(p1 - p0);
			CF_V2 n1 = cf_skew(n0) * core_half_width;
			CF_V2 q0 = p0 + n1;
			CF_V2 q1 = p1 + n1;
			CF_V2 q2 = p1 - n1;
			CF_V2 q3 = p0 - n1;
			cf_batch_quad_verts2(q0, q1, q2, q3, c0, c0, c1, c1);

			// Zero opacity aliased quads.
			CF_V2 n2 = cf_cw90(n0) * alias_scale;
			CF_V2 q4 = q3 + n2;
			CF_V2 q5 = q2 + n2;
			CF_V2 q6 = q1 - n2;
			CF_V2 q7 = q0 - n2;
			cf_batch_quad_verts2(q3, q2, q5, q4, c0, c1, c3, c2);
			cf_batch_quad_verts2(q0, q7, q6, q1, c0, c2, c3, c1);

			// End caps.
			n0 = n0 * alias_scale;
			CF_V2 r0 = q5 + n0;
			CF_V2 r1 = q2 + n0;
			CF_V2 r2 = q1 + n0;
			CF_V2 r3 = q6 + n0;
			cf_batch_quad_verts2(q2, r1, r0, q5, c1, c3, c3, c3);
			cf_batch_quad_verts2(q2, q1, r2, r1, c1, c1, c3, c3);
			cf_batch_quad_verts2(q1, q6, r3, r2, c1, c3, c3, c3);

			CF_V2 r4 = q4 - n0;
			CF_V2 r5 = q3 - n0;
			CF_V2 r6 = q0 - n0;
			CF_V2 r7 = q7 - n0;
			cf_batch_quad_verts2(q3, r5, r4, q4, c0, c2, c2, c2);
			cf_batch_quad_verts2(q3, q0, r6, r5, c0, c0, c2, c2);
			cf_batch_quad_verts2(q0, q7, r7, r6, c0, c2, c2, c2);
		} else {
			// Zero opacity aliased quads, without any core line.
			CF_V2 n = cf_skew(cf_norm(p1 - p0)) * alias_scale * 0.5f;
			CF_V2 q0 = p0 + n;
			CF_V2 q1 = p1 + n;
			CF_V2 q2 = p1 - n;
			CF_V2 q3 = p0 - n;
			cf_batch_quad_verts2(p0, p1, q1, q0, c0, c1, c3, c2);
			cf_batch_quad_verts2(p1, p0, q3, q2, c1, c0, c3, c2);
		}
	} else {
		CF_V2 n = cf_skew(cf_norm(p1 - p0)) * thickness * 0.5f;
		CF_V2 q0 = p0 + n;
		CF_V2 q1 = p1 + n;
		CF_V2 q2 = p1 - n;
		CF_V2 q3 = p0 - n;
		cf_batch_quad_verts2(q0, q1, q2, q3, c0, c0, c1, c1);
	}
}

CUTE_INLINE static CF_V2 s_rot_b_about_a(CF_SinCos r, CF_V2 b, CF_V2 a)
{
	CF_V2 result = cf_mul_sc_v2(r, a - b);
	return result + b;
}

CUTE_INLINE static void s_bevel_arc_feather(CF_V2 b, CF_V2 i3, CF_V2 f3, CF_V2 i4, CF_V2 f4, CF_Color c0, CF_Color c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	CF_SinCos r = cf_sincos_f(arc);
	CF_V2 p0 = i3;
	CF_V2 p1 = f3;
	for (int i = 1; i < bevel_count; ++i) {
		CF_V2 p2 = s_rot_b_about_a(r, b, p1);
		CF_V2 p3 = s_rot_b_about_a(r, b, p0);
		CF_Batchri(b, p0, p3, c0);
		cf_batch_quad_verts2(p3, p2, p1, p0, c0, c1, c1, c0);
		p0 = p3;
		p1 = p2;
	}
	CF_Batchri(b, i4, p0, c0);
	cf_batch_quad_verts2(p0, i4, f4, p1, c0, c0, c1, c1);
}

CUTE_INLINE static void s_bevel_arc(CF_V2 b, CF_V2 i3, CF_V2 i4, CF_Color c0, CF_Color c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	CF_SinCos r = cf_sincos_f(arc);
	CF_V2 p0 = i3;
	for (int i = 1; i < bevel_count; ++i) {
		CF_V2 p3 = s_rot_b_about_a(r, b, p0);
		CF_Batchri(b, p0, p3, c0);
		p0 = p3;
	}
	CF_Batchri(b, i4, p0, c0);
}

static void s_polyline(CF_V2* points, int count, float thickness, CF_Color c0, CF_Color c1, bool loop, bool feather, float alias_scale, int bevel_count)
{
	float inner_half = (thickness - alias_scale);
	float outer_half = inner_half + alias_scale;
	int iter = 0;
	int i = 2;
	CF_V2 a = points[0];
	CF_V2 b = points[1];
	CF_V2 n0 = cf_skew(cf_norm(b - a)) * inner_half;
	CF_V2 i0 = a + n0;
	CF_V2 i1 = a - n0;
	CF_V2 fn0 = cf_norm(n0) * outer_half;
	CF_V2 f0 = a + fn0;
	CF_V2 f1 = a - fn0;
	int end = count;

	// Optionally emits geometry about each corner of the polyline, and sets up i0, i1, f0, f1, n0 and fn0.
	auto do_polyline_corner = [&](CF_V2 c, bool emit) {
		CF_V2 n1 = cf_skew(cf_norm(c - b)) * inner_half;
		CF_V2 fn1 = cf_norm(n1) * outer_half;
		float ab_x_bc = cf_cross(b - a, c - b);
		float d = cf_dot(cf_cw90(n0), cf_cw90(n1));
		const float k_tol = 1.e-6f;
		auto cc = cf_color_white();

		if (ab_x_bc < -k_tol) {
			if (d >= 0) {
				CF_V2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b - n0), b - n1, c - n1);
				CF_V2 i3 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				if (feather) {
					CF_V2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b - fn0), b - fn1, c - fn1);
					CF_V2 f3 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
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
				CF_V2 i2 = cf_intersect_halfspace2(cf_plane2(-n0, b - n0), b - n1, c - n1);
				CF_V2 i3 = b + n0;
				CF_V2 i4 = b + n1;
				if (feather) {
					CF_V2 f2 = cf_intersect_halfspace2(cf_plane2(-fn0, b - fn0), b - fn1, c - fn1);
					CF_V2 n = cf_norm(n0 + n1);
					CF_Halfspace h = cf_plane2(n, i3 + n * alias_scale);
					CF_V2 f3 = cf_intersect_halfspace2(h, a + fn0, b + fn0);
					CF_V2 f4 = cf_intersect_halfspace2(h, b + fn1, c + fn1);
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
				CF_V2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				CF_V2 i3 = cf_intersect_halfspace2(cf_plane2(n0, b - n0), b - n1, c - n1);
				if (feather) {
					CF_V2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					CF_V2 f3 = cf_intersect_halfspace2(cf_plane2(fn0, b - fn0), b - fn1, c - fn1);
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
				CF_V2 i2 = cf_intersect_halfspace2(cf_plane2(n0, b + n0), b + n1, c + n1);
				CF_V2 i3 = b - n0;
				CF_V2 i4 = b - n1;
				if (feather) {
					CF_V2 f2 = cf_intersect_halfspace2(cf_plane2(fn0, b + fn0), b + fn1, c + fn1);
					CF_V2 n = cf_norm(n0 + n1);
					CF_Halfspace h = cf_plane2(-n, i3 - n * alias_scale);
					CF_V2 f3 = cf_intersect_halfspace2(h, a - fn0, b - fn0);
					CF_V2 f4 = cf_intersect_halfspace2(h, b - fn1, c - fn1);
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
			CF_V2 i2 = b + n0;
			CF_V2 i3 = b - n0;
			if (feather) {
				CF_V2 f2 = b + fn0;
				CF_V2 f3 = b - fn0;
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
		CF_V2 c = points[1];
		n0 = cf_skew(cf_norm(b - a)) * inner_half;
		fn0 = cf_norm(n0) * outer_half;
		do_polyline_corner(c, false);
	} else {
		end -= 2;
	}

	// Main loop, emit geometry about each corner of the polyline.
	do {
		CF_V2 c = points[i];
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
			CF_V2 n = cf_norm(b - a) * alias_scale;
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

void cf_batch_polyline(CF_V2* points, int count, float thickness, CF_Color color, bool loop, bool antialias, int bevel_count)
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

CF_TemporaryImage cf_batch_fetch(const CF_Sprite* sprite)
{
	spritebatch_sprite_t s = spritebatch_fetch(&b->sb, sprite->animation->frames[sprite->frame_index].id, sprite->w, sprite->h);
	CF_TemporaryImage image;
	image.tex = { s.texture_id };
	image.w = s.w;
	image.h = s.h;
	image.u = cf_V2(s.minx, s.miny);
	image.v = cf_V2(s.maxx, s.maxy);
	return image;
}
