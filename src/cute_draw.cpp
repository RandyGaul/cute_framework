/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_draw.h>
#include <cute_alloc.h>
#include <cute_array.h>
#include <cute_file_system.h>
#include <cute_defer.h>
#include <cute_routine.h>
#include <cute_rnd.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_font_internal.h>

#include <shaders/sprite_shader.h>

struct CF_Draw* draw;

#include <cute/cute_png.h>

#define SPRITEBATCH_IMPLEMENTATION
//#define SPRITEBATCH_LOG CF_DEBUG_PRINTF
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#include <cute/cute_png.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <imgui.h>
#include <imgui_internal.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <algorithm>

#define DEBUG_VERT(v, c) batch_quad(make_aabb(v, 3, 3), c)

// Initial design of this API comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_draw.h

using namespace Cute;

#define VA_TYPE_SPRITE        (0)
#define VA_TYPE_TEXT          (1)
#define VA_TYPE_BOX           (2)
#define VA_TYPE_SEGMENT       (3)
#define VA_TYPE_TRIANGLE      (4)
#define VA_TYPE_TRIANGLE_SDF  (5)

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CF_UNUSED(udata);
	CF_TextureParams params = cf_texture_defaults(w, h);
	params.filter = draw->filter;
	params.initial_data = pixels;
	params.initial_data_size = w * h * sizeof(CF_Pixel);
	CF_Texture texture = cf_make_texture(params);
	return texture.id;
}

void cf_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CF_UNUSED(udata);
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
	CF_UNUSED(udata);
	if (image_id >= CF_ASEPRITE_ID_RANGE_LO && image_id <= CF_ASEPRITE_ID_RANGE_HI) {
		cf_aseprite_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CF_PNG_ID_RANGE_LO && image_id <= CF_PNG_ID_RANGE_HI) {
		cf_png_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CF_FONT_ID_RANGE_LO && image_id <= CF_FONT_ID_RANGE_HI) {
		CF_Pixel* pixels = app->font_pixels.get(image_id);
		CF_MEMCPY(buffer, pixels, bytes_to_fill);
	} else if (image_id >= CF_EASY_ID_RANGE_LO && image_id <= CF_EASY_ID_RANGE_HI) {
		CF_Pixel* pixels = app->easy_sprites.get(image_id).pix;
		CF_MEMCPY(buffer, pixels, bytes_to_fill);
	} else {
		CF_ASSERT(false);
		CF_MEMSET(buffer, 0, sizeof(bytes_to_fill));
	}
}

static CF_INLINE float s_intersect(float a, float b, float u0, float u1, float plane_d)
{
	float da = a - plane_d;
	float db = b - plane_d;
	return u0 + (u1 - u0) * (da / (da - db));
}

static void s_draw_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	CF_UNUSED(udata);
	int vert_count = 0;
	draw->verts.ensure_count(count * 6);
	DrawVertex* verts = draw->verts.data();
	CF_MEMSET(verts, 0, sizeof(DrawVertex) * count * 6);

	for (int i = 0; i < count; ++i) {
		spritebatch_sprite_t* s = sprites + i;
		BatchGeometry geom = s->geom;
		DrawVertex* out = verts + vert_count;

		v2 quad[6] = {
			geom.box[0],
			geom.box[3],
			geom.box[1],
			geom.box[1],
			geom.box[3],
			geom.box[2],
		};

		v2 quadH[6] = {
			geom.boxH[0],
			geom.boxH[3],
			geom.boxH[1],
			geom.boxH[1],
			geom.boxH[3],
			geom.boxH[2],
		};

		switch (s->geom.type) {
		case BATCH_GEOMETRY_TYPE_TRI:
		{
			for (int i = 0; i < 3; ++i) {
				out[i].color = s->geom.color;
				out[i].radius = 0;
				out[i].stroke = 0;
				out[i].type = VA_TYPE_TRIANGLE;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = 255;
				out[i].aa = 0;
				out[i].user_params = geom.user_params;
			}

			out[0].posH = geom.a;
			out[1].posH = geom.b;
			out[2].posH = geom.c;
		
			vert_count += 3;
		}	break;

		case BATCH_GEOMETRY_TYPE_TRI_SDF:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].p = quad[i];
				out[i].posH = quadH[i];
				out[i].a = geom.a;
				out[i].b = geom.b;
				out[i].c = geom.c;
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_TRIANGLE_SDF;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].user_params = geom.user_params;
			}

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_QUAD:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].a = geom.a;
				out[i].b = geom.b;
				out[i].c = geom.c;
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_BOX;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].user_params = geom.user_params;
			}

			out[0].p = quad[0];
			out[1].p = quad[1];
			out[2].p = quad[2];
			out[3].p = quad[3];
			out[4].p = quad[4];
			out[5].p = quad[5];

			out[0].posH = quadH[0];
			out[1].posH = quadH[1];
			out[2].posH = quadH[2];
			out[3].posH = quadH[3];
			out[4].posH = quadH[4];
			out[5].posH = quadH[5];
		
			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_SPRITE:
		{
			bool clipped_away = false;
			if (geom.do_clipping) {
				CF_ASSERT(geom.is_text);

				CF_Aabb bb = make_aabb(geom.c, geom.b);
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

				geom.a = V2(bb.min.x, bb.max.y);
				geom.b = bb.max;
				geom.c = V2(bb.max.x, bb.min.y);
				geom.d = bb.min;
			}

			for (int i = 0; i < 6; ++i) {
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				if (s->geom.is_sprite) {
					out[i].type = VA_TYPE_SPRITE;
				} else if (s->geom.is_text) {
					out[i].type = VA_TYPE_TEXT;
				} else {
					CF_ASSERT(false);
				}
				out[i].color = s->geom.color;
				out[i].user_params = geom.user_params;
			}

			out[0].posH = geom.a;
			out[0].uv.x = s->minx;
			out[0].uv.y = s->maxy;

			out[1].posH = geom.d;
			out[1].uv.x = s->minx;
			out[1].uv.y = s->miny;

			out[2].posH = geom.b;
			out[2].uv.x = s->maxx;
			out[2].uv.y = s->maxy;

			out[3].posH = geom.b;
			out[3].uv.x = s->maxx;
			out[3].uv.y = s->maxy;

			out[4].posH = geom.d;
			out[4].uv.x = s->minx;
			out[4].uv.y = s->miny;

			out[5].posH = geom.c;
			out[5].uv.x = s->maxx;
			out[5].uv.y = s->miny;

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_CIRCLE: // Use the capsule path for circle rendering.
		case BATCH_GEOMETRY_TYPE_CAPSULE:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].p = quad[i];
				out[i].posH = quadH[i];
				out[i].a = geom.a;
				out[i].b = geom.b;
				out[i].c = geom.c;
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_SEGMENT;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].user_params = geom.user_params;
			}

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_SEGMENT:
		{
			for (int i = 0; i < 3; ++i) {
				out[i].a = geom.a;
				out[i].b = geom.b;
				out[i].c = geom.c;
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_SEGMENT;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].user_params = geom.user_params;
			}
			
			out[0].p = geom.box[0];
			out[1].p = geom.box[1];
			out[2].p = geom.box[2];

			out[0].posH = geom.boxH[0];
			out[1].posH = geom.boxH[1];
			out[2].posH = geom.boxH[2];
		
			vert_count += 3;
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
	v2 u_texel_size = cf_v2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
	cf_material_set_uniform_fs(draw->material, "fs_params", "u_texel_size", &u_texel_size, CF_UNIFORM_TYPE_FLOAT2, 1);

	// Apply render state.
	cf_material_set_render_state(draw->material, draw->render_states.last());

	// Kick off a draw call.
	cf_apply_shader(draw->shaders.last(), draw->material);
	cf_draw_elements();
}

//--------------------------------------------------------------------------------------------------
// Hidden API called by CF_App.

static void s_init_sb(int w, int h)
{
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
	config.atlas_height_in_pixels = w;
	config.atlas_width_in_pixels = h;

	if (spritebatch_init(&draw->sb, &config, NULL)) {
		CF_FREE(draw);
		draw = NULL;
		CF_ASSERT(false);
	}
}

void cf_make_draw()
{
	draw = CF_NEW(CF_Draw);

	// Setup a good default camera dimensions size.
	cf_camera_dimensions((float)app->w, (float)app->h);

	// Mesh + vertex attributes.
	draw->mesh = cf_make_mesh(CF_USAGE_TYPE_STREAM, CF_MB * 25, 0, 0);
	CF_VertexAttribute attrs[12] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(DrawVertex, p);
	attrs[1].name = "in_posH";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(DrawVertex, posH);
	attrs[2].name = "in_a";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(DrawVertex, a);
	attrs[3].name = "in_b";
	attrs[3].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[3].offset = CF_OFFSET_OF(DrawVertex, b);
	attrs[4].name = "in_c";
	attrs[4].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[4].offset = CF_OFFSET_OF(DrawVertex, c);
	attrs[5].name = "in_uv";
	attrs[5].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[5].offset = CF_OFFSET_OF(DrawVertex, uv);
	attrs[6].name = "in_col";
	attrs[6].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[6].offset = CF_OFFSET_OF(DrawVertex, color);
	attrs[7].name = "in_radius";
	attrs[7].format = CF_VERTEX_FORMAT_FLOAT;
	attrs[7].offset = CF_OFFSET_OF(DrawVertex, radius);
	attrs[8].name = "in_stroke";
	attrs[8].format = CF_VERTEX_FORMAT_FLOAT;
	attrs[8].offset = CF_OFFSET_OF(DrawVertex, stroke);
	attrs[9].name = "in_aa";
	attrs[9].format = CF_VERTEX_FORMAT_FLOAT;
	attrs[9].offset = CF_OFFSET_OF(DrawVertex, aa);
	attrs[10].name = "in_params";
	attrs[10].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[10].offset = CF_OFFSET_OF(DrawVertex, type);
	attrs[11].name = "in_user_params";
	attrs[11].format = CF_VERTEX_FORMAT_UBYTE4N;
	attrs[11].offset = CF_OFFSET_OF(DrawVertex, user_params);
	cf_mesh_set_attributes(draw->mesh, attrs, CF_ARRAY_SIZE(attrs), sizeof(DrawVertex), 0);

	// Shaders.
	draw->shaders.add(CF_MAKE_SOKOL_SHADER(sprite_shader));

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
	s_init_sb(2048, 2048);
}

void cf_destroy_draw()
{
	spritebatch_term(&draw->sb);
	cf_destroy_mesh(draw->mesh);
	cf_destroy_material(draw->material);
	cf_destroy_shader(draw->shaders[0]);
	draw->~CF_Draw();
	CF_FREE(draw);
}

//--------------------------------------------------------------------------------------------------

void cf_draw_sprite(const CF_Sprite* sprite)
{
	spritebatch_sprite_t s = { };
	if (sprite->animation) {
		s.image_id = sprite->animation->frames[sprite->frame_index].id;
	} else {
		s.image_id = sprite->easy_sprite_id;
	}
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;

	v2 p = cf_add_v2(sprite->transform.p, cf_mul_v2(sprite->local_offset, sprite->scale));

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

		x *= scale.x;
		y *= scale.y;

		float x0 = sprite->transform.r.c * x - sprite->transform.r.s * y;
		float y0 = sprite->transform.r.s * x + sprite->transform.r.c * y;
		x = x0;
		y = y0;

		x += p.x;
		y += p.y;

		quad[j].x = x;
		quad[j].y = y;
	}

	s.geom.a = mul(draw->cam, quad[0]);
	s.geom.b = mul(draw->cam, quad[1]);
	s.geom.c = mul(draw->cam, quad[2]);
	s.geom.d = mul(draw->cam, quad[3]);
	s.geom.is_sprite = true;

	s.geom.color = premultiply(to_pixel(draw->tints.last()));
	s.geom.alpha = sprite->opacity;
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

static void s_draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->cam;
	float aaf = draw->aaf * draw->antialias_scale.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_QUAD;

	v2 u = norm(p1 - p0);
	v2 v = skew(u);
	v2 he = V2(distance(p1, p0), distance(p3, p0)) * 0.5f;
	v2 c = ((p0 + p1) * 0.5f + (p2 + p3) * 0.5f) * 0.5f;
	v2 inflate = V2(stroke+radius+aaf,stroke+radius+aaf);
	p0 = p0 - u * inflate - v * inflate;
	p1 = p1 + u * inflate - v * inflate;
	p2 = p2 + u * inflate + v * inflate;
	p3 = p3 - u * inflate + v * inflate;

	s.geom.box[0] = p0;
	s.geom.box[1] = p1;
	s.geom.box[2] = p2;
	s.geom.box[3] = p3;
	s.geom.boxH[0] = mul(m, p0);
	s.geom.boxH[1] = mul(m, p1);
	s.geom.boxH[2] = mul(m, p2);
	s.geom.boxH[3] = mul(m, p3);
	s.geom.a = c;
	s.geom.b = he;
	s.geom.c = u;

	s.geom.color = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = aaf;
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_quad(CF_Aabb bb, float thickness, float chubbiness)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	s_draw_quad(verts[0], verts[1], verts[2], verts[3], thickness, chubbiness, false);
}

void cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, float chubbiness)
{
	s_draw_quad(p0, p1, p2, p3, thickness, chubbiness, false);
}

void cf_draw_quad_fill(CF_Aabb bb, float chubbiness)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	s_draw_quad(verts[0], verts[1], verts[2], verts[3], 0, chubbiness, true);
}

void cf_draw_quad_fill2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float chubbiness)
{
	s_draw_quad(p0, p1, p2, p3, 0, chubbiness, true);
}

static void s_draw_circle(v2 position, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->cam;
	float aaf = draw->aaf * draw->antialias_scale.last();
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_CIRCLE;

	v2 rr = V2(radius, radius);
	v2 inflate = V2(stroke+aaf, stroke+aaf);
	CF_Aabb bb = make_aabb(position - (rr+inflate), position + (rr+inflate));
	cf_aabb_verts(s.geom.box, bb);
	s.geom.box[0] = s.geom.box[0];
	s.geom.box[1] = s.geom.box[1];
	s.geom.box[2] = s.geom.box[2];
	s.geom.box[3] = s.geom.box[3];
	s.geom.boxH[0] = mul(m, s.geom.box[0]);
	s.geom.boxH[1] = mul(m, s.geom.box[1]);
	s.geom.boxH[2] = mul(m, s.geom.box[2]);
	s.geom.boxH[3] = mul(m, s.geom.box[3]);
	s.geom.a = position;
	s.geom.b = position;
	s.geom.c = position;

	s.geom.color = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = aaf;
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_circle(CF_Circle circle, float thickness)
{
	s_draw_circle(circle.p, thickness, circle.r, false);
}

void cf_draw_circle2(CF_V2 position, float radius, float thickness)
{
	s_draw_circle(position, thickness, radius, false);
}

void cf_draw_circle_fill(CF_Circle circle)
{
	s_draw_circle(circle.p, 0, circle.r, true);
}

void cf_draw_circle_fill2(CF_V2 position, float radius)
{
	s_draw_circle(position, 0, radius, true);
}

void cf_draw_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness)
{
}

static CF_INLINE void s_bounding_box_of_capsule(v2 a, v2 b, float radius, float stroke, v2 out[4])
{
	float aaf = draw->aaf * draw->antialias_scale.last();
	v2 n0 = norm(b - a) * (radius + stroke + aaf);
	v2 n1 = skew(n0);
	out[0] = a - n0 + n1;
	out[1] = a - n0 - n1;
	out[2] = b + n0 - n1;
	out[3] = b + n0 + n1;
}

static void s_draw_capsule(v2 a, v2 b, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_CAPSULE;

	s_bounding_box_of_capsule(a, b, radius, stroke, s.geom.box);
	s.geom.box[0] = s.geom.box[0];
	s.geom.box[1] = s.geom.box[1];
	s.geom.box[2] = s.geom.box[2];
	s.geom.box[3] = s.geom.box[3];
	s.geom.boxH[0] = mul(m, s.geom.box[0]);
	s.geom.boxH[1] = mul(m, s.geom.box[1]);
	s.geom.boxH[2] = mul(m, s.geom.box[2]);
	s.geom.boxH[3] = mul(m, s.geom.box[3]);
	s.geom.a = a;
	s.geom.b = b;
	s.geom.c = a;

	s.geom.color = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = draw->aaf * draw->antialias.last();
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_capsule(CF_Capsule capsule, float thickness)
{
	s_draw_capsule(capsule.a, capsule.b, thickness, capsule.r, false);
}

void cf_draw_capsule2(CF_V2 a, CF_V2 b, float radius, float thickness)
{
	s_draw_capsule(a, b, thickness, radius, false);
}

void cf_draw_capsule_fill(CF_Capsule capsule)
{
	s_draw_capsule(capsule.a, capsule.b, 0, capsule.r, true);
}

void cf_draw_capsule_fill2(CF_V2 a, CF_V2 b, float radius)
{
	s_draw_capsule(a, b, 0, radius, true);
}

void CF_INLINE s_bounding_box_of_triangle(v2 a, v2 b, v2 c, float radius, float stroke, v2* out)
{
	v2 ab = b - a;
	v2 bc = c - b;
	v2 ca = a - c;
	float d0 = dot(ab, ab);
	float d1 = dot(bc, bc);
	float d2 = dot(ca, ca);
	auto build_box = [](float d, v2 a, v2 b, v2 c, float inflate, v2* out) {
		float w = CF_SQRTF(d);
		v2 u = (b - a) / w;
		v2 v = skew(u);
		float h = dot(v, c) - dot(v, a);
		if (h < 0) {
			h = -h;
			v = -v;
		}
		out[0] = a - u * inflate - v * inflate;
		out[1] = b + u * inflate - v * inflate;
		out[2] = b + u * inflate + v * (inflate + h);
		out[3] = a - u * inflate + v * (inflate + h);
	};
	float aaf = draw->aaf * draw->antialias_scale.last();
	if (d0 >= d1 && d0 >= d2) {
		build_box(d0, a, b, c, radius + stroke + aaf, out);
	} else if (d1 >= d0 && d1 >= d2) {
		build_box(d1, b, c, a, radius + stroke + aaf, out);
	} else {
		build_box(d2, c, a, b, radius + stroke + aaf, out);
	}
}

static void s_cf_draw_tri(v2 a, v2 b, v2 c, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;

	if (stroke > 0 || radius > 0 || !fill || draw->antialias.last()) {
		s.geom.type = BATCH_GEOMETRY_TYPE_TRI_SDF;
		s_bounding_box_of_triangle(a, b, c, radius, stroke, s.geom.box);
		s.geom.box[0] = s.geom.box[0];
		s.geom.box[1] = s.geom.box[1];
		s.geom.box[2] = s.geom.box[2];
		s.geom.box[3] = s.geom.box[3];
		s.geom.boxH[0] = mul(m, s.geom.box[0]);
		s.geom.boxH[1] = mul(m, s.geom.box[1]);
		s.geom.boxH[2] = mul(m, s.geom.box[2]);
		s.geom.boxH[3] = mul(m, s.geom.box[3]);
		s.geom.a = a;
		s.geom.b = b;
		s.geom.c = c;
	} else {
		s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
		s.geom.a = mul(m, a);
		s.geom.b = mul(m, b);
		s.geom.c = mul(m, c);
	}

	s.geom.color = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = draw->aaf * draw->antialias.last();
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	spritebatch_push(&draw->sb, s);
}

void cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, float chubbiness)
{
	s_cf_draw_tri(p0, p1, p2, thickness, chubbiness, false);
}

void cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, float chubbiness)
{
	s_cf_draw_tri(p0, p1, p2, 0, chubbiness, true);
}

void cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness)
{
	s_draw_capsule(p0, p1, 0, thickness, true);
}

void cf_draw_polyline(CF_V2* pts, int count, float thickness, bool loop)
{
	float radius = thickness * 0.5f;

	if (count <= 0) {
		return;
	} else if (count == 1) {
		cf_draw_circle_fill2(pts[0], radius);
	} else if (count == 2) {
		cf_draw_capsule_fill2(pts[0], pts[1], radius);
	}

	// Each portion of the polyline will be rendered with a single triangle per spritebatch entry.
	CF_M3x2 m = draw->cam;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.geom.color = premultiply(to_pixel(cf_overlay_color(draw->colors.last(), draw->tints.last())));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = 0;
	s.geom.fill = true;
	s.geom.aa = draw->aaf * draw->antialias.last();
	s.geom.type = BATCH_GEOMETRY_TYPE_SEGMENT;
	s.geom.user_params = draw->user_params.last();
	s.sort_bits = draw->layers.last();
	s.w = s.h = 1;

	// Expand to account for aa.
	radius += draw->aaf;

	int i2 = 2;
	v2 p0 = pts[0];
	v2 p1 = pts[1];
	v2 p2 = pts[i2];
	v2 n0 = norm(p1 - p0);
	v2 n1 = norm(p2 - p1);
	v2 t0 = skew(n0);
	v2 t1 = skew(n1);
	v2 a = p0 - n0 * radius + t0 * radius;
	v2 b = p0 - n0 * radius - t0 * radius;

	bool debug = false;
	bool skip = false;
	int iters = count - 2;
	if (loop) {
		skip = true;
		iters += 3;
	}

	auto submit = [&](v2 a, v2 b, v2 c, bool solo = false) {
		if (skip) return;
		if (debug) {
			draw_push_antialias(false);
			draw_tri(a, b, c);
			draw_tri_fill(a, b, c);
			draw_pop_antialias();
		} else {
			s.geom.a = p0;
			s.geom.b = p1;
			s.geom.c = solo ? p0 : p2;
			s.geom.box[0] = a;
			s.geom.box[1] = b;
			s.geom.box[2] = c;
			s.geom.boxH[0] = mul(m, a);
			s.geom.boxH[1] = mul(m, b);
			s.geom.boxH[2] = mul(m, c);
			spritebatch_push(&draw->sb, s);
		}
	};

#define DBG_POINT(P, C) \
	draw_push_color(color_##C()); \
	draw_quad(make_aabb(P, 2.0f, 2.0f)); \
	draw_text(#P, P + V2(0, 5)); \
	draw_pop_color()

#define DBG_PLANE(H, P, C, L) \
	draw_push_color(color_##C()); \
	draw_arrow(project(H, P), (P) + H.n * L * 15.0f, L, L * 3.0f); \
	draw_text(#H, (P) + H.n * L * 15.0f); \
	draw_pop_color()

	for (int i = 0; i < iters; ++i) {
		n0 = norm(p1 - p0);
		n1 = norm(p2 - p1);
		t0 = skew(n0);
		t1 = skew(n1);

		float p0p1_x_p1p2 = cross(n0, n1);
		float d = dot(n0, n1);
		Halfspace h0 = plane(t0, a);
		Halfspace h1 = plane(-t0, b);
		Halfspace h2 = plane(-t1, p2 - t1 * radius);
		Halfspace h3 = plane(t1, p2 + t1 * radius);
		v2 n = norm(n0 - n1);
		Halfspace h4 = plane(n, p1 + n * radius);
		Halfspace x = plane(n1, p2);
		Halfspace y = plane(-n0, p0);

		if (p0p1_x_p1p2 < 0) {
			if (d < 0) {
				// Acute.
				v2 c = intersect(h1, h2);
				if (dot(c, x.n) - radius > dot(p2, x.n) || dot(c, y.n) - radius > dot(p0, y.n)) {
					// Self-intersecting.
					v2 e = p1 + h1.n * radius + n0 * radius;
					v2 d = p1 + h0.n * radius + n0 * radius;
					submit(a, b, e, true);
					submit(d, a, e, true);
					a = p1 + h3.n * radius - n1 * radius;
					b = p1 + h2.n * radius - n1 * radius;
				} else {
					v2 d = intersect(h3, h4);
					v2 e = intersect(h0, h4);
					submit(a, b, e);
					submit(e, b, c);
					submit(e, c, d);
					a = d;
					b = c;
				}
			} else {
				// Obtuse.
				Halfspace hn = plane(norm(n0 + n1), p1);
				v2 c = intersect(hn, h0);
				v2 d = intersect(hn, h1);
				submit(a, b, c);
				submit(c, b, d);
				a = c;
				b = d;
			}
		} else {
			if (d < 0) {
				// Acute.
				v2 e = intersect(h0, h3);
				if (dot(e, x.n) - radius > dot(p2, x.n) || dot(e, y.n) - radius > dot(p0, y.n)) {
					// Self-intersecting.
					v2 c = p1 + h1.n * radius + n0 * radius;
					v2 d = p1 + h0.n * radius + n0 * radius;
					submit(a, b, c, true);
					submit(d, a, c, true);
					a = p1 + h3.n * radius - n1 * radius;
					b = p1 + h2.n * radius - n1 * radius;
				} else {
					v2 c = intersect(h1, h4);
					v2 d = intersect(h4, h2);
					submit(a, b, c);
					submit(c, e, a);
					submit(d, e, c);
					a = e;
					b = d;
				}
			} else {
				// Obtuse.
				Halfspace hn = plane(norm(n0 + n1), p1);
				v2 c = intersect(hn, h0);
				v2 d = intersect(hn, h1);
				submit(a, b, c);
				submit(c, b, d);
				a = c;
				b = d;
			}
		}

		p0 = p1;
		p1 = p2;
		i2 = i2 + 1 == count ? 0 : i2 + 1;
		p2 = pts[i2];

		skip = false;
	}

	if (!loop) {
		v2 d = p1 + n1 * radius + t1 * radius;
		v2 c = p1 + n1 * radius - t1 * radius;
		p2 = p1;
		submit(a, b, c);
		submit(c, a, d);
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
	cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, false);
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
	cf_draw_polyline(draw->temp.data(), draw->temp.count(), thickness, false);
}

void cf_draw_arrow(CF_V2 a, CF_V2 b, float thickness, float arrow_width)
{
	v2 n = norm(b - a);
	v2 t = skew(n) * arrow_width;
	n = n * arrow_width;
	cf_draw_capsule_fill2(a, b - n, thickness * 0.5f);
	cf_draw_tri_fill(b, b - n + t, b - n - t, 0);
}

CF_Result cf_make_font_from_memory(void* data, int size, const char* font_name)
{
	font_name = sintern(font_name);
	CF_Font* font = (CF_Font*)CF_NEW(CF_Font);
	font->file_data = (uint8_t*)data;
	if (!stbtt_InitFont(&font->info, font->file_data, stbtt_GetFontOffsetForIndex(font->file_data, 0))) {
		CF_FREE(data);
		CF_FREE(font);
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
	return cf_make_font_from_memory(data, (int)size, font_name);
}

void cf_destroy_font(const char* font_name)
{
	font_name = sintern(font_name);
	CF_Font* font = app->fonts.get(font_name);
	if (!font) return;
	app->fonts.remove(font_name);
	CF_FREE(font->file_data);
	for (int i = 0; i < font->image_ids.count(); ++i) {
		uint64_t image_id = font->image_ids[i];
		CF_Pixel* pixels = app->font_pixels.get(image_id);
		if (pixels) {
			CF_FREE(pixels);
			app->font_pixels.remove(image_id);
		}
	}
	font->~CF_Font();
	CF_FREE(font);
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
	CF_ASSERT(font_name);
	return app->fonts.get(sintern(font_name));
}

CF_INLINE uint64_t cf_glyph_key(int cp, float font_size, int blur)
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
	img.pix = (cp_pixel_t*)CF_ALLOC(sizeof(cp_pixel_t) * w * h);
	for (int i = 0; i < w * h; ++i) {
		cp_pixel_t pix;
		pix.r = pix.g = pix.b = pixels[i];
		pix.a = 255;
		img.pix[i] = pix;
	}
	cp_save_png(path, &img);
	CF_FREE(img.pix);
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
	uint8_t* pixels_1bpp = (uint8_t*)CF_CALLOC(w * h);
	CF_DEFER(CF_FREE(pixels_1bpp));
	stbtt_MakeGlyphBitmap(&font->info, pixels_1bpp + pad * w + pad, w - pad*2, h - pad*2, w, scale, scale, glyph->index);
	//s_save("glyph.png", pixels_1bpp, w, h);

	// Apply blur.
	if (blur) s_blur(pixels_1bpp, w, h, w, blur);
	//s_save("glyph_blur.png", pixels_1bpp, w, h);

	// Convert to premultiplied RGBA8 pixel format.
	CF_Pixel* pixels = (CF_Pixel*)CF_ALLOC(w * h * sizeof(CF_Pixel));
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

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render = true);

float cf_text_width(const char* text, int text_length)
{
	return s_draw_text(text, V2(0,0), text_length, false).x;
}

float cf_text_height(const char* text, int text_length)
{
	float h = s_draw_text(text, V2(0,0), text_length, false).y;
	if (h < 0) h = -h;
	return h;
}

CF_V2 cf_text_size(const char* text, int num_chars_to_draw)
{
	v2 result = s_draw_text(text, V2(0,0), num_chars_to_draw, false);
	float h = result.y;
	if (h < 0) h = -h;
	result.y = h;
	return result;
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
	CF_RndState rnd = rnd_seed(seed);
	v2 offset = V2(rnd_range(rnd, -x, y), rnd_range(rnd, -x, y));
	effect->q0 += offset;
	effect->q1 += offset;
	return true;
}

static bool s_text_fx_fade(TextEffect* effect)
{
	double speed = effect->get_number("speed", 2);
	double span = effect->get_number("span", 5);
	effect->opacity = CF_COSF((float)(effect->elapsed * speed + effect->index_into_effect / span)) * 0.5f + 0.5f;
	return true;
}

static bool s_text_fx_wave(TextEffect* effect)
{
	double speed = effect->get_number("speed", 5);
	double span = effect->get_number("span", 10);
	double height = effect->get_number("height", 5);
	float offset = (CF_COSF((float)(effect->elapsed * speed + effect->index_into_effect / span)) * 0.5f + 0.5f) * (float)height;
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
	s->end = text + CF_STRLEN(text);
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

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render)
{
	CF_Font* font = cf_font_get(draw->fonts.last());
	CF_ASSERT(font);
	if (!font) return V2(0,0);

	// Cache effect state key'd by input text pointer.
	CF_TextEffectState* effect_state = app->text_effect_states.try_find(text);
	if (!effect_state) {
		effect_state = app->text_effect_states.insert(text);
		effect_state->hash = fnv1a(text, (int)CF_STRLEN(text) + 1);
		s_parse_codes(effect_state, text);
	} else {
		uint64_t h = fnv1a(text, (int)CF_STRLEN(text) + 1);
		if (effect_state->hash != h) {
			// Contents have changed, re-parse the whole thing.
			app->text_effect_states.remove(text);
			effect_state = app->text_effect_states.insert(text);
			effect_state->hash = h;
			s_parse_codes(effect_state, text);
		}
	}
	if (render) {
		effect_state->alive = true;
		effect_state->elapsed += CF_DELTA_TIME;
	}

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
	
	if (text_length < 0) {
		text_length = INT_MAX;
	}

	// Render the string glyph-by-glyph.
	while (*text) {
		cp_prev = cp;
		const char* prev_text = text;
		if (render) effect_spawn();
		text = cf_decode_UTF8(text, &cp);
		++index;
		CF_DEFER(effect_cleanup());

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
		float xadvance = glyph->xadvance;
		if (render) {
			bool visible = glyph->visible;
			s.image_id = glyph->image_id;
			s.w = glyph->w;
			s.h = glyph->h;
			s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;
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
				s.geom.a = mul(draw->cam, V2(q0.x, q1.y));
				s.geom.b = mul(draw->cam, V2(q1.x, q1.y));
				s.geom.c = mul(draw->cam, V2(q1.x, q0.y));
				s.geom.d = mul(draw->cam, V2(q0.x, q0.y));
				s.geom.color = premultiply(to_pixel(color));
				s.geom.clip = make_aabb(mul(draw->cam, clip.min), mul(draw->cam, clip.max));
				s.geom.do_clipping = do_clipping;
				s.geom.is_text = true;
				s.sort_bits = draw->layers.last();

				spritebatch_push(&draw->sb, s);
			}
		}

		advance_to_next_glyph(xadvance);
	}

	if (render) {
		// Draw strike-lines just after the text.
		for (int i = 0; i < draw->strikes.size(); ++i) {
			v2 p0 = draw->strikes[i].p0;
			v2 p1 = draw->strikes[i].p1;
			float thickness = draw->strikes[i].thickness;
			cf_draw_line(p0, p1, thickness);
		}
		draw->strikes.clear();
	}

	return V2(w, h);
}

void cf_draw_text(const char* text, CF_V2 position, int text_length)
{
	s_draw_text(text, position, text_length);
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

void cf_draw_push_antialias_scale(float scale)
{
	draw->antialias_scale.add(scale);
}

float cf_draw_pop_antialias_scale()
{
	if (draw->antialias_scale.count() > 1) {
		return draw->antialias_scale.pop();
	} else {
		return draw->antialias_scale.last();
	}
}

float cf_draw_peek_antialias_scale()
{
	return draw->antialias_scale.last();
}

void cf_draw_push_vertex_attributes(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	draw->user_params.add(cf_make_pixel_rgba(r, g, b, a));
}

void cf_draw_push_vertex_attributes2(CF_Pixel attributes)
{
	draw->user_params.add(attributes);
}

CF_Pixel cf_draw_pop_vertex_attributes()
{
	return draw->user_params.count() > 1 ? draw->user_params.pop() : draw->user_params.last();
}

CF_Pixel cf_draw_peek_vertex_attributes()
{
	return draw->user_params.last();
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

void cf_render_settings_set_atlas_dimensions(int width_in_pixels, int height_in_pixels)
{
	spritebatch_term(&draw->sb);
	s_init_sb(width_in_pixels, height_in_pixels);
	draw->atlas_dims.x = (float)width_in_pixels;
	draw->atlas_dims.y = (float)height_in_pixels;
	draw->texel_dims.x = 1.0f / draw->atlas_dims.x;
	draw->texel_dims.y = 1.0f / draw->atlas_dims.y;
}

void cf_render_settings_push_shader(CF_Shader shader)
{
	draw->shaders.add(shader);
}

CF_Shader cf_render_settings_pop_shader()
{
	return draw->shaders.count() > 1 ? draw->shaders.pop() : draw->shaders.last();
}

CF_Shader cf_render_settings_peek_shader()
{
	return draw->shaders.last();
}

void cf_render_settings_push_texture(const char* name, CF_Texture texture)
{
	material_set_texture_fs(draw->material, name, texture);
}

void cf_render_settings_push_uniform(const char* name, void* data, CF_UniformType type, int array_length)
{
	material_set_uniform_fs(draw->material, "shader_uniforms", name, data, type, array_length);
}

void cf_render_settings_push_uniform_int(const char* name, int val)
{
	material_set_uniform_fs(draw->material, "shader_uniforms", name, &val, CF_UNIFORM_TYPE_INT, 1);
}

void cf_render_settings_push_uniform_float(const char* name, float val)
{
	material_set_uniform_fs(draw->material, "shader_uniforms", name, &val, CF_UNIFORM_TYPE_FLOAT, 1);
}

void cf_render_settings_push_uniform_v2(const char* name, v2 val)
{
	material_set_uniform_fs(draw->material, "shader_uniforms", name, &val, CF_UNIFORM_TYPE_FLOAT2, 1);
}

void cf_render_settings_push_uniform_color(const char* name, CF_Color val)
{
	material_set_uniform_fs(draw->material, "shader_uniforms", name, &val, CF_UNIFORM_TYPE_FLOAT4, 1);
}

void cf_render_to(CF_Canvas canvas, bool clear)
{
	cf_apply_canvas(canvas, clear);
	spritebatch_flush(&draw->sb);
	draw->verts.clear();
}

void cf_camera_dimensions(float w, float h)
{
	draw->cam_dimensions = V2(w, h) * 0.5f;
	draw->aaf = draw->cam_dimensions.y / (float)app->h;
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
	CF_Cam cam;
	cam.m = draw->cam;
	cam.pos = draw->cam_position;
	cam.scale = draw->cam_dimensions;
	cam.angle = draw->cam_rotation;
	cam.aaf = draw->aaf;
	draw->cam_stack.add(cam);
}

void cf_camera_pop()
{
	if (draw->cam_stack.size()) {
		CF_Cam cam = draw->cam_stack.pop();
		draw->cam = cam.m;
		draw->cam_position = cam.pos;
		draw->cam_dimensions = cam.scale;
		draw->cam_rotation = cam.angle;
		draw->aaf = cam.aaf;
	} else {
		draw->cam_dimensions = V2((float)app->w, (float)app->h) * 0.5f;
		draw->cam_position = V2(0, 0);
		draw->cam_rotation = 0;
		draw->aaf = draw->cam_dimensions.y / (float)app->h;
		draw->cam = cf_invert(cf_make_transform_TSR(draw->cam_position, draw->cam_dimensions, draw->cam_rotation));
	}
}

CF_V2 cf_camera_peek_position()
{
	return draw->cam_position;
}

CF_V2 cf_camera_peek_dimensions()
{
	return draw->cam_dimensions * 2.0f;
}

float cf_camera_peek_rotation()
{
	return draw->cam_rotation;
}

CF_M3x2 cf_camera_peek()
{
	return draw->cam;
}

CF_TemporaryImage cf_fetch_image(const CF_Sprite* sprite)
{
	uint64_t image_id;
	if (sprite->animation)
	{
		image_id = sprite->animation->frames[sprite->frame_index].id;
	}
	else
	{
		image_id = sprite->easy_sprite_id;
	}
	spritebatch_sprite_t s = spritebatch_fetch(&draw->sb, image_id, sprite->w, sprite->h);
	CF_TemporaryImage image;
	image.tex = { s.texture_id };
	image.w = sprite->w;
	image.h = sprite->h;
	image.u = cf_v2(s.minx, s.miny);
	image.v = cf_v2(s.maxx, s.maxy);
	return image;
}
