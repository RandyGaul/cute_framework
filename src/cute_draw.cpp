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

#include <cute_draw.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_file_system.h>
#include <cute_defer.h>
#include <cute_routine.h>
#include <cute_rnd.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_font_internal.h>

#include <shaders/sprite_shader.h>

static struct CF_Draw* draw;

#include <cute/cute_png.h>

#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <algorithm>

#define DEBUG_VERT(v, c) batch_quad(make_aabb(v, 3, 3), c)

// Initial design of this API comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_draw.h

using namespace Cute;

#define VA_TYPE_SPRITE (0)
#define VA_TYPE_SHAPE  (1)
#define VA_TYPE_TEXT   (2)

CUTE_STATIC_ASSERT(sizeof(BatchGeometry) <= 64, "Try to fix this nicely into a cache line.");

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CUTE_UNUSED(udata);
	CF_TextureParams params = cf_texture_defaults();
	params.width = w;
	params.height = h;
	params.filter = draw->filter;
	params.initial_data = pixels;
	params.initial_data_size = w * h * sizeof(CF_Pixel);
	CF_Texture texture = cf_make_texture(params);
	return texture.id;
}

void cf_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CUTE_UNUSED(udata);
	CF_Texture tex;
	tex.id = texture_id;
	cf_destroy_texture(tex);
}

spritebatch_t* cf_get_draw_sb()
{
	return &draw->sb;
}

void cf_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	CUTE_UNUSED(udata);
	if (image_id >= CUTE_ASEPRITE_ID_RANGE_LO && image_id <= CUTE_ASEPRITE_ID_RANGE_HI) {
		cf_aseprite_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CUTE_PNG_ID_RANGE_LO && image_id <= CUTE_PNG_ID_RANGE_HI) {
		cf_png_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CUTE_FONT_ID_RANGE_LO && image_id <= CUTE_FONT_ID_RANGE_HI) {
		CF_Pixel* pixels = app->font_pixels.get(image_id);
		CUTE_MEMCPY(buffer, pixels, bytes_to_fill);
	} else {
		CUTE_ASSERT(false);
		CUTE_MEMSET(buffer, 0, sizeof(bytes_to_fill));
	}
}

static CUTE_INLINE float s_intersect(float a, float b, float u0, float u1, float plane_d)
{
	float da = a - plane_d;
	float db = b - plane_d;
	return u0 + (u1 - u0) * (da / (da - db));
}

static void s_draw_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	CUTE_UNUSED(udata);
	int vert_count = 0;
	draw->verts.ensure_count(count * 6);
	DrawVertex* verts = draw->verts.data();
	CUTE_MEMSET(verts, 0, sizeof(DrawVertex) * count * 6);

	for (int i = 0; i < count; ++i) {
		spritebatch_sprite_t* s = sprites + i;

		switch (s->geom.type) {
		case BATCH_GEOMETRY_TYPE_TRI:
		{
			BatchTri geom = s->geom.u.tri;
			DrawVertex* out_verts = verts + vert_count;

			for (int i = 0; i < 3; ++i) {
				out_verts[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out_verts[i].type = VA_TYPE_SHAPE;
			}

			out_verts[0].position.x = geom.p0.x;
			out_verts[0].position.y = geom.p0.y;
			out_verts[0].color      = geom.c0;

			out_verts[1].position.x = geom.p1.x;
			out_verts[1].position.y = geom.p1.y;
			out_verts[1].color      = geom.c1;

			out_verts[2].position.x = geom.p2.x;
			out_verts[2].position.y = geom.p2.y;
			out_verts[2].color      = geom.c2;

			vert_count += 3;
		}	break;

		case BATCH_GEOMETRY_TYPE_QUAD:
		{
			BatchQuad geom = s->geom.u.quad;
			DrawVertex* out_verts = verts + vert_count;

			for (int i = 0; i < 6; ++i) {
				out_verts[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out_verts[i].type = VA_TYPE_SHAPE;
			}

			out_verts[0].position.x = geom.p0.x;
			out_verts[0].position.y = geom.p0.y;
			out_verts[0].color      = geom.c0;

			out_verts[1].position.x = geom.p3.x;
			out_verts[1].position.y = geom.p3.y;
			out_verts[1].color      = geom.c3;

			out_verts[2].position.x = geom.p1.x;
			out_verts[2].position.y = geom.p1.y;
			out_verts[2].color      = geom.c1;

			out_verts[3].position.x = geom.p1.x;
			out_verts[3].position.y = geom.p1.y;
			out_verts[3].color      = geom.c1;

			out_verts[4].position.x = geom.p3.x;
			out_verts[4].position.y = geom.p3.y;
			out_verts[4].color      = geom.c3;

			out_verts[5].position.x = geom.p2.x;
			out_verts[5].position.y = geom.p2.y;
			out_verts[5].color      = geom.c2;

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_SPRITE:
		{
			BatchSprite geom = s->geom.u.sprite;
			DrawVertex* out_verts = verts + vert_count;

			bool clipped_away = false;
			if (geom.do_clipping) {
				CUTE_ASSERT(geom.is_text);

				CF_Aabb bb = make_aabb(geom.p3, geom.p1);
				CF_Aabb clip = geom.clip;
				float top = clip.max.y;
				float left = clip.min.x;
				float bottom = clip.min.y;
				float right = clip.max.x;

				int separating_x_axis = (bb.max.x < left) | (bb.min.x > right);
				if (separating_x_axis) continue;

				if (bb.min.x < left) {
					s->minx = s_intersect(bb.min.x, bb.max.x, s->minx, s->maxx, left);
					bb.min.x = left;
				}

				if (bb.max.x > right) {
					s->maxx = s_intersect(bb.min.x, bb.max.x, s->minx, s->maxx, right);
					bb.max.x = right;
				}

				if (bb.min.y < bottom) {
					s->miny = s_intersect(bb.min.y, bb.max.y, s->miny, s->maxy, bottom);
					bb.min.y = bottom;
				}

				if (bb.max.y > top) {
					s->maxy = s_intersect(bb.min.y, bb.max.y, s->miny, s->maxy, top);
					bb.max.y = top;
				}

				if ((bb.min.x >= bb.max.x) | (bb.min.y >= bb.max.y)) {
					continue;
				}

				geom.p0 = V2(bb.min.x, bb.max.y);
				geom.p1 = bb.max;
				geom.p2 = V2(bb.max.x, bb.min.y);
				geom.p3 = bb.min;
			}

			for (int i = 0; i < 6; ++i) {
				out_verts[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				if (s->geom.u.sprite.is_sprite) {
					out_verts[i].type = VA_TYPE_SPRITE;
				} else if (s->geom.u.sprite.is_text) {
					out_verts[i].type = VA_TYPE_TEXT;
				} else {
					CUTE_ASSERT(false);
				}

				out_verts[i].color = s->geom.u.sprite.c;
			}

			out_verts[0].position.x = geom.p0.x;
			out_verts[0].position.y = geom.p0.y;
			out_verts[0].uv.x = s->minx;
			out_verts[0].uv.y = s->maxy;
			out_verts[0].color = geom.c;

			out_verts[1].position.x = geom.p3.x;
			out_verts[1].position.y = geom.p3.y;
			out_verts[1].uv.x = s->minx;
			out_verts[1].uv.y = s->miny;
			out_verts[1].color = geom.c;

			out_verts[2].position.x = geom.p1.x;
			out_verts[2].position.y = geom.p1.y;
			out_verts[2].uv.x = s->maxx;
			out_verts[2].uv.y = s->maxy;
			out_verts[2].color = geom.c;

			out_verts[3].position.x = geom.p1.x;
			out_verts[3].position.y = geom.p1.y;
			out_verts[3].uv.x = s->maxx;
			out_verts[3].uv.y = s->maxy;
			out_verts[3].color = geom.c;

			out_verts[4].position.x = geom.p3.x;
			out_verts[4].position.y = geom.p3.y;
			out_verts[4].uv.x = s->minx;
			out_verts[4].uv.y = s->miny;
			out_verts[4].color = geom.c;

			out_verts[5].position.x = geom.p2.x;
			out_verts[5].position.y = geom.p2.y;
			out_verts[5].uv.x = s->maxx;
			out_verts[5].uv.y = s->miny;
			out_verts[5].color = geom.c;

			vert_count += 6;
		}	break;
		}
	}

	// Apply viewport.
	Rect viewport = draw->viewports.last();
	if (viewport.w >= 0 && viewport.h >= 0) {
		cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
	}

	// Apply scissor.
	Rect scissor = draw->scissors.last();
	if (scissor.w >= 0 && scissor.h >= 0) {
		cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
	}

	// Map the vertex buffer with sprite vertex data.
	cf_mesh_append_vertex_data(draw->mesh, verts, vert_count);
	cf_apply_mesh(draw->mesh);

	// Apply the atlas texture.
	CF_Texture atlas = { sprites->texture_id };
	cf_material_set_texture_fs(draw->material, "u_image", atlas);

	// Apply uniforms.
	v2 u_texture_size = cf_v2((float)texture_w, (float)texture_h);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);

	// Outline shader uniforms.
	v2 u_texel_size = cf_v2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_texel_size", &u_texel_size, CF_UNIFORM_TYPE_FLOAT2, 1);

	// Apply render state.
	cf_material_set_render_state(draw->material, draw->render_states.last());

	// Kick off a draw call.
	cf_apply_shader(draw->shader, draw->material);
	cf_draw_elements();
}

//--------------------------------------------------------------------------------------------------
// Hidden API called by CF_App.

void cf_make_draw()
{
	draw = CUTE_NEW(CF_Draw);

	// Setup a good default camera dimensions size.
	cf_camera_dimensions((float)app->w, (float)app->h);

	// Mesh + vertex attributes.
	draw->mesh = cf_make_mesh(CF_USAGE_TYPE_STREAM, CUTE_MB * 25, 0, 0);
	CF_VertexAttribute attrs[4] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CUTE_OFFSET_OF(DrawVertex, position);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CUTE_OFFSET_OF(DrawVertex, uv);
	attrs[2].name = "in_col";
	attrs[2].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[2].offset = CUTE_OFFSET_OF(DrawVertex, color);
	attrs[3].name = "in_params";
	attrs[3].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[3].offset = CUTE_OFFSET_OF(DrawVertex, alpha);
	cf_mesh_set_attributes(draw->mesh, attrs, CUTE_ARRAY_SIZE(attrs), sizeof(DrawVertex), 0);

	// Shaders.
	draw->shader = CF_MAKE_SOKOL_SHADER(sprite_shd);

	// Material.
	draw->material = cf_make_material();
	CF_RenderState state = cf_render_state_defaults();
	state.blend.enabled = true;
	state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.rgb_op = CF_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.alpha_op = CF_BLEND_OP_ADD;
	draw->render_states.add(state);
	cf_material_set_render_state(draw->material, state);

	// Spritebatcher.
	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_draw_report;
	config.get_pixels_callback = cf_get_pixels;
	config.generate_texture_callback = cf_generate_texture_handle;
	config.delete_texture_callback = cf_destroy_texture_handle;
	config.allocator_context = NULL;
	config.lonely_buffer_count_till_flush = 0;

	if (spritebatch_init(&draw->sb, &config, NULL)) {
		CUTE_FREE(draw);
		draw = NULL;
		CUTE_ASSERT(false);
	}
}

void cf_destroy_draw()
{
	spritebatch_term(&draw->sb);
	cf_destroy_mesh(draw->mesh);
	cf_destroy_material(draw->material);
	cf_destroy_shader(draw->shader);
	draw->~CF_Draw();
	CUTE_FREE(draw);
}

//--------------------------------------------------------------------------------------------------

void cf_draw_sprite(const CF_Sprite* sprite)
{
	spritebatch_sprite_t s = { };
	s.image_id = sprite->animation->frames[sprite->frame_index].id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;

	v2 p = cf_add_v2(sprite->transform.p, sprite->local_offset);

	// Expand sprite's scale to account for border pixels in the atlas.
	v2 scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	scale.x = scale.x + (scale.x / (float)sprite->w) * 2.0f;
	scale.y = scale.y + (scale.y / (float)sprite->h) * 2.0f;

	CF_V2 quad[] = {
		{ -0.5f,  0.5f },
		{  0.5f,  0.5f },
		{  0.5f, -0.5f },
		{ -0.5f, -0.5f },
	};

	// Construct quad in local space.
	for (int j = 0; j < 4; ++j) {
		float x = quad[j].x;
		float y = quad[j].y;

		float x0 = sprite->transform.r.c * x - sprite->transform.r.s * y;
		float y0 = sprite->transform.r.s * x + sprite->transform.r.c * y;
		x = x0;
		y = y0;

		x *= scale.x;
		y *= scale.y;

		x += p.x;
		y += p.y;

		quad[j].x = x;
		quad[j].y = y;
	}

	// Transform quad to camera space.
	s.geom.u.sprite.p0 = mul(draw->cam, quad[0]);
	s.geom.u.sprite.p1 = mul(draw->cam, quad[1]);
	s.geom.u.sprite.p2 = mul(draw->cam, quad[2]);
	s.geom.u.sprite.p3 = mul(draw->cam, quad[3]);
	s.geom.u.sprite.is_sprite = true;
	s.geom.u.sprite.c = premultiply(to_pixel(draw->tints.last()));
	s.geom.alpha = sprite->opacity;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_quad(CF_Aabb bb, float thickness)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_draw_quad2(verts[0], verts[1], verts[2], verts[3], thickness);
}

static void s_draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3, bool antialias)
{
	if (antialias) {
		CF_V2 verts[] = { p0, p1, p2, p3 };
		cf_draw_polyline(verts, 4, thickness, true, 0);
	} else {
		float sqrt_2 = 1.41421356237f;
		CF_V2 n = cf_v2(sqrt_2, sqrt_2) * thickness;
		CF_V2 q0 = p0 + cf_v2(-n.x, -n.y);
		CF_V2 q1 = p1 + cf_v2( n.x, -n.y);
		CF_V2 q2 = p2 + cf_v2( n.x,  n.y);
		CF_V2 q3 = p3 + cf_v2(-n.x,  n.y);
		cf_draw_quad_fill3(p0, p1, q1, q0, c0, c1, c2, c3);
		cf_draw_quad_fill3(p1, p2, q2, q1, c0, c1, c2, c3);
		cf_draw_quad_fill3(p2, p3, q3, q2, c0, c1, c2, c3);
		cf_draw_quad_fill3(p3, p0, q0, q3, c0, c1, c2, c3);
	}
}

void cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness)
{
	CF_Color c = draw->colors.last();
	s_draw_quad(p0, p1, p2, p3, thickness, c, c, c, c, draw->antialias.last());
}

void cf_draw_quad3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	s_draw_quad(p0, p1, p2, p3, thickness, c0, c1, c2, c3, false);
}

void cf_draw_quad_fill(CF_Aabb bb)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_draw_quad_fill2(verts[0], verts[1], verts[2], verts[3]);
}

void cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_QUAD;
	s.geom.u.quad.p0 = mul(m, p0);
	s.geom.u.quad.p1 = mul(m, p1);
	s.geom.u.quad.p2 = mul(m, p2);
	s.geom.u.quad.p3 = mul(m, p3);
	s.geom.u.quad.c0 = s.geom.u.quad.c1 = s.geom.u.quad.c2 = s.geom.u.quad.c3 = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_QUAD;
	s.geom.u.quad.p0 = mul(m, p0);
	s.geom.u.quad.p1 = mul(m, p1);
	s.geom.u.quad.p2 = mul(m, p2);
	s.geom.u.quad.p3 = mul(m, p3);
	s.geom.u.quad.c0 = premultiply(to_pixel(cf_overlay_color(c0, draw->tints.last())));
	s.geom.u.quad.c1 = premultiply(to_pixel(cf_overlay_color(c1, draw->tints.last())));
	s.geom.u.quad.c2 = premultiply(to_pixel(cf_overlay_color(c2, draw->tints.last())));
	s.geom.u.quad.c3 = premultiply(to_pixel(cf_overlay_color(c3, draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_circle(CF_V2 p, float r, int iters, float thickness)
{
	if (draw->antialias.last()) {
		draw->temp.ensure_capacity(iters);
		draw->temp.clear();
		CF_V2 p0 = cf_v2(p.x + r, p.y);
		draw->temp.add(p0);

		for (int i = 1; i < iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p1 = p + n * r;
			draw->temp.add(p1);
			p0 = p1;
		}

		cf_draw_polyline(draw->temp.data(), draw->temp.size(), thickness, true, 0);
	} else {
		float half_thickness = thickness * 0.5f;
		CF_V2 p0 = cf_v2(p.x + r - half_thickness, p.y);
		CF_V2 p1 = cf_v2(p.x + r + half_thickness, p.y);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p2 = p + n * (r + half_thickness);
			CF_V2 p3 = p + n * (r - half_thickness);
			cf_draw_quad_fill2(p0, p1, p2, p3);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_draw_circle_fill(CF_V2 p, float r, int iters)
{
	CF_V2 prev = cf_v2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		CF_V2 next = cf_from_angle(a) * r;
		cf_draw_tri_fill(p + prev, p + next, p);
		prev = next;
	}
}

static void s_circle_arc_aa(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness)
{
	float r = cf_len(center_of_arc - p);
	CF_V2 d = cf_norm(center_of_arc - p);
	CF_SinCos m = cf_sincos_f(range * 0.5f);

	CF_V2 t = cf_mul_sc_v2(m, d);
	CF_V2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;
	draw->temp.add(p0);

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		CF_V2 p1 = p + t * r;
		draw->temp.add(p1);
		p0 = p1;
	}
}

void cf_draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness)
{
	if (draw->antialias.last()) {
		draw->temp.ensure_capacity(iters);
		draw->temp.clear();
		s_circle_arc_aa(p, center_of_arc, range, iters, thickness);
		cf_draw_polyline(draw->temp.data(), draw->temp.size(), thickness, false, 3);
	} else {
		float r = cf_len(center_of_arc - p);
		CF_V2 d = cf_norm(center_of_arc - p);
		CF_SinCos m = cf_sincos_f(range * 0.5f);

		float half_thickness = thickness * 0.5f;
		CF_V2 t = cf_mul_sc_v2(m, d);
		CF_V2 p0 = p + t * (r + half_thickness);
		CF_V2 p1 = p + t * (r - half_thickness);
		d = cf_norm(p0 - p);
		float inc = range / iters;

		for (int i = 1; i <= iters; i++) {
			m = cf_sincos_f(i * inc);
			t = cf_mul_sc_v2(m, d);
			CF_V2 p2 = p + t * (r + half_thickness);
			CF_V2 p3 = p + t * (r - half_thickness);
			cf_draw_quad_fill2(p0, p1, p2, p3);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters)
{
	float r = cf_len(center_of_arc - p);
	CF_V2 d = cf_norm(center_of_arc - p);
	CF_SinCos m = cf_sincos_f(range * 0.5f);

	CF_V2 t = cf_mul_sc_v2(m, d);
	CF_V2 p0 = p + t * r;
	d = cf_norm(p0 - p);
	float inc = range / iters;

	for (int i = 1; i <= iters; i++) {
		m = cf_sincos_f(i * inc);
		t = cf_mul_sc_v2(m, d);
		CF_V2 p1 = p + t * r;
		cf_draw_tri_fill(p, p1, p0);
		p0 = p1;
	}
}

void cf_draw_capsule(CF_V2 a, CF_V2 b, float r, int iters, float thickness)
{
	if (draw->antialias.last()) {
		draw->temp.ensure_capacity(iters * 2 + 2);
		draw->temp.clear();
		s_circle_arc_aa(a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness);
		s_circle_arc_aa(b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness);
		cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, true, 0);
	} else {
		cf_draw_circle_arc(a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness);
		cf_draw_circle_arc(b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness);
		CF_V2 n = cf_skew(cf_norm(b - a)) * r;
		CF_V2 q0 = a + n;
		CF_V2 q1 = b + n;
		CF_V2 q2 = b - n;
		CF_V2 q3 = a - n;
		cf_draw_line(q0, q1, thickness);
		cf_draw_line(q2, q3, thickness);
	}
}

void cf_draw_capsule_fill(CF_V2 a, CF_V2 b, float r, int iters)
{
	cf_draw_circle_arc_fill(a, a + cf_norm(a - b) * r, CUTE_PI, iters);
	cf_draw_circle_arc_fill(b, b + cf_norm(b - a) * r, CUTE_PI, iters);
	CF_V2 n = cf_skew(cf_norm(b - a)) * r;
	CF_V2 q0 = a + n;
	CF_V2 q1 = b + n;
	CF_V2 q2 = b - n;
	CF_V2 q3 = a - n;
	cf_draw_quad_fill2(q0, q1, q2, q3);
}

void cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness)
{
	draw->temp.ensure_capacity(3);
	draw->temp.clear();
	draw->temp.add(p0);
	draw->temp.add(p1);
	draw->temp.add(p2);
	cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, true, 0);
}

void cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
	s.geom.u.tri.p0 = mul(m, p0);
	s.geom.u.tri.p1 = mul(m, p1);
	s.geom.u.tri.p2 = mul(m, p2);
	s.geom.u.tri.c0 = s.geom.u.tri.c1 = s.geom.u.tri.c2 = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_tri_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
	s.geom.u.tri.p0 = mul(m, p0);
	s.geom.u.tri.p1 = mul(m, p1);
	s.geom.u.tri.p2 = mul(m, p2);
	s.geom.u.tri.c0 = premultiply(to_pixel(c0));
	s.geom.u.tri.c1 = premultiply(to_pixel(c1));
	s.geom.u.tri.c2 = premultiply(to_pixel(c2));
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness)
{
	CF_Color c = draw->colors.last();
	cf_draw_line2(p0, p1, thickness, c, c);
}

void cf_draw_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1)
{
	float scale = (float)app->w / draw->cam_dimensions.x; // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (draw->antialias.last()) {
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
			cf_draw_quad_fill3(q0, q1, q2, q3, c0, c0, c1, c1);

			// Zero opacity aliased quads.
			CF_V2 n2 = cf_cw90(n0) * alias_scale;
			CF_V2 q4 = q3 + n2;
			CF_V2 q5 = q2 + n2;
			CF_V2 q6 = q1 - n2;
			CF_V2 q7 = q0 - n2;
			cf_draw_quad_fill3(q3, q2, q5, q4, c0, c1, c3, c2);
			cf_draw_quad_fill3(q0, q7, q6, q1, c0, c2, c3, c1);

			// End caps.
			n0 = n0 * alias_scale;
			CF_V2 r0 = q5 + n0;
			CF_V2 r1 = q2 + n0;
			CF_V2 r2 = q1 + n0;
			CF_V2 r3 = q6 + n0;
			cf_draw_quad_fill3(q2, r1, r0, q5, c1, c3, c3, c3);
			cf_draw_quad_fill3(q2, q1, r2, r1, c1, c1, c3, c3);
			cf_draw_quad_fill3(q1, q6, r3, r2, c1, c3, c3, c3);

			CF_V2 r4 = q4 - n0;
			CF_V2 r5 = q3 - n0;
			CF_V2 r6 = q0 - n0;
			CF_V2 r7 = q7 - n0;
			cf_draw_quad_fill3(q3, r5, r4, q4, c0, c2, c2, c2);
			cf_draw_quad_fill3(q3, q0, r6, r5, c0, c0, c2, c2);
			cf_draw_quad_fill3(q0, q7, r7, r6, c0, c2, c2, c2);
		} else {
			// Zero opacity aliased quads, without any core line.
			CF_V2 n = cf_skew(cf_norm(p1 - p0)) * alias_scale * 2.0f;
			CF_V2 q0 = p0 + n;
			CF_V2 q1 = p1 + n;
			CF_V2 q2 = p1 - n;
			CF_V2 q3 = p0 - n;
			cf_draw_quad_fill3(p0, p1, q1, q0, c0, c1, c3, c2);
			cf_draw_quad_fill3(p1, p0, q3, q2, c1, c0, c3, c2);
		}
	} else {
		CF_V2 n = cf_skew(cf_norm(p1 - p0)) * thickness * 0.5f;
		CF_V2 q0 = p0 + n;
		CF_V2 q1 = p1 + n;
		CF_V2 q2 = p1 - n;
		CF_V2 q3 = p0 - n;
		cf_draw_quad_fill3(q0, q1, q2, q3, c0, c0, c1, c1);
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
		cf_draw_tri_fill(b, p0, p3);
		cf_draw_quad_fill3(p3, p2, p1, p0, c0, c1, c1, c0);
		p0 = p3;
		p1 = p2;
	}
	cf_draw_tri_fill(b, i4, p0);
	cf_draw_quad_fill3(p0, i4, f4, p1, c0, c0, c1, c1);
}

CUTE_INLINE static void s_bevel_arc(CF_V2 b, CF_V2 i3, CF_V2 i4, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	CF_SinCos r = cf_sincos_f(arc);
	CF_V2 p0 = i3;
	for (int i = 1; i < bevel_count; ++i) {
		CF_V2 p3 = s_rot_b_about_a(r, b, p0);
		cf_draw_tri_fill(b, p0, p3);
		p0 = p3;
	}
	cf_draw_tri_fill(b, i4, p0);
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
						cf_draw_quad_fill2(a, b, i3, i0);
						cf_draw_quad_fill2(i1, i2, b, a);
						cf_draw_quad_fill3(i0, i3, f3, f0, c0, c0, c1, c1);
						cf_draw_quad_fill3(f1, f2, i2, i1, c1, c1, c0, c0);
					}
					f0 = f3;
					f1 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i0, c0, c0, c1, c1);
					cf_draw_quad_fill3(i1, i2, b, a, c1, c1, c0, c0);
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
						cf_draw_quad_fill2(a, b, i3, i0);
						cf_draw_quad_fill2(i1, i2, b, a);
						cf_draw_quad_fill3(i0, i3, f3, f0, c0, c0, c1, c1);
						cf_draw_quad_fill3(i1, f1, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f0 = f4;
					f1 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i0, c0, c0, c1, c1);
					cf_draw_quad_fill3(i1, i2, b, a, c1, c1, c0, c0);
					s_bevel_arc(b, i3, i4, bevel_count);
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
						cf_draw_quad_fill2(a, b, i3, i1);
						cf_draw_quad_fill2(i0, i2, b, a);
						cf_draw_quad_fill3(i1, i3, f3, f1, c0, c0, c1, c1);
						cf_draw_quad_fill3(f0, f2, i2, i0, c1, c1, c0, c0);
					}
					f1 = f3;
					f0 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i1, c0, c0, c1, c1);
					cf_draw_quad_fill3(i0, i2, b, a, c1, c1, c0, c0);
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
						cf_draw_quad_fill2(a, b, i3, i1);
						cf_draw_quad_fill2(i0, i2, b, a);
						cf_draw_quad_fill3(i1, i3, f3, f1, c0, c0, c1, c1);
						cf_draw_quad_fill3(i0, f0, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f1 = f4;
					f0 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i1, c0, c0, c1, c1);
					cf_draw_quad_fill3(i0, i2, b, a, c1, c1, c0, c0);
					s_bevel_arc(b, i3, i4, bevel_count);
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
					cf_draw_quad_fill2(a, b, i2, i0);
					cf_draw_quad_fill2(i1, i3, b, a);
					cf_draw_quad_fill3(i0, i2, f2, f0, c0, c0, c1, c1);
					cf_draw_quad_fill3(i1, f1, f3, i3, c0, c1, c1, c0);
				}
				f1 = f3;
				f0 = f2;
			} else if (emit) {
				cf_draw_quad_fill3(a, b, i2, i0, c0, c0, c1, c1);
				cf_draw_quad_fill3(i1, i3, b, a, c1, c1, c0, c0);
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
			cf_draw_quad_fill2(a, b, b + n0, i0);
			cf_draw_quad_fill2(a, i1, b - n0, b);
			cf_draw_quad_fill3(f0, i0, b + n0, b + fn0, c1, c0, c0, c1);
			cf_draw_quad_fill3(b - fn0, b - n0, i1, f1, c1, c0, c0, c1);

			// End caps.
			CF_V2 n = cf_norm(b - a) * alias_scale;
			cf_draw_quad_fill3(b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_draw_quad_fill3(b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_draw_quad_fill3(b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);

			a = points[1];
			b = points[0];
			n = cf_norm(b - a) * alias_scale;
			n0 = cf_skew(cf_norm(b - a)) * inner_half;
			fn0 = cf_norm(n0) * outer_half;
			cf_draw_quad_fill3(b - n0, b + n0, b + n0 + n, b - n0 + n, c0, c0, c1, c1);
			cf_draw_quad_fill3(b - fn0, b - n0, b - n0 + n, b - fn0 + n, c1, c0, c1, c1);
			cf_draw_quad_fill3(b + fn0, b + n0, b + n0 + n, b + fn0 + n, c1, c0, c1, c1);
		} else {
			cf_draw_quad_fill3(a, b, b + n0, i0, c0, c0, c1, c1);
			cf_draw_quad_fill3(a, i1, b - n0, b, c0, c1, c1, c0);
		}
	}
}

void cf_draw_polyline(CF_V2* points, int count, float thickness, bool loop, int bevel_count)
{
	CUTE_ASSERT(count >= 3);
	CF_Color color = draw->colors.last();
	float scale = (float)app->w / draw->cam_dimensions.x; // Assume x/y uniform scaling.
	float alias_scale = 1.0f / scale;
	bool thick_line = thickness > alias_scale;
	thickness = cf_max(thickness, alias_scale);
	if (draw->antialias.last()) {
		CF_Color no_alpha = color;
		no_alpha.a = 0;
		if (thick_line) {
			s_polyline(points, count, thickness, color, no_alpha, loop, true, alias_scale, bevel_count);
		} else {
			s_polyline(points, count, alias_scale * 2.0f, color, no_alpha, loop, false, 0, bevel_count);
		}
	} else {
		s_polyline(points, count, thickness, color, color, loop, false, 0, bevel_count);
	}
}

void cf_draw_bezier_line(CF_V2 a, CF_V2 c0, CF_V2 b, int iters, float thickness)
{
	draw->temp.ensure_capacity(iters);
	draw->temp.clear();
	float step = 1.0f / (float)iters;
	draw->temp.add(a);
	for (int i = 1; i < iters; ++i) {
		CF_V2 p = cf_bezier(a, c0, b, i * step);
		draw->temp.add(p);
	}
	draw->temp.add(b);
	cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, false, 1);
}

void cf_draw_bezier_line2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, int iters, float thickness)
{
	draw->temp.ensure_capacity(iters);
	draw->temp.clear();
	float step = 1.0f / (float)iters;
	draw->temp.add(a);
	for (int i = 1; i < iters; ++i) {
		CF_V2 p = cf_bezier2(a, c0, c1, b, i * step);
		draw->temp.add(p);
	}
	draw->temp.add(b);
	cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, false, 1);
}

CF_Result cf_make_font_mem(void* data, int size, const char* font_name)
{
	font_name = sintern(font_name);
	CF_Font* font = (CF_Font*)CUTE_NEW(CF_Font);
	font->file_data = (uint8_t*)data;
	if (!stbtt_InitFont(&font->info, font->file_data, stbtt_GetFontOffsetForIndex(font->file_data, 0))) {
		CUTE_FREE(data);
		CUTE_FREE(font);
		return result_failure("Failed to parse ttf file with stb_truetype.h.");
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

	int x0, y0, x1, y1;
	stbtt_GetFontBoundingBox(&font->info, &x0, &y0, &x1, &y1);
	font->width = x1 - x0;

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

	return result_success();
}

CF_Result cf_make_font(const char* path, const char* font_name)
{
	size_t size;
	void* data = fs_read_entire_file_to_memory(path, &size);
	if (!data) {
		return cf_result_error("Unable to open font file.");;
	}
	return cf_make_font_mem(data, (int)size, font_name);
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
	int pad = blur + 2;
	float scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
	int xadvance, lsb, x0, y0, x1, y1;
	stbtt_GetGlyphHMetrics(&font->info, glyph->index, &xadvance, &lsb);
	stbtt_GetGlyphBitmapBox(&font->info, glyph->index, scale, scale, &x0, &y0, &x1, &y1);
	int w = x1 - x0 + pad*2;
	int h = y1 - y0 + pad*2;
	glyph->w = w;
	glyph->h = h;
	glyph->q0 = V2((float)x0, -(float)(y0 + h)); // Swapped y.
	glyph->q1 = V2((float)(x0 + w), -(float)y0); // Swapped y.
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
			glyph_index = 0xFFFD;
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

void cf_push_font(const char* font)
{
	draw->fonts.add(sintern(font));
}

const char* cf_pop_font()
{
	if (draw->fonts.count() > 1) {
		return draw->fonts.pop();
	} else {
		return draw->fonts.last();
	}
}

const char* cf_peek_font()
{
	return draw->fonts.last();
}

void cf_push_font_size(float size)
{
	draw->font_sizes.add(size);
}

float cf_pop_font_size()
{
	if (draw->font_sizes.count() > 1) {
		return draw->font_sizes.pop();
	} else {
		return draw->font_sizes.last();
	}
}

float cf_peek_font_size()
{
	return draw->font_sizes.last();
}

void cf_push_font_blur(int blur)
{
	draw->blurs.add(blur);
}

int cf_pop_font_blur()
{
	if (draw->blurs.count() > 1) {
		return draw->blurs.pop();
	} else {
		return draw->blurs.last();
	}
}

int cf_peek_font_blur()
{
	return draw->blurs.last();
}

void cf_push_text_wrap_width(float width)
{
	draw->text_wrap_widths.add(width);
}

float cf_pop_text_wrap_width()
{
	if (draw->text_wrap_widths.count() > 1) {
		return draw->text_wrap_widths.pop();
	} else {
		return draw->text_wrap_widths.last();
	}
}

float cf_peek_text_wrap_width()
{
	return draw->text_wrap_widths.last();
}

void cf_push_text_clip_box(CF_Aabb clip_box)
{
	draw->text_clip_boxes.add(clip_box);
}

CF_Aabb cf_pop_text_clip_box()
{
	if (draw->text_clip_boxes.count() > 1) {
		return draw->text_clip_boxes.pop();
	} else {
		return draw->text_clip_boxes.last();
	}
}

CF_Aabb cf_peek_text_clip_box()
{
	return draw->text_clip_boxes.last();
}

void cf_push_text_vertical_layout(bool layout_vertically)
{
	draw->vertical.add(layout_vertically);
}

bool cf_pop_text_vertical_layout()
{
	if (draw->vertical.count() > 1) {
		return draw->vertical.pop();
	} else {
		return draw->vertical.last();
	}
}

bool cf_peek_text_vertical_layout()
{
	return draw->vertical.last();
}

float cf_text_width(const char* text)
{
	CF_Font* font = cf_font_get(draw->fonts.last());
	float font_size = draw->font_sizes.last();
	int blur = draw->blurs.last();
	float x = 0;
	float w = 0;
	int cp;

	while (*text) {
		text = cf_decode_UTF8(text, &cp);
		if (cp == '\n' || cp == '\r') {
			x = 0;
		} else {
			CF_Glyph* glyph = cf_font_get_glyph(font, cp, font_size, blur);
			x += glyph->xadvance;
			w = (x > w) ? x : w;
		}
	}

	return w;
}

float cf_text_height(const char* text)
{
	CF_Font* font = cf_font_get(draw->fonts.last());
	float font_size = draw->font_sizes.last();
	float font_height, h;
	h = font_height = font->line_height * stbtt_ScaleForPixelHeight(&font->info, font_size);
	int cp;

	while (*text) {
		text = cf_decode_UTF8(text, &cp);
		if (cp == '\n' && *text) h += font_height; 
	}

	return h;
}

static bool s_is_space(int cp)
{
	switch (cp) {
	case ' ':
	case '\n':
	case '\t':
	case '\v':
	case '\f':
	case '\r': return true;
	default:   return false;
	}
}

static const char* s_find_end_of_line(CF_Font* font, const char* text, float wrap_width)
{
	float font_size = draw->font_sizes.last();
	int blur = draw->blurs.last();
	float x = 0;
	const char* start_of_word = 0;
	float word_w = 0;
	int cp;

	while (*text) {
		const char* text_prev = text;
		text = cf_decode_UTF8(text, &cp);
		CF_Glyph* glyph = cf_font_get_glyph(font, cp, font_size, blur);

		if (cp == '\n') {
			x = 0;
			word_w = 0;
			start_of_word = 0;
			continue;
		} else if (cp == '\r') {
			continue;
		} else {
			if (s_is_space(cp)) {
				x += word_w + glyph->xadvance;
				word_w = 0;
				start_of_word = 0;
			} else {
				if (!start_of_word) {
					start_of_word = text_prev;
				}
				if (x + word_w + glyph->xadvance < wrap_width) {
					word_w += glyph->xadvance;
				} else {
					if (word_w + glyph->xadvance < wrap_width) {
						// Put entire word on the next line.
						return start_of_word;
					} else {
						// Word itself does not fit on one line, so just cut it here.
						return text;
					}
				}
			}
		}
	}

	return text + 1;
}

struct CF_CodeParseState
{
	CF_TextEffectState* effect;
	const char* in;
	const char* end;
	int glyph_count;
	String sanitized;

	bool done() { return in >= end; }
	void append(int ch) { sanitized.append(ch); ++glyph_count; }
	void ltrim() { while (!done()) { int cp = *in; if (s_is_space(cp)) ++in; else break; } }
	int next(bool trim = true) { if (trim) ltrim(); int cp; in = cf_decode_UTF8(in, &cp); return cp; }
	int peek(bool trim = true) { if (trim) ltrim(); int cp; cf_decode_UTF8(in, &cp); return cp; }
	void skip(bool trim = true) { if (trim) ltrim(); int cp; in = cf_decode_UTF8(in, &cp); }
	bool expect(int ch) { int cp = next(); if (cp != ch) { return false; } return true; }
	bool try_next(int ch, bool trim = true) { if (trim) ltrim(); int cp; const char* next = cf_decode_UTF8(in, &cp); if (cp == ch) { in = next; return true; } return false; }
};

static String s_parse_code_name(CF_CodeParseState* s)
{
	String name;
	while (!s->done()) {
		int cp = s->peek(false);
		if (cp == '=' || cp == '>') {
			return name;
		} else if (cp == '/') {
			s->skip();
			if (s->try_next('>')) {
				s->append('>');
			} else {
				s->append('/');
			}
		} else if (s_is_space(cp)) {
			return name;
		} else {
			name.append(cp);
			s->skip(false);
		}
	}
	return name;
}

static bool s_is_hex_alphanum(int ch)
{
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
	case '8': case '9': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': return true;
	default: return false;
	}
}

static CF_Color s_parse_color(CF_CodeParseState* s)
{
	String string;
	s->expect('#');
	int digits = 0;
	while (!s->done()) {
		int cp = s->peek();
		if (!s_is_hex_alphanum(cp)) {
			break;
		} else {
			string.append(cp);
			++digits;
			s->skip();
		}
	}
	int hex = (int)string.to_hex();
	if (digits == 6) {
		// Treat the color as opaque if only 3 bytes were found.
		hex = hex << 8 | 0xFF;
	}
	CF_Color result = make_color(hex);
	return result;
}

static bool s_is_num(int ch)
{
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return true;
	default: return false;
	}
}

static double s_parse_number(CF_CodeParseState* s)
{
	String string;
	bool is_float = false;
	bool is_neg = false;
	if (s->try_next('-')) {
		is_neg = true;
	} else {
		s->try_next('+');
	}
	while (!s->done()) {
		int cp = s->peek();
		if (cp == '.') {
			string.append('.');
			s->skip(false);
			is_float = true;
		} else if (!s_is_num(cp)) {
			break;
		} else {
			string.append(cp);
			s->skip(false);
		}
	}
	double result;
	if (is_float) {
		result = string.to_double();
	} else {
		result = (double)string.to_int();
	}
	if (is_neg) result = -result;
	return result;
}

static String s_parse_string(CF_CodeParseState* s)
{
	String string;
	s->expect('"');
	while (!s->done()) {
		int cp = s->next(false);
		if (cp == '"') {
			break;
		} else if (cp == '/') {
			if (s->peek(false) == '"') {
				string.append('"');
				s->skip();
			}
		} else {
			string.append(cp);
		}
	}
	return string;
}

static CF_TextCodeVal s_parse_code_val(CF_CodeParseState* s)
{
	CF_TextCodeVal val = { };
	int cp = s->peek();
	if (cp == '#') {
		CF_Color c = s_parse_color(s);
		val.type = CF_TEXT_CODE_VAL_TYPE_COLOR;
		val.u.color = c;
	} else if (cp == '"') {
		String string = s_parse_string(s);
		val.type = CF_TEXT_CODE_VAL_TYPE_STRING;
		val.u.string = sintern(string.c_str());
	} else {
		double number = s_parse_number(s);
		val.type = CF_TEXT_CODE_VAL_TYPE_NUMBER;
		val.u.number = number;
	}
	return val;
}

static void s_parse_code(CF_CodeParseState* s)
{
	CF_TextCode code = { };
	bool finish = s->try_next('/');
	bool first = true;
	while (!s->done()) {
		const char* name = sintern(s_parse_code_name(s).c_str());
		if (first) {
			first = false;
			code.effect_name = name;
			code.fn = app->text_effect_fns.find(name);
		}
		if (s->try_next('=')) {
			CF_TextCodeVal val = s_parse_code_val(s);
			code.params.insert(name, val);
		}
		if (s->try_next('>')) {
			break;
		}
	}
	code.index_in_string = s->glyph_count;
	if (finish) {
		bool success = s->effect->parse_finish(code.effect_name, code.index_in_string);
		// TODO - Error handling.
	} else {
		s->effect->parse_add(code);
	}
}

static bool s_text_fx_color(TextEffect* fx)
{
	CF_Color c = fx->get_color("color");
	fx->color = c;
	return true;
}

static bool s_text_fx_shake(TextEffect* effect)
{
	double freq = effect->get_number("freq", 35);
	int seed = (int)(effect->elapsed * freq);
	float x = (float)effect->get_number("x", 2);
	float y = (float)effect->get_number("y", 2);
	CF_Rnd rnd = cf_rnd_seed(seed);
	v2 offset = V2(rnd_next_range(rnd, -x, y), rnd_next_range(rnd, -x, y));
	effect->q0 += offset;
	effect->q1 += offset;
	return true;
}

static bool s_text_fx_fade(TextEffect* effect)
{
	double speed = effect->get_number("speed", 2);
	double span = effect->get_number("span", 5);
	effect->opacity = cosf((float)(effect->elapsed * speed + effect->index_into_effect / span)) * 0.5f + 0.5f;
	return true;
}

static bool s_text_fx_wave(TextEffect* effect)
{
	double speed = effect->get_number("speed", 5);
	double span = effect->get_number("span", 10);
	double height = effect->get_number("height", 5);
	float offset = (cosf((float)(effect->elapsed * speed + effect->index_into_effect / span)) * 0.5f + 0.5f) * (float)height;
	effect->q0.y += offset;
	effect->q1.y += offset;
	return true;
}

static bool s_text_fx_strike(TextEffect* effect)
{
	if (!s_is_space(effect->character) || effect->character == ' ') {
		v2 hw = V2((float)effect->xadvance, 0) * 0.5f;
		float h = effect->font_size / 20.0f;
		h = (float)effect->get_number("strike", (double)h);
		CF_Strike strike;
		strike.p0 = effect->center - hw;
		strike.p1 = effect->center + hw;
		strike.thickness = h;
		draw->strikes.add(strike);
	}
	return true;
}

static void s_parse_codes(CF_TextEffectState* effect, const char* text)
{
	// Register built-in text effects.
	static bool init = false;
	if (!init) {
		init = true;
		text_effect_register("color", s_text_fx_color);
		text_effect_register("shake", s_text_fx_shake);
		text_effect_register("fade", s_text_fx_fade);
		text_effect_register("wave", s_text_fx_wave);
		text_effect_register("strike", s_text_fx_strike);
	}

	CF_CodeParseState state = { };
	CF_CodeParseState* s = &state;
	s->effect = effect;
	s->in = text;
	s->end = text + CUTE_STRLEN(text);
	while (!s->done()) {
		int cp = s->next(false);
		if (cp == '/' && s->try_next('<', false)) {
			s->append('<');
		} else if (cp == '<') {
			s_parse_code(s);
		} else {
			s->append(cp);
		}
	}
	std::sort(effect->codes.begin(), effect->codes.end(),
		[](const CF_TextCode& a, const CF_TextCode&b) {
			return a.index_in_string < b.index_in_string;
		}
	);
	effect->sanitized = s->sanitized;
}

void cf_draw_text(const char* text, CF_V2 position)
{
	CF_Font* font = cf_font_get(draw->fonts.last());
	CUTE_ASSERT(font);
	if (!font) return;

	// Cache effect state key'd by input text pointer.
	CF_TextEffectState* effect_state = app->text_effect_states.try_find(text);
	if (!effect_state) {
		effect_state = app->text_effect_states.insert(text);
		effect_state->hash = fnv1a(text, (int)CUTE_STRLEN(text) + 1);
		s_parse_codes(effect_state, text);
	} else {
		uint64_t h = fnv1a(text, (int)CUTE_STRLEN(text) + 1);
		if (effect_state->hash != h) {
			// Contents have changed, re-parse the whole thing.
			app->text_effect_states.remove(text);
			effect_state = app->text_effect_states.insert(text);
			effect_state->hash = h;
			s_parse_codes(effect_state, text);
		}
	}
	effect_state->alive = true;
	effect_state->elapsed += CF_DELTA_TIME;

	// Use the sanitized string for rendering. This excludes all text codes.
	text = effect_state->sanitized.c_str();

	// Gather up all state required for rendering.
	float font_size = draw->font_sizes.last();
	int blur = draw->blurs.last();
	bool do_clipping = draw->text_clip_boxes.size() > 1;
	CF_Aabb clip = draw->text_clip_boxes.last();
	float wrap_w = draw->text_wrap_widths.last();
	float scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
	float line_height = font->line_height * scale;
	int cp_prev = 0;
	int cp = 0;
	const char* end_of_line = NULL;
	float h = (font->ascent + font->descent) * scale;
	float w = font->width * scale;
	float x = position.x;
	float initial_y = position.y - font->ascent * scale;
	float y = initial_y;
	int index = 0;
	int code_index = 0;

	// Called whenever text-effects need to be spawned, before going to the next glyph.
	auto effect_spawn = [&]() {
		if (code_index < effect_state->codes.count()) {
			CF_TextCode* code = effect_state->codes + code_index;
			if (index == code->index_in_string) {
				++code_index;
				TextEffect effect = { };
				effect.effect_name = code->effect_name;
				effect.index_into_string = code->index_in_string;
				effect.index_into_effect = 0;
				effect.glyph_count = code->glyph_count;
				effect.elapsed = effect_state->elapsed;
				effect.params = &code->params;
				effect.fn = code->fn;
				effect_state->effects.add(effect);
			}
		}
	};

	// Called whenever text-effects need to be cleaned up, when going to the next glyph.
	auto effect_cleanup = [&]() {
		for (int i = 0; i < effect_state->effects.count();) {
			TextEffect* effect = effect_state->effects + i;
			if (effect->index_into_string + effect->glyph_count == index) {
				effect->index_into_effect = index - effect->index_into_string - 1;
				effect->fn(effect); // Signal we're done (one past the end).
				effect_state->effects.unordered_remove(i);
			} else {
				 ++i;
			}
		}
	};

	// Used by the line-wrapping algorithm to skip characters.
	auto skip_to_next = [&]() {
		text = cf_decode_UTF8(text, &cp);
		effect_cleanup();
		++index;
	};

	bool vertical = draw->vertical.last();

	auto advance_to_next_glyph = [&](float xadvance = 0) {
		if (vertical) {
			y -= line_height;
		} else {
			x += xadvance;
		}
	};

	auto apply_newline = [&]() {
		if (vertical) {
			x += w;
			y = initial_y;
		} else {
			x = position.x;
			y -= line_height;
		}
	};

	// Render the string glyph-by-glyph.
	while (*text) {
		cp_prev = cp;
		const char* prev_text = text;
		effect_spawn();
		text = cf_decode_UTF8(text, &cp);
		++index;
		CUTE_DEFER(effect_cleanup());

		if (cp == '\n') {
			apply_newline();
			continue;
		}

		// Word wrapping logic.
		if (!end_of_line) {
			end_of_line = s_find_end_of_line(font, prev_text, wrap_w);
		}
		
		int finished_rendering_line = !(text < end_of_line);
		if (finished_rendering_line) {
			end_of_line = NULL;
			apply_newline();
		
			// Skip whitespace at the beginning of new lines.
			while (cp) {
				cp = *text;
				if (cp == '\n') {
					apply_newline();
					skip_to_next();
					break;
				}
				else if (s_is_space(cp)) { skip_to_next(); }
				else break;
			}
		
			continue;
		}

		spritebatch_sprite_t s = { };
		CF_Glyph* glyph = cf_font_get_glyph(font, cp, font_size, blur);
		if (!glyph) {
			continue;
		}

		// Prepare a sprite struct for rendering.
		bool visible = glyph->visible;
		float xadvance = glyph->xadvance;
		s.image_id = glyph->image_id;
		s.w = glyph->w;
		s.h = glyph->h;
		s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
		s.geom.u.sprite;
		s.geom.alpha = 1.0f;
		CF_Color color = draw->colors.last();

		uint64_t kern_key = CF_KERN_KEY(cp_prev, cp);
		v2 kern = V2(cf_font_get_kern(font, font_size, cp_prev, cp), 0);
		v2 pad = V2(1,1); // Account for 1-pixel padding in spritebatch.
		v2 q0 = glyph->q0 + V2(x,y) + kern - pad;
		v2 q1 = glyph->q1 + V2(x,y) + kern + pad;

		// Apply any active custom text effects.
		for (int i = 0; i < effect_state->effects.count();) {
			TextEffect* effect = effect_state->effects + i;
			CF_TextEffectFn* fn = effect->fn;
			bool keep_going = true;
			if (fn) {
				effect->character = cp;
				effect->index_into_effect = index - effect->index_into_string - 1;
				effect->center = V2(x + xadvance*0.5f, y + h*0.25f);
				effect->q0 = q0;
				effect->q1 = q1;
				effect->w = s.w;
				effect->h = s.h;
				effect->color = color;
				effect->opacity = s.geom.alpha;
				effect->xadvance = xadvance;
				effect->visible = visible;
				effect->font_size = font_size;
				keep_going = fn(effect);
				q0 = effect->q0;
				q1 = effect->q1;
				color = effect->color;
				s.geom.alpha = effect->opacity;
				xadvance = effect->xadvance;
				visible = effect->visible;
				if (!keep_going) {
					effect_state->effects.unordered_remove(i);
				}
			}
			if (keep_going) {
				++i;
			}
		}

		// Actually render the sprite.
		if (visible) {
			s.geom.u.sprite.p0 = mul(draw->cam, V2(q0.x, q1.y));
			s.geom.u.sprite.p1 = mul(draw->cam, V2(q1.x, q1.y));
			s.geom.u.sprite.p2 = mul(draw->cam, V2(q1.x, q0.y));
			s.geom.u.sprite.p3 = mul(draw->cam, V2(q0.x, q0.y));
			s.geom.u.sprite.c = premultiply(to_pixel(color));
			s.geom.u.sprite.clip = make_aabb(mul(draw->cam, clip.min), mul(draw->cam, clip.max));
			s.geom.u.sprite.do_clipping = do_clipping;
			s.geom.u.sprite.is_text = true;
			s.sort_bits = draw->layers.last();

			spritebatch_push(&draw->sb, s);
		}

		advance_to_next_glyph(xadvance);
	}

	// Draw strike-lines just after the text.
	for (int i = 0; i < draw->strikes.size(); ++i) {
		v2 p0 = draw->strikes[i].p0;
		v2 p1 = draw->strikes[i].p1;
		float thickness = draw->strikes[i].thickness;
		cf_draw_line(p0, p1, thickness);
	}
	draw->strikes.clear();
}

void cf_text_effect_register(const char* name, CF_TextEffectFn* fn)
{
	app->text_effect_fns.insert(sintern(name), fn);
}

bool cf_text_effect_on_start(CF_TextEffect* fx)
{
	return ((TextEffect*)fx)->on_start();
}

bool cf_text_effect_on_finish(CF_TextEffect* fx)
{
	return ((TextEffect*)fx)->on_finish();
}

double cf_text_effect_get_number(CF_TextEffect* fx, const char* key, double default_val)
{
	return ((TextEffect*)fx)->get_number(key, default_val);
}

CF_Color cf_text_effect_get_color(CF_TextEffect* fx, const char* key, CF_Color default_val)
{
	return ((TextEffect*)fx)->get_color(key, default_val);
}

const char* cf_text_effect_get_string(CF_TextEffect* fx, const char* key, const char* default_val)
{
	return ((TextEffect*)fx)->get_string(key, default_val);
}

void cf_draw_push_layer(int layer)
{
	draw->layers.add(layer);
}

int cf_draw_pop_layer()
{
	if (draw->layers.count() > 1) {
		return draw->layers.pop();
	} else {
		return draw->layers.last();
	}
}

int cf_draw_peek_layer()
{
	return draw->layers.last();
}

void cf_draw_push_color(CF_Color c)
{
	draw->colors.add(c);
}

CF_Color cf_draw_pop_color()
{
	if (draw->colors.count() > 1) {
		return draw->colors.pop();
	} else {
		return draw->colors.last();
	}
}

CF_Color cf_draw_peek_color()
{
	return draw->colors.last();
}

void cf_draw_push_tint(CF_Color c)
{
	draw->tints.add(c);
}

CF_Color cf_draw_pop_tint()
{
	if (draw->tints.count() > 1) {
		return draw->tints.pop();
	} else {
		return draw->tints.last();
	}
}

CF_Color cf_draw_peek_tint()
{
	return draw->tints.last();
}

void cf_draw_push_antialias(bool antialias)
{
	draw->antialias.add(antialias);
}

bool cf_draw_pop_antialias()
{
	if (draw->antialias.count() > 1) {
		return draw->antialias.pop();
	} else {
		return draw->antialias.last();
	}
}

bool cf_draw_peek_antialias()
{
	return draw->antialias.last();
}

void cf_render_settings_filter(CF_Filter filter)
{
	draw->filter = filter;
}

void cf_render_settings_push_viewport(CF_Rect viewport)
{
	draw->viewports.add(viewport);
}

CF_Rect cf_render_settings_pop_viewport()
{
	if (draw->viewports.count()) {
		CF_Rect result = draw->viewports.pop();
		return result;
	} else {
		return draw->viewports.last();
	}
}

CF_Rect cf_render_settings_peek_viewport()
{
	return draw->viewports.last();
}

void cf_render_settings_push_scissor(CF_Rect scissor)
{
	draw->scissors.add(scissor);
}

CF_Rect cf_render_settings_pop_scissor()
{
	if (draw->scissors.count()) {
		CF_Rect result = draw->scissors.pop();
		return result;
	} else {
		return draw->scissors.last();
	}
}

CF_Rect cf_render_settings_peek_scissor()
{
	return draw->scissors.last();
}

void cf_render_settings_push_render_state(CF_RenderState render_state)
{
	draw->render_states.add(render_state);
}

CF_RenderState cf_render_settings_pop_render_state()
{
	if (draw->render_states.count()) {
		CF_RenderState result = draw->render_states.pop();
		return result;
	} else {
		return draw->render_states.last();
	}
}

CF_RenderState cf_render_settings_peek_render_state()
{
	return draw->render_states.last();
}

void cf_render_to(CF_Canvas canvas, bool clear)
{
	cf_apply_canvas(canvas, clear);
	spritebatch_tick(&draw->sb);
	spritebatch_defrag(&draw->sb);
	spritebatch_flush(&draw->sb);
	draw->verts.clear();
}

void cf_camera_dimensions(float w, float h)
{
	draw->cam_dimensions = V2(w, h) * 0.5f;
	draw->cam = cf_invert(cf_make_transform_TSR(draw->cam_position, draw->cam_dimensions, draw->cam_rotation));
}

void cf_camera_look_at(float x, float y)
{
	draw->cam_position = V2(x, y);
	draw->cam = cf_invert(cf_make_transform_TSR(draw->cam_position, draw->cam_dimensions, draw->cam_rotation));
}

void cf_camera_rotate(float radians)
{
	draw->cam_rotation = radians;
	draw->cam = cf_invert(cf_make_transform_TSR(draw->cam_position, draw->cam_dimensions, draw->cam_rotation));
}

void cf_camera_push()
{
	draw->cam_stack.add(draw->cam);
}

void cf_camera_pop()
{
	if (draw->cam_stack.size()) {
		draw->cam = draw->cam_stack.pop();
	} else {
		draw->cam_dimensions = V2((float)app->w, (float)app->h) * 0.5f;
		draw->cam_position = V2(0, 0);
		draw->cam_rotation = 0;
		draw->cam = cf_invert(cf_make_transform_TSR(draw->cam_position, draw->cam_dimensions, draw->cam_rotation));
	}
}

CF_TemporaryImage cf_fetch_image(const CF_Sprite* sprite)
{
	spritebatch_sprite_t s = spritebatch_fetch(&draw->sb, sprite->animation->frames[sprite->frame_index].id, sprite->w, sprite->h);
	CF_TemporaryImage image;
	image.tex = { s.texture_id };
	image.w = s.w;
	image.h = s.h;
	image.u = cf_v2(s.minx, s.miny);
	image.v = cf_v2(s.maxx, s.maxy);
	return image;
}
