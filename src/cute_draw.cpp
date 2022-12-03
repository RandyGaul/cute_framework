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
//#include <cute_debug_printf.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

#include <shaders/sprite_shader.h>

static struct CF_Draw* draw;

#include <cute/cute_png.h>

#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CUTE_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define DEBUG_VERT(v, c) batch_quad(make_aabb(v, 3, 3), c)

// Initial design of this API comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_draw.h

using namespace Cute;

void cf_get_pixels(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
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
				out_verts[i].solid = 255;
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
				out_verts[i].solid = 255;
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
			CF_V2 quad[] = {
				{ -0.5f,  0.5f },
				{  0.5f,  0.5f },
				{  0.5f, -0.5f },
				{ -0.5f, -0.5f },
			};

			// Expand sprite's scale to account for border pixels in the atlas.
			geom.scale.x = geom.scale.x + (geom.scale.x / (float)s->w) * 2.0f;
			geom.scale.y = geom.scale.y + (geom.scale.y / (float)s->h) * 2.0f;

			for (int j = 0; j < 4; ++j) {
				float x = quad[j].x;
				float y = quad[j].y;

				// Rotate sprite about origin.
				float x0 = geom.rotation.c * x - geom.rotation.s * y;
				float y0 = geom.rotation.s * x + geom.rotation.c * y;
				x = x0;
				y = y0;

				// Scale sprite about origin.
				x *= geom.scale.x;
				y *= geom.scale.y;

				// Translate sprite.
				x += geom.position.x;
				y += geom.position.y;

				quad[j].x = x;
				quad[j].y = y;
			}

			// output transformed quad into CPU buffer
			DrawVertex* out_verts = verts + vert_count;

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

			vert_count += 6;
		}	break;
		}
	}

	// Apply scissor.
	Rect scissor = draw->scissors.last();
	if (scissor.x >= 0 && scissor.y >= 0) {
		cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
	}

	// Map the vertex buffer with sprite vertex data.
	cf_mesh_append_vertex_data(draw->mesh, verts, vert_count);
	cf_apply_mesh(draw->mesh);

	// Apply the atlas texture.
	CF_Texture atlas = { sprites->texture_id };
	cf_material_set_texture_fs(draw->material, "u_image", atlas);

	// Apply uniforms.
	cf_material_set_uniform_vs(draw->material, "vs_params", "u_mvp", &draw->projection, CF_UNIFORM_TYPE_MAT4, 1);
	v2 u_texture_size = cf_v2(draw->atlas_width, draw->atlas_height);
	CF_Color u_tint = draw->tints.last();
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_tint", &u_tint, CF_UNIFORM_TYPE_FLOAT4, 1);

	// Outline shader uniforms.
	CF_Color u_border_color = draw->outline_color;
	v2 u_texel_size = cf_v2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
	float u_use_border = draw->outline_use_border;
	float u_use_corners = draw->outline_use_corners;
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_border_color", &u_border_color, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_texel_size", &u_texel_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_use_border", &u_use_border, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_use_corners", &u_use_corners, CF_UNIFORM_TYPE_FLOAT, 1);

	// Apply render state.
	cf_material_set_render_state(draw->material, draw->render_states.last());

	// Kick off a draw call.
	cf_apply_shader(draw->shader, draw->material);
	cf_draw_elements();
}

static SPRITEBATCH_U64 s_generate_texture_handle(void* pixels, int w, int h, void* udata)
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

static void s_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CUTE_UNUSED(udata);
	CF_Texture tex;
	tex.id = texture_id;
	cf_destroy_texture(tex);
}

//--------------------------------------------------------------------------------------------------
// Hidden API called by CF_App.

void cf_make_draw()
{
	draw = CUTE_NEW(CF_Draw);

	// Setup a good default projection matrix.
	draw->projection = cf_matrix_ortho_2d((float)app->w, (float)app->h, 0, 0);

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
	attrs[3].offset = CUTE_OFFSET_OF(DrawVertex, solid);
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

	// Scissor.
	CF_Rect scissor;
	scissor.w = -1;
	scissor.h = -1;
	scissor.x = 0;
	scissor.y = 0;
	draw->scissors.add(scissor);

	// Spritebatcher.
	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_draw_report;
	config.get_pixels_callback = cf_get_pixels;
	config.generate_texture_callback = s_generate_texture_handle;
	config.delete_texture_callback = s_destroy_texture_handle;
	config.allocator_context = NULL;
	config.lonely_buffer_count_till_flush = 0;

	if (spritebatch_init(&draw->sb, &config, NULL)) {
		CUTE_FREE(draw);
		draw = NULL;
		CUTE_ASSERT(false);
	}

	draw->atlas_width = (float)config.atlas_width_in_pixels;
	draw->atlas_height = (float)config.atlas_height_in_pixels;
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
	v2 p = cf_mul_m32_v2(draw->m3x2s.last(), cf_add_v2(sprite->transform.p, sprite->local_offset));
	spritebatch_sprite_t s = { };
	s.image_id = sprite->animation->frames[sprite->frame_index].id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
	s.geom.u.sprite.position = p;
	s.geom.u.sprite.rotation = sprite->transform.r;
	s.geom.u.sprite.scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	s.geom.alpha = sprite->opacity;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_sprite2(const CF_Sprite* sprite, CF_Transform transform)
{
	transform = mul(transform, sprite->transform);
	v2 p = cf_mul_m32_v2(draw->m3x2s.last(), cf_add_v2(transform.p, sprite->local_offset));
	spritebatch_sprite_t s = { };
	s.image_id = sprite->animation->frames[sprite->frame_index].id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
	s.geom.u.sprite.position = p;
	s.geom.u.sprite.rotation = sprite->transform.r;
	s.geom.u.sprite.scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	s.geom.alpha = sprite->opacity;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_quad(CF_Aabb bb, float thickness, CF_Color c, bool antialias)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_draw_quad2(verts[0], verts[1], verts[2], verts[3], thickness, c, antialias);
}

static void s_draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3, bool antialias)
{
	if (antialias) {
		CF_V2 verts[] = { p0, p1, p2, p3 };
		cf_draw_polyline(verts, 4, thickness, c0, true, true, 0);
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

void cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c, bool antialias)
{
	s_draw_quad(p0, p1, p2, p3, thickness, c, c, c, c, antialias);
}

void cf_draw_quad3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	s_draw_quad(p0, p1, p2, p3, thickness, c0, c1, c2, c3, false);
}

void cf_draw_quad_fill(CF_Aabb bb, CF_Color c)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	cf_draw_quad_fill2(verts[0], verts[1], verts[2], verts[3], c);
}

void cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c)
{
	CF_M3x2 m = draw->m3x2s.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_QUAD;
	s.geom.u.quad.p0 = mul(m, p0);
	s.geom.u.quad.p1 = mul(m, p1);
	s.geom.u.quad.p2 = mul(m, p2);
	s.geom.u.quad.p3 = mul(m, p3);
	s.geom.u.quad.c0 = s.geom.u.quad.c1 = s.geom.u.quad.c2 = s.geom.u.quad.c3 = to_pixel(c);
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3)
{
	CF_M3x2 m = draw->m3x2s.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_QUAD;
	s.geom.u.quad.p0 = mul(m, p0);
	s.geom.u.quad.p1 = mul(m, p1);
	s.geom.u.quad.p2 = mul(m, p2);
	s.geom.u.quad.p3 = mul(m, p3);
	s.geom.u.quad.c0 = to_pixel(c0);
	s.geom.u.quad.c1 = to_pixel(c1);
	s.geom.u.quad.c2 = to_pixel(c2);
	s.geom.u.quad.c3 = to_pixel(c3);
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_circle(CF_V2 p, float r, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters);
		CF_V2 p0 = cf_v2(p.x + r, p.y);
		verts.add(p0);

		for (int i = 1; i < iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p1 = p + n * r;
			verts.add(p1);
			p0 = p1;
		}

		cf_draw_polyline(verts.data(), verts.size(), thickness, color, true, true, 0);
	} else {
		float half_thickness = thickness * 0.5f;
		CF_V2 p0 = cf_v2(p.x + r - half_thickness, p.y);
		CF_V2 p1 = cf_v2(p.x + r + half_thickness, p.y);

		for (int i = 1; i <= iters; i++) {
			float a = (i / (float)iters) * (2.0f * CUTE_PI);
			CF_V2 n = cf_from_angle(a);
			CF_V2 p2 = p + n * (r + half_thickness);
			CF_V2 p3 = p + n * (r - half_thickness);
			cf_draw_quad_fill2(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_draw_circle_fill(CF_V2 p, float r, int iters, CF_Color c)
{
	CF_V2 prev = cf_v2(r, 0);

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		CF_V2 next = cf_from_angle(a) * r;
		cf_draw_tri_fill(p + prev, p + next, p, c);
		prev = next;
	}
}

static void s_circle_arc_aa(Array<CF_V2>* verts, CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color)
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

void cf_draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness, CF_Color color, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters);
		s_circle_arc_aa(&verts, p, center_of_arc, range, iters, thickness, color);
		cf_draw_polyline(verts.data(), verts.size(), thickness, color, false, true, 3);
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
			cf_draw_quad_fill2(p0, p1, p2, p3, color);
			p1 = p2;
			p0 = p3;
		}
	}
}

void cf_draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters, CF_Color color)
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
		cf_draw_tri_fill(p, p1, p0, color);
		p0 = p1;
	}
}

void cf_draw_capsule(CF_V2 a, CF_V2 b, float r, int iters, float thickness, CF_Color c, bool antialias)
{
	if (antialias) {
		Array<CF_V2> verts(iters * 2 + 2);
		s_circle_arc_aa(&verts, a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c);
		s_circle_arc_aa(&verts, b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c);
		cf_draw_polyline(verts.data(), verts.count(), thickness, c, true, true, 0);
	} else {
		cf_draw_circle_arc(a, a + cf_norm(a - b) * r, CUTE_PI, iters, thickness, c, false);
		cf_draw_circle_arc(b, b + cf_norm(b - a) * r, CUTE_PI, iters, thickness, c, false);
		CF_V2 n = cf_skew(cf_norm(b - a)) * r;
		CF_V2 q0 = a + n;
		CF_V2 q1 = b + n;
		CF_V2 q2 = b - n;
		CF_V2 q3 = a - n;
		cf_draw_line(q0, q1, thickness, c, false);
		cf_draw_line(q2, q3, thickness, c, false);
	}
}

void cf_draw_capsule_fill(CF_V2 a, CF_V2 b, float r, int iters, CF_Color c)
{
	cf_draw_circle_arc_fill(a, a + cf_norm(a - b) * r, CUTE_PI, iters, c);
	cf_draw_circle_arc_fill(b, b + cf_norm(b - a) * r, CUTE_PI, iters, c);
	CF_V2 n = cf_skew(cf_norm(b - a)) * r;
	CF_V2 q0 = a + n;
	CF_V2 q1 = b + n;
	CF_V2 q2 = b - n;
	CF_V2 q3 = a - n;
	cf_draw_quad_fill2(q0, q1, q2, q3, c);
}

void cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_draw_tri2(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, CF_Color c0, CF_Color c1, CF_Color c2, bool antialias)
{
	CUTE_ASSERT(0);
}

void cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c)
{
	CF_M3x2 m = draw->m3x2s.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
	s.geom.u.tri.p0 = mul(m, p0);
	s.geom.u.tri.p1 = mul(m, p1);
	s.geom.u.tri.p2 = mul(m, p2);
	s.geom.u.tri.c0 = s.geom.u.tri.c1 = s.geom.u.tri.c2 = to_pixel(c);
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_tri_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_Color c0, CF_Color c1, CF_Color c2)
{
	CF_M3x2 m = draw->m3x2s.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = 1;
	s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
	s.geom.u.tri.p0 = mul(m, p0);
	s.geom.u.tri.p1 = mul(m, p1);
	s.geom.u.tri.p2 = mul(m, p2);
	s.geom.u.tri.c0 = to_pixel(c0);
	s.geom.u.tri.c1 = to_pixel(c1);
	s.geom.u.tri.c2 = to_pixel(c2);
	s.geom.alpha = 1.0f;
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c, bool antialias)
{
	cf_draw_line2(p0, p1, thickness, c, c, antialias);
}

void cf_draw_line2(CF_V2 p0, CF_V2 p1, float thickness, CF_Color c0, CF_Color c1, bool antialias)
{
	float scale = draw->scale_x; // Assume x/y uniform scaling.
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
			CF_V2 n = cf_skew(cf_norm(p1 - p0)) * alias_scale * 0.5f;
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
		cf_draw_tri_fill(b, p0, p3, c0);
		cf_draw_quad_fill3(p3, p2, p1, p0, c0, c1, c1, c0);
		p0 = p3;
		p1 = p2;
	}
	cf_draw_tri_fill(b, i4, p0, c0);
	cf_draw_quad_fill3(p0, i4, f4, p1, c0, c0, c1, c1);
}

CUTE_INLINE static void s_bevel_arc(CF_V2 b, CF_V2 i3, CF_V2 i4, CF_Color c0, CF_Color c1, int bevel_count)
{
	float arc = cf_shortest_arc(cf_norm(i3 - b), cf_norm(i4 - b)) / (float)(bevel_count + 1);
	CF_SinCos r = cf_sincos_f(arc);
	CF_V2 p0 = i3;
	for (int i = 1; i < bevel_count; ++i) {
		CF_V2 p3 = s_rot_b_about_a(r, b, p0);
		cf_draw_tri_fill(b, p0, p3, c0);
		p0 = p3;
	}
	cf_draw_tri_fill(b, i4, p0, c0);
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
						cf_draw_quad_fill2(a, b, i3, i0, c0);
						cf_draw_quad_fill2(i1, i2, b, a, c0);
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
						cf_draw_quad_fill2(a, b, i3, i0, c0);
						cf_draw_quad_fill2(i1, i2, b, a, c0);
						cf_draw_quad_fill3(i0, i3, f3, f0, c0, c0, c1, c1);
						cf_draw_quad_fill3(i1, f1, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f0 = f4;
					f1 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i0, c0, c0, c1, c1);
					cf_draw_quad_fill3(i1, i2, b, a, c1, c1, c0, c0);
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
						cf_draw_quad_fill2(a, b, i3, i1, c0);
						cf_draw_quad_fill2(i0, i2, b, a, c0);
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
						cf_draw_quad_fill2(a, b, i3, i1, c0);
						cf_draw_quad_fill2(i0, i2, b, a, c0);
						cf_draw_quad_fill3(i1, i3, f3, f1, c0, c0, c1, c1);
						cf_draw_quad_fill3(i0, f0, f2, i2, c0, c1, c1, c0);
						s_bevel_arc_feather(b, i3, f3, i4, f4, c0, c1, bevel_count);
					}
					f1 = f4;
					f0 = f2;
				} else if (emit) {
					cf_draw_quad_fill3(a, b, i3, i1, c0, c0, c1, c1);
					cf_draw_quad_fill3(i0, i2, b, a, c1, c1, c0, c0);
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
					cf_draw_quad_fill2(a, b, i2, i0, c0);
					cf_draw_quad_fill2(i1, i3, b, a, c0);
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
			cf_draw_quad_fill2(a, b, b + n0, i0, c0);
			cf_draw_quad_fill2(a, i1, b - n0, b, c0);
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

void cf_draw_polyline(CF_V2* points, int count, float thickness, CF_Color color, bool loop, bool antialias, int bevel_count)
{
	CUTE_ASSERT(count >= 3);
	float scale = draw->scale_x;
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

void cf_draw_push_m3x2(CF_M3x2 m)
{
	draw->scale_x = cf_len(m.m.x);
	draw->scale_y = cf_len(m.m.y);
	draw->m3x2s.add(m);
}

CF_M3x2 cf_draw_pop_m3x2()
{
	if (draw->m3x2s.count() > 1) {
		draw->scale_x = cf_len(draw->m3x2s.last().m.x);
		draw->scale_y = cf_len(draw->m3x2s.last().m.y);
		return draw->m3x2s.pop();
	} else {
		return draw->m3x2s.last();
	}
}

CF_M3x2 cf_draw_peek_m3x2()
{
	return draw->m3x2s.last();
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

void cf_render_settings_projection(CF_Matrix4x4 projection)
{
	draw->projection = projection;
}

void cf_render_settings_filter(CF_Filter filter)
{
	// TODO - Invalidate spritebatch textures.
	draw->filter = filter;
}

void cf_render_settings_outlines(bool use_outlines)
{
	float val = use_outlines ? 1.0f : 0;
	draw->outline_use_border = val;
}

void cf_render_settings_outlines_use_corners(bool use_corners)
{
	float val = use_corners ? 1.0f : 0;
	draw->outline_use_corners = val;
}

void cf_render_settings_outlines_color(CF_Color c)
{
	draw->outline_color = c;
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

void cf_render_settings_push_tint(CF_Color c)
{
	draw->tints.add(c);
}

CF_Color cf_render_settings_pop_tint()
{
	if (draw->tints.count() > 1) {
		return draw->tints.pop();
	} else {
		return draw->tints.last();
	}
}

CF_Color cf_render_settings_peek_tint()
{
	return draw->tints.last();
}

void cf_render_to(CF_Canvas canvas, bool clear)
{
	cf_apply_canvas(canvas, clear);
	spritebatch_tick(&draw->sb);
	spritebatch_defrag(&draw->sb);
	spritebatch_flush(&draw->sb);
	draw->verts.clear();
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
