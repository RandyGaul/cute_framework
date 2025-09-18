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
#include <internal/cute_graphics_internal.h>

struct CF_Draw* draw;

//#define SPRITEBATCH_LOG printf
#define SPRITEBATCH_IMPLEMENTATION
#include <cute/cute_spritebatch.h>

#define CUTE_PNG_IMPLEMENTATION
#define CUTE_PNG_ASSERT CF_ASSERT
#define CUTE_PNG_ALLOC cf_alloc
#define CUTE_PNG_FREE cf_free
#define CUTE_PNG_CALLOC cf_calloc
#define CUTE_PNG_REALLOC cf_realloc
#include <cute/cute_png.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert CF_ASSERT
#include <stb/stb_truetype.h>

#define IM_ASSERT CF_ASSERT
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>

// Initial design of this API comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_draw.h

using namespace Cute;

#define VA_TYPE_SPRITE        (0)
#define VA_TYPE_TEXT          (1)
#define VA_TYPE_BOX           (2)
#define VA_TYPE_SEGMENT       (3)
#define VA_TYPE_TRIANGLE      (4)
#define VA_TYPE_TRIANGLE_SDF  (5)
#define VA_TYPE_POLYGON       (6)

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CF_UNUSED(udata);
	CF_TextureParams params = cf_texture_defaults(w, h);
	params.filter = CF_FILTER_LINEAR;
	CF_Texture texture = cf_make_texture(params);
	cf_texture_update(texture, pixels, w * h * sizeof(CF_Pixel));
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
		CF_Pixel** pixels = app->font_pixels.try_get(image_id);
		if (pixels) {
			CF_MEMCPY(buffer, *pixels, bytes_to_fill);
		} else {
			CF_MEMSET(buffer, 0, bytes_to_fill);
		}
	} else if (image_id >= CF_EASY_ID_RANGE_LO && image_id <= CF_EASY_ID_RANGE_HI) {
		CF_Image* img = app->easy_sprites.try_get(image_id);
		if (img) {
			CF_MEMCPY(buffer, img->pix, bytes_to_fill);
		} else {
			CF_MEMSET(buffer, 0, bytes_to_fill);
		}
	} else if (image_id >= CF_PREMADE_ID_RANGE_LO && image_id <= CF_PREMADE_ID_RANGE_HI) {
		// These are handled externally by the user, so spritebatch should never ask for pixels.
		// It's assumed premade atlases are generated properly externally.
		CF_ASSERT(!"This should never be hit -- Invalid image_id sent to spritebatch.");
		CF_MEMSET(buffer, 0, sizeof(bytes_to_fill));
	} else {
		CF_ASSERT(!"Invalid image_id when attempting to fetch pixels.");
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
	CF_Vertex* verts = draw->verts.data();
	CF_MEMSET(verts, 0, sizeof(CF_Vertex) * count * 6);

	for (int i = 0; i < count; ++i) {
		spritebatch_sprite_t* s = sprites + i;
		BatchGeometry geom = s->geom;
		CF_Vertex* out = verts + vert_count;

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

		switch (geom.type) {
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
				out[i].attributes = geom.user_params;
			}

			out[0].posH = geom.shape[0];
			out[1].posH = geom.shape[1];
			out[2].posH = geom.shape[2];
		
			vert_count += 3;
		}	break;

		case BATCH_GEOMETRY_TYPE_TRI_SDF:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].p = quad[i];
				out[i].posH = quadH[i];
				out[i].shape[0] = geom.shape[0];
				out[i].shape[1] = geom.shape[1];
				out[i].shape[2] = geom.shape[2];
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_TRIANGLE_SDF;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].attributes = geom.user_params;
			}

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_QUAD:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].shape[0] = geom.shape[0];
				out[i].shape[1] = geom.shape[1];
				out[i].shape[2] = geom.shape[2];
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_BOX;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].attributes = geom.user_params;
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
				out[i].attributes = geom.user_params;
			}

			out[0].posH = geom.shape[0];
			out[0].uv.x = s->minx;
			out[0].uv.y = s->maxy;

			out[1].posH = geom.shape[3];
			out[1].uv.x = s->minx;
			out[1].uv.y = s->miny;

			out[2].posH = geom.shape[1];
			out[2].uv.x = s->maxx;
			out[2].uv.y = s->maxy;

			out[3].posH = geom.shape[1];
			out[3].uv.x = s->maxx;
			out[3].uv.y = s->maxy;

			out[4].posH = geom.shape[3];
			out[4].uv.x = s->minx;
			out[4].uv.y = s->miny;

			out[5].posH = geom.shape[2];
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
				out[i].shape[0] = geom.shape[0];
				out[i].shape[1] = geom.shape[1];
				out[i].shape[2] = geom.shape[2];
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_SEGMENT;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].attributes = geom.user_params;
			}

			vert_count += 6;
		}	break;

		case BATCH_GEOMETRY_TYPE_SEGMENT:
		{
			for (int i = 0; i < 3; ++i) {
				out[i].shape[0] = geom.shape[0];
				out[i].shape[1] = geom.shape[1];
				out[i].shape[2] = geom.shape[2];
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].stroke = s->geom.stroke;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_SEGMENT;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = s->geom.fill ? 255 : 0;
				out[i].attributes = geom.user_params;
			}
			
			out[0].p = geom.box[0];
			out[1].p = geom.box[1];
			out[2].p = geom.box[2];

			out[0].posH = geom.boxH[0];
			out[1].posH = geom.boxH[1];
			out[2].posH = geom.boxH[2];
		
			vert_count += 3;
		}	break;

		case BATCH_GEOMETRY_TYPE_POLYGON:
		{
			for (int i = 0; i < 6; ++i) {
				out[i].p = quad[i];
				out[i].posH = quadH[i];
				out[i].n = geom.n;
				for (int j = 0; j < geom.n; ++j) {
					out[i].shape[j] = geom.shape[j];
				}
				out[i].color = s->geom.color;
				out[i].radius = s->geom.radius;
				out[i].aa = s->geom.aa;
				out[i].type = VA_TYPE_POLYGON;
				out[i].alpha = (uint8_t)(s->geom.alpha * 255.0f);
				out[i].fill = 255;
				out[i].attributes = geom.user_params;
			}

			vert_count += 6;
		}	break;
		}
	}

	// Allow users to optionally modulate vertices.
	if (draw->vertex_fn) {
		draw->vertex_fn(verts, vert_count);
	}

	CF_Command& cmd = draw->cmds[draw->cmd_index];

	// Map the vertex buffer with sprite vertex data.
	cf_mesh_update_vertex_data(draw->mesh, verts, vert_count);
	cf_apply_mesh(draw->mesh);

	// Apply the atlas texture.
	CF_Texture atlas = { sprites->texture_id };
	cf_material_set_texture_fs(draw->material, "u_image", atlas);

	// Apply uniforms.
	v2 u_texture_size = cf_v2((float)texture_w, (float)texture_h);
	cf_material_set_uniform_fs(draw->material, "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	v2 u_texel_size = cf_v2(1.0f / (float)texture_w, 1.0f / (float)texture_h);
	cf_material_set_uniform_fs(draw->material, "u_texel_size", &u_texel_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(draw->material, "u_alpha_discard", &cmd.alpha_discard, CF_UNIFORM_TYPE_FLOAT, 1);

	// Apply render state.
	cf_material_set_render_state(draw->material, cmd.render_state);

	// Kick off a draw call.
	cf_apply_shader(cmd.shader, draw->material);

	// Apply viewport.
	CF_Rect viewport = cmd.viewport;
	if (viewport.w >= 0 && viewport.h >= 0) {
		cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
	}

	// Apply scissor.
	CF_Rect scissor = cmd.scissor;
	if (scissor.w >= 0 && scissor.h >= 0) {
		cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
	}

	cf_draw_elements();
	cf_commit();

	draw->has_drawn_something = true;
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
	draw->atlas_dims = V2((float)w, (float)h);

	if (spritebatch_init(&draw->sb, &config, NULL)) {
		CF_FREE(draw);
		draw = NULL;
		CF_ASSERT(false);
	}
}

void CF_Draw::reset_cam()
{
	cam_stack.clear();
	cam_stack.add(cf_make_identity());
	mvp = projection;
	draw->antialias_scale.set_count(1);
	draw->set_aaf();
}

// Sets the anti-alias factor, the width of roughly one pixel scaled.
// This factor remains constant-size despite zooming in/out with the camera.
void CF_Draw::set_aaf()
{
	float on_or_off = draw->antialias.last() ? 1.0f : 0.0f;
	float inv_cam_scale = 1.0f / len(draw->cam_stack.last().m.y);
	float scale = draw->antialias_scale.last();
	aaf = scale * inv_cam_scale * on_or_off;
}

void cf_make_draw()
{
	draw = CF_NEW(CF_Draw);
	draw->projection = ortho_2d(0, 0, (float)app->w, (float)app->h);
	draw->reset_cam();
	draw->uniform_arena = cf_make_arena(32, CF_MB);

	// Mesh + vertex attributes.
	Array<CF_VertexAttribute> attrs;
	attrs.add({
		.name = "in_pos",
		.format = CF_VERTEX_FORMAT_FLOAT2,
		.offset = CF_OFFSET_OF(CF_Vertex, p)
	});

	attrs.add({
		.name = "in_posH",
		.format = CF_VERTEX_FORMAT_FLOAT2,
		.offset = CF_OFFSET_OF(CF_Vertex, posH),
	});

	attrs.add({
		.name = "in_n",
		.format = CF_VERTEX_FORMAT_INT,
		.offset = CF_OFFSET_OF(CF_Vertex, n),
	});

	attrs.add({
		.name = "in_ab",
		.format = CF_VERTEX_FORMAT_FLOAT4,
		.offset = CF_OFFSET_OF(CF_Vertex, shape[0]),
	});

	attrs.add({
		.name = "in_cd",
		.format = CF_VERTEX_FORMAT_FLOAT4,
		.offset = CF_OFFSET_OF(CF_Vertex, shape[2]),
	});

	attrs.add({
		.name = "in_ef",
		.format = CF_VERTEX_FORMAT_FLOAT4,
		.offset = CF_OFFSET_OF(CF_Vertex, shape[4]),
	});

	attrs.add({
		.name = "in_gh",
		.format = CF_VERTEX_FORMAT_FLOAT4,
		.offset = CF_OFFSET_OF(CF_Vertex, shape[6]),
	});

	attrs.add({
		.name = "in_uv",
		.format = CF_VERTEX_FORMAT_FLOAT2,
		.offset = CF_OFFSET_OF(CF_Vertex, uv),
	});

	attrs.add({
		.name = "in_col",
		.format = CF_VERTEX_FORMAT_UBYTE4_NORM,
		.offset = CF_OFFSET_OF(CF_Vertex, color),
	});

	attrs.add({
		.name = "in_radius",
		.format = CF_VERTEX_FORMAT_FLOAT,
		.offset = CF_OFFSET_OF(CF_Vertex, radius),
	});

	attrs.add({
		.name = "in_stroke",
		.format = CF_VERTEX_FORMAT_FLOAT,
		.offset = CF_OFFSET_OF(CF_Vertex, stroke),
	});

	attrs.add({
		.name = "in_aa",
		.format = CF_VERTEX_FORMAT_FLOAT,
		.offset = CF_OFFSET_OF(CF_Vertex, aa),
	});

	attrs.add({
		.name = "in_params",
		.format = CF_VERTEX_FORMAT_UBYTE4_NORM,
		.offset = CF_OFFSET_OF(CF_Vertex, type),
	});

	attrs.add({
		.name = "in_user_params",
		.format = CF_VERTEX_FORMAT_FLOAT4,
		.offset = CF_OFFSET_OF(CF_Vertex, attributes),
	});
	draw->mesh = cf_make_mesh(CF_MB * 5, attrs.data(), attrs.count(), sizeof(CF_Vertex));

	// Shaders.
	draw->shaders.add(app->draw_shader);

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

	// Create an initial draw command.
	draw->add_cmd();
}

void cf_destroy_draw()
{
	if (draw->blit_init) {
		cf_destroy_mesh(draw->blit_mesh);
	}
	spritebatch_term(&draw->sb);
	cf_destroy_mesh(draw->mesh);
	cf_destroy_material(draw->material);
	draw->~CF_Draw();
	CF_FREE(draw);
}

//--------------------------------------------------------------------------------------------------

void cf_draw_sprite(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	spritebatch_sprite_t s = { };
	bool apply_border_scale = true;
	if (sprite->animation) {
		s.image_id = sprite->animation->frames[sprite->frame_index].id;
	} else if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		CF_AtlasSubImage sub_image = draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
		s.minx = sub_image.minx;
		s.maxx = sub_image.maxx;
		s.miny = sub_image.miny;
		s.maxy = sub_image.maxy;
		s.image_id = sprite->easy_sprite_id;
		s.texture_id = sub_image.image_id; // @JANK - Hijacked to store texture_id and avoid an extra hashtable lookup.
		apply_border_scale = false;
	} else {
		s.image_id = sprite->easy_sprite_id;
	}
	s.w = sprite->w;
	s.h = sprite->h;
	s.geom.type = BATCH_GEOMETRY_TYPE_SPRITE;

	v2 offset = sprite->offset + (sprite->pivots ? sprite->pivots[sprite->frame_index] : V2(0,0));
	v2 p = cf_add_v2(sprite->transform.p, cf_mul_v2(offset, sprite->scale));

	v2 scale = V2(sprite->scale.x * s.w, sprite->scale.y * s.h);
	if (apply_border_scale) {
		// Expand sprite's scale to account for border pixels in the atlas.
		scale.x = scale.x + (scale.x / (float)sprite->w) * 2.0f;
		scale.y = scale.y + (scale.y / (float)sprite->h) * 2.0f;
	}

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

	CF_M3x2 m = draw->mvp;
	s.geom.shape[0] = mul(m, quad[0]);
	s.geom.shape[1] = mul(m, quad[1]);
	s.geom.shape[2] = mul(m, quad[2]);
	s.geom.shape[3] = mul(m, quad[3]);
	s.geom.is_sprite = true;
	s.geom.color = premultiply(pixel_white());
	s.geom.alpha = sprite->opacity;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

void cf_draw_prefetch(const CF_Sprite* sprite)
{
	if (sprite->animation) {
		for (int i = 0; i < hsize(sprite->animations); ++i) {
			const CF_Animation* animation = sprite->animations[i];
			for (int j = 0; j < asize(animation->frames); ++j) {
				CF_Frame* frame = animation->frames + j;
				spritebatch_prefetch(&draw->sb, frame->id, sprite->w, sprite->h);
			}
		}
	} else if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		spritebatch_prefetch(&draw->sb, sprite->easy_sprite_id, sprite->w, sprite->h);
	} else {
		spritebatch_prefetch(&draw->sb, sprite->easy_sprite_id, sprite->w, sprite->h);
	}
}

static void s_draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->mvp;
	float aaf = draw->aaf;
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
	s.geom.shape[0] = c;
	s.geom.shape[1] = he;
	s.geom.shape[2] = u;
	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = aaf;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

void cf_draw_quad(CF_Aabb bb, float thickness, float chubbiness)
{
	CF_V2 verts[4];
	cf_aabb_verts(verts, bb);
	s_draw_quad(verts[0], verts[1], verts[2], verts[3], thickness, chubbiness, false);
}

void cf_draw_box_rounded(CF_Aabb bb, float thickness, float radius)
{
	v2 p = center(bb);
	float x = p.x;
	float y = p.y;
	float hw = (width(bb) - 2*radius) * 0.5f;
	float hh = (height(bb) - 2*radius) * 0.5f;
	bb = make_aabb(V2(x - hw, y - hh), V2(x + hw, y + hh));
	draw_box(bb, thickness, radius);
}

void cf_draw_box_rounded_fill(CF_Aabb bb, float radius)
{
	v2 p = center(bb);
	float x = p.x;
	float y = p.y;
	float hw = (width(bb) - 2*radius) * 0.5f;
	float hh = (height(bb) - 2*radius) * 0.5f;
	bb = make_aabb(V2(x - hw, y - hh), V2(x + hw, y + hh));
	draw_box_fill(bb, radius);
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
	CF_M3x2 m = draw->mvp;
	float aaf = draw->aaf;
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
	s.geom.shape[0] = position;
	s.geom.shape[1] = position;
	s.geom.shape[2] = position;
	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = aaf;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
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

static CF_INLINE void s_bounding_box_of_capsule(v2 a, v2 b, float radius, float stroke, v2 out[4])
{
	float aaf = draw->aaf;
	v2 n0 = norm(b - a) * (radius + stroke + aaf);
	v2 n1 = skew(n0);
	out[0] = a - n0 + n1;
	out[1] = a - n0 - n1;
	out[2] = b + n0 - n1;
	out[3] = b + n0 + n1;
}

static void s_draw_capsule(v2 a, v2 b, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->mvp;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;
	s.geom.type = BATCH_GEOMETRY_TYPE_CAPSULE;

	s_bounding_box_of_capsule(a, b, radius, stroke, s.geom.box);
	s.geom.boxH[0] = mul(m, s.geom.box[0]);
	s.geom.boxH[1] = mul(m, s.geom.box[1]);
	s.geom.boxH[2] = mul(m, s.geom.box[2]);
	s.geom.boxH[3] = mul(m, s.geom.box[3]);
	s.geom.shape[0] = a;
	s.geom.shape[1] = b;
	s.geom.shape[2] = a;
	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = draw->aaf;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
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
	float aaf = draw->aaf;
	if (d0 >= d1 && d0 >= d2) {
		build_box(d0, a, b, c, radius + stroke + aaf, out);
	} else if (d1 >= d0 && d1 >= d2) {
		build_box(d1, b, c, a, radius + stroke + aaf, out);
	} else {
		build_box(d2, c, a, b, radius + stroke + aaf, out);
	}
}

static void s_draw_tri(v2 a, v2 b, v2 c, float stroke, float radius, bool fill)
{
	CF_M3x2 m = draw->mvp;
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
		s.geom.shape[0] = a;
		s.geom.shape[1] = b;
		s.geom.shape[2] = c;
	} else {
		s.geom.type = BATCH_GEOMETRY_TYPE_TRI;
		s.geom.shape[0] = mul(m, a);
		s.geom.shape[1] = mul(m, b);
		s.geom.shape[2] = mul(m, c);
	}

	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = stroke;
	s.geom.fill = fill;
	s.geom.aa = draw->aaf;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

void cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, float chubbiness)
{
	s_draw_tri(p0, p1, p2, thickness, chubbiness, false);
}

void cf_draw_tri_fill(CF_V2 p0, CF_V2 p1, CF_V2 p2, float chubbiness)
{
	s_draw_tri(p0, p1, p2, 0, chubbiness, true);
}

void cf_draw_line(CF_V2 p0, CF_V2 p1, float thickness)
{
	s_draw_capsule(p0, p1, 0, thickness * 0.5f, true);
}

void cf_draw_polyline(const CF_V2* pts, int count, float thickness, bool loop)
{
	float radius = thickness * 0.5f;

	if (count <= 0) {
		return;
	} else if (count == 1) {
		cf_draw_circle_fill2(pts[0], thickness);
	} else if (count == 2) {
		cf_draw_line(pts[0], pts[1], thickness);
	}

	// Each portion of the polyline will be rendered with a single triangle per spritebatch entry.
	CF_M3x2 m = draw->mvp;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = radius;
	s.geom.stroke = 0;
	s.geom.fill = true;
	s.geom.aa = draw->aaf;
	s.geom.type = BATCH_GEOMETRY_TYPE_SEGMENT;
	s.geom.user_params = draw->user_params.last();
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
			s.geom.shape[0] = p0;
			s.geom.shape[1] = p1;
			s.geom.shape[2] = solo ? p0 : p2;
			s.geom.box[0] = a;
			s.geom.box[1] = b;
			s.geom.box[2] = c;
			s.geom.boxH[0] = mul(m, a);
			s.geom.boxH[1] = mul(m, b);
			s.geom.boxH[2] = mul(m, c);
			DRAW_PUSH_ITEM(s);
		}
	};

#define DBG_CF_V2(P, C) \
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
		CF_Halfspace h0 = plane(t0, a);
		CF_Halfspace h1 = plane(-t0, b);
		CF_Halfspace h2 = plane(-t1, p2 - t1 * radius);
		CF_Halfspace h3 = plane(t1, p2 + t1 * radius);
		v2 n = norm(n0 - n1);
		CF_Halfspace h4 = plane(n, p1 + n * radius);
		CF_Halfspace x = plane(n1, p2);
		CF_Halfspace y = plane(-n0, p0);

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
				CF_Halfspace hn = plane(norm(n0 + n1), p1);
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
				CF_Halfspace hn = plane(norm(n0 + n1), p1);
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

void cf_draw_polygon_fill(const CF_V2* points, int count, float chubbiness)
{
	CF_ASSERT(count >= 3 && count <= 8);
	CF_M3x2 m = draw->mvp;
	spritebatch_sprite_t s = { };
	s.image_id = app->default_image_id;
	s.w = s.h = 1;

	s.geom.type = BATCH_GEOMETRY_TYPE_POLYGON;
	CF_Aabb bb = expand(make_aabb(points, count), draw->aaf+chubbiness);
	CF_V2 box[4];
	aabb_verts(box, bb);
	s.geom.box[0] = box[0];
	s.geom.box[1] = box[1];
	s.geom.box[2] = box[2];
	s.geom.box[3] = box[3];
	s.geom.boxH[0] = mul(m, s.geom.box[0]);
	s.geom.boxH[1] = mul(m, s.geom.box[1]);
	s.geom.boxH[2] = mul(m, s.geom.box[2]);
	s.geom.boxH[3] = mul(m, s.geom.box[3]);
	s.geom.n = count;
	for (int i = 0; i < count; ++i) {
		s.geom.shape[i] = points[i];
	}

	s.geom.color = premultiply(to_pixel(draw->colors.last()));
	s.geom.alpha = 1.0f;
	s.geom.radius = chubbiness;
	s.geom.aa = draw->aaf;
	s.geom.user_params = draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

// Calculates the signed area of an oriented triangle.
// ...Implemented as a macro to force-inline for slightly better debug perf.
#define SIGNED_AREA_2D(A, B, C) \
	(((B).x - (A).x) * ((C).y - (A).y) - ((B).y - (A).y) * ((C).x - (A).x))

// Returns true if a point is within an oriented triangle.
// ...Implemented as a macro to force-inline for slightly better debug perf.
#define IS_PT_IN_TRIANGLE(A, B, C, P) \
	(SIGNED_AREA_2D(A, B, P) > 0 && \
	 SIGNED_AREA_2D(B, C, P) > 0 && \
	 SIGNED_AREA_2D(C, A, P) > 0)

bool is_ear(const v2* polygon, int i, int n)
{
	int prev = i - 1 < 0 ? n - 1 : i - 1;
	int next = i + 1 == n ? 0 : i + 1;

	if (SIGNED_AREA_2D(polygon[prev], polygon[i], polygon[next]) <= 0) {
		return false; // Not convex.
	}

	// Check if any other vertex is inside this triangle.
	for (int j = 0; j < n; j++) {
		if (j == prev || j == i || j == next) {
			continue;
		}
		if (IS_PT_IN_TRIANGLE(polygon[prev], polygon[i], polygon[next], polygon[j])) {
			// Another vertex is inside, not an ear.
			return false;
		}
	}

	return true;
}

// Converts a polygon into renderable triangles.
// ...Uses a simple ear-clipping routine.
// ...Will produce incorrect results for: complex polygons (self-intersecting), duplicate/repeat verts,
//    non-CCW ordering of inputs.
v2* triangulate(v2* polygon, int n, int* out_count)
{
	CF_ASSERT(out_count);
	if (n < 3) {
		*out_count = 0;
		return NULL;
	}

	int max_triangles = n - 2;
	v2* triangles = (v2*)cf_alloc(max_triangles * 3 * sizeof(v2));
	int count = 0;

	int remaining = n;
	while (remaining > 2) {
		bool ear_found = false;
		for (int i = 0; i < remaining; i++) {
			if (is_ear(polygon, i, remaining)) {
				int prev = i - 1 < 0 ? remaining - 1 : i - 1;
				int next = i + 1 == remaining ? 0 : i + 1;
				triangles[count] = polygon[prev];
				triangles[count+1] = polygon[i];
				triangles[count+2] = polygon[next];
				count += 3;

				// Remove the ear vertex by shifting the array.
				for (int j = i; j < remaining - 1; j++) {
					polygon[j] = polygon[j + 1];
				}
				remaining--;
				ear_found = true;
				break;
			}
		}

		if (!ear_found) {
			// If we can't find an ear, the polygon might be invalid (e.g. self-intersecting).
			cf_free(triangles);
			*out_count = 0;
			return NULL;
		}
	}

	*out_count = count;
	return triangles;
}

void cf_draw_polygon_fill_simple(const CF_V2* points, int count)
{
	v2* points_copy = (v2*)cf_alloc(sizeof(v2) * count);
	CF_MEMCPY(points_copy, points, sizeof(v2) * count);

	int n = 0;
	v2* triangles = triangulate(points_copy, count, &n);
	for (int i = 0; i < n; i += 3) {
		v2 a = triangles[i];
		v2 b = triangles[i+1];
		v2 c = triangles[i+2];
		s_draw_tri(a, b, c, 0, 0, true);
	}

	cf_free(triangles);
	cf_free(points_copy);
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
	cf_draw_line(a, b - n, thickness);
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

// From fontstash.h, memononen
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

// Unused, was for debugging atlases and rasterized text glyphs at one point.
#if 0
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
#endif

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

CF_Glyph* cf_font_get_glyph(CF_Font* font, int code, float font_size, int blur)
{
	uint64_t glyph_key = cf_glyph_key(code, font_size, blur);
	CF_Glyph* glyph = font->glyphs.try_get(glyph_key);
	if (!glyph) {
		int glyph_index = stbtt_FindGlyphIndex(&font->info, code);
		if (!glyph_index) {
			// This code doesn't exist in this font.
			// Try and use a backup glyph instead.
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

float cf_font_get_kern(CF_Font* font, float font_size, int code0, int code1)
{
	uint64_t key = CF_KERN_KEY(code0, code1);
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

void cf_push_text_id(uint64_t id)
{
	draw->text_ids.add(id);
}

uint64_t cf_pop_text_id()
{
	if (draw->text_ids.count() > 1) {
		return draw->text_ids.pop();
	} else {
		return draw->text_ids.last();
	}
}

uint64_t cf_peek_text_id()
{
	return draw->text_ids.last();
}

void cf_push_text_effect_active(bool text_effects_on)
{
	draw->text_effects.add(text_effects_on);
}

bool cf_pop_text_effect_active()
{
	if (draw->text_effects.count() > 1) {
		return draw->text_effects.pop();
	} else {
		return draw->text_effects.last();
	}
}

bool cf_peek_text_effect_active()
{
	return draw->text_effects.last();
}

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render = true, cf_text_markup_info_fn* markups = NULL);

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
	CF_ParsedTextState* text_state;
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
	int hex = 0;
	if (!string.empty()) {
		hex = (int)string.to_hex();
		if (digits == 6) {
			// Treat the color as opaque if only 3 bytes were found.
			hex = hex << 8 | 0xFF;
		}
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
	double result = 0;
	if (is_float) {
		result = string.to_double();
	} else {
		if (!string.empty()) {
			result = (double)string.to_int();
		}
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
		val.u.string = !string.empty() ? sintern(string.c_str()) : NULL;
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
		bool success = s->text_state->parse_finish(code.effect_name, code.index_in_string);
		CF_UNUSED(success);
	} else {
		s->text_state->parse_add(code);
	}
}

static bool s_text_fx_color(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	CF_Color c = fx->get_color("color");
	fx->color = c;
	return true;
}

static bool s_text_fx_shake(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	double freq = fx->get_number("freq", 35);
	int seed = (int)(fx->elapsed * freq);
	float x = (float)fx->get_number("x", 2);
	float y = (float)fx->get_number("y", 2);
	CF_Rnd rnd = rnd_seed(seed);
	v2 offset = V2(rnd_range(rnd, -x, x), rnd_range(rnd, -y, y));
	fx->q0 += offset;
	fx->q1 += offset;
	return true;
}

static bool s_text_fx_fade(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	double speed = fx->get_number("speed", 2);
	double span = fx->get_number("span", 5);
	fx->opacity = CF_COSF((float)(fx->elapsed * speed + fx->index_into_effect / span)) * 0.5f + 0.5f;
	return true;
}

static bool s_text_fx_wave(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	double speed = fx->get_number("speed", 5);
	double span = fx->get_number("span", 10);
	double height = fx->get_number("height", 5);
	float offset = (CF_COSF((float)(fx->elapsed * speed + fx->index_into_effect / span)) * 0.5f + 0.5f) * (float)height;
	fx->q0.y += offset;
	fx->q1.y += offset;
	return true;
}

static bool s_text_fx_strike(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	if (!s_is_space(fx->character) || fx->character == ' ') {
		v2 hw = V2((float)fx->xadvance, 0) * 0.5f;
		float h = fx->font_size / 20.0f;
		h = (float)fx->get_number("strike", (double)h);
		CF_Strike strike;
		strike.p0 = fx->center - hw;
		strike.p1 = fx->center + hw;
		strike.thickness = h;
		draw->strikes.add(strike);
	}
	return true;
}

static void s_parse_codes(CF_ParsedTextState* text_state, const char* text)
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
	s->text_state = text_state;
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
	std::sort(text_state->codes.begin(), text_state->codes.end(),
		[](const CF_TextCode& a, const CF_TextCode&b) {
			return a.index_in_string < b.index_in_string;
		}
	);
	text_state->sanitized = s->sanitized;
}

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render, cf_text_markup_info_fn* markups)
{
	CF_Font* font = cf_font_get(draw->fonts.last());
	CF_ASSERT(font);
	if (!font) return V2(0,0);

	// Text id can be custom or based on string pointer
	uint64_t text_id = draw->text_ids.last();
	if (text_id == 0) { text_id = (uint64_t)text; }

	// Effect state is key'd by text id
	CF_TextEffectState* effect_state = app->text_effect_states.try_find(text_id);
	if (!effect_state) {
		effect_state = app->text_effect_states.insert(text_id);
	}

	// Text state is key'd by text content and pointer
	CF_ParsedTextState* text_state = app->parsed_text_states.try_find(text);
	if (!text_state) {
		text_state = app->parsed_text_states.insert(text);
		text_state->hash = fnv1a(text, (int)CF_STRLEN(text) + 1);
		s_parse_codes(text_state, text);
	} else {
		uint64_t h = fnv1a(text, (int)CF_STRLEN(text) + 1);
		if (text_state->hash != h) {
			// Contents have changed, re-parse the whole thing.
			app->parsed_text_states.remove(text);
			text_state = app->parsed_text_states.insert(text);
			text_state->hash = h;
			s_parse_codes(text_state, text);
		}
	}
	if (render || markups) {
		effect_state->alive = true;
		effect_state->elapsed += CF_DELTA_TIME;
		text_state->alive = true;
	}

	// Use the sanitized string for rendering. This excludes all text codes.
	bool do_effects = draw->text_effects.last();
	if (do_effects) {
		text = text_state->sanitized.c_str();
	}

	// Gather up all state required for rendering.
	float font_size = draw->font_sizes.last();
	int blur = draw->blurs.last();
	float wrap_w = draw->text_wrap_widths.last();
	float scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
	float line_height = font->line_height * scale;
	int cp_prev = 0;
	int cp = 0;
	const char* end_of_line = NULL;
	float h = (font->ascent + font->descent) * scale;
	float w = font->width * scale;

	// @NOTE -- Not 100% sure snapping to pixel is the best thing here, but it really does make
	// text rendering feel a lot more robust, especially for nearest-neighbor rendering.
	float inv_cam_scale_y = 1.0f / len(draw->cam_stack.last().m.y);
	float inv_cam_scale_x = 1.0f / len(draw->cam_stack.last().m.x);
	float x = roundf(position.x * inv_cam_scale_x);
	float initial_y = roundf((position.y - font->ascent * scale) * inv_cam_scale_y);
	float y = initial_y;
	float max_x = x;
	// Extend the height by descent to include spaces below the baseline.
	// e.g: Characters such as "g", "y"...
	float min_y = y + font->descent * scale;

	int index = 0;
	int code_index = 0;
	int newline_count = 0;

	// Called whenever text-effects need to be spawned, before going to the next glyph.
	auto effect_spawn = [&]() {
		if (code_index < text_state->codes.count()) {
			CF_TextCode* code = text_state->codes + code_index;
			if (index == code->index_in_string) {
				++code_index;
				TextEffect effect = { };
				effect.effect_name = code->effect_name;
				effect.initial_index = effect.index_into_string = code->index_in_string;
				effect.index_into_effect = 0;
				effect.glyph_count = code->glyph_count;
				effect.elapsed = effect_state->elapsed;
				effect.params = &code->params;
				effect.fn = code->fn;
				text_state->effects.add(effect);
			}
		}
	};

	// Called whenever text-effects need to be cleaned up, when going to the next glyph.
	auto effect_cleanup = [&]() {
		for (int i = 0; i < text_state->effects.count();) {
			TextEffect* effect = text_state->effects + i;
			if (effect->index_into_string + effect->glyph_count == index) {
				effect->index_into_effect = index - effect->index_into_string - 1;
				effect->on_end = true;
				if (effect->fn) effect->fn(effect); // Signal we're done (one past the end).
				if (markups) {
					effect->bounds.add(effect->line_bound);
					CF_MarkupInfo info;
					info.effect_name = effect->effect_name;
					info.start_glyph_index = effect->initial_index;
					info.glyph_count = effect->glyph_count;
					info.bounds_count = effect->bounds.count();
					info.bounds = effect->bounds.data();
					markups(text, info, (CF_TextEffect*)effect);
				}
				text_state->effects.unordered_remove(i);
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

	auto advance_to_next_glyph = [&](CF_Glyph* last_glyph) {
		// Max bound covers the entire glyph without kerning so we use w instead
		// of xadvance
		max_x = max(max_x, x + last_glyph->w);
		if (vertical) {
			min_y = min(min_y, y + font->descent * scale);

			y -= line_height;
		} else {
			x += last_glyph->xadvance;
		}
	};

	bool hit_newline = false;
	auto apply_newline = [&]() {
		if (vertical) {
			x += w;
			y = initial_y;

			max_x = max(max_x, x);
		} else {
			x = position.x;
			y -= line_height;

			min_y = min(min_y, y + font->descent * scale);
		}
		hit_newline = true;
		++newline_count;
	};
	
	if (text_length < 0) {
		text_length = INT_MAX;
	}
	if (!text) {
		text_length = 0;
	}

	// Render the string glyph-by-glyph.
	while (text_length-- && *text) {
		cp_prev = cp;
		const char* prev_text = text;
		if ((render || markups) && do_effects) effect_spawn();
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
		if (render || markups) {
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
			for (int i = 0; i < text_state->effects.count();) {
				TextEffect* effect = text_state->effects + i;
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
					effect->on_begin = effect->on_start();
					keep_going = fn(effect);
					q0 = effect->q0;
					q1 = effect->q1;
					color = effect->color;
					s.geom.alpha = effect->opacity;
					xadvance = effect->xadvance;
					visible = effect->visible;

					// Track bounds while rendering.
					if (markups) {
						if (!effect->line_bound_init) {
							effect->line_bound = make_aabb(q0, q1);
							effect->line_bound_init = true;
						} else {
							if (hit_newline) {
								effect->bounds.add(effect->line_bound);
								effect->line_bound = make_aabb(q0, q1);
							} else {
								effect->line_bound = combine(effect->line_bound, make_aabb(q0, q1));
							}
						}
					}

					if (!keep_going) {
						text_state->effects.unordered_remove(i);
					}
				}
				if (keep_going) {
					++i;
				}
			}

			// Actually render the sprite.
			if (visible && render) {
				CF_M3x2 m = draw->mvp;
				s.geom.shape[0] = mul(m, V2(q0.x, q1.y));
				s.geom.shape[1] = mul(m, V2(q1.x, q1.y));
				s.geom.shape[2] = mul(m, V2(q1.x, q0.y));
				s.geom.shape[3] = mul(m, V2(q0.x, q0.y));
				s.geom.color = premultiply(to_pixel(color));
				s.geom.is_text = true;
				DRAW_PUSH_ITEM(s);
			}
		}

		advance_to_next_glyph(glyph);
		hit_newline = false;
	}

	if (render) {
		// Draw strike-lines just after the text.
		for (int i = 0; i < draw->strikes.size(); ++i) {
			v2 p0 = draw->strikes[i].p0;
			v2 p1 = draw->strikes[i].p1;
			float thickness = draw->strikes[i].thickness;
			cf_draw_line(p0, p1, thickness);
		}
	}
	draw->strikes.clear();

	return V2(max_x - position.x, position.y - min_y);
}

void cf_draw_text(const char* text, CF_V2 position, int text_length)
{
	s_draw_text(text, position, text_length);
}

void cf_text_effect_register(const char* name, CF_TextEffectFn* fn)
{
	app->text_effect_fns.insert(sintern(name), fn);
}

double cf_text_effect_get_number(const CF_TextEffect* fx, const char* key, double default_val)
{
	return ((TextEffect*)fx)->get_number(key, default_val);
}

CF_Color cf_text_effect_get_color(const CF_TextEffect* fx, const char* key, CF_Color default_val)
{
	return ((TextEffect*)fx)->get_color(key, default_val);
}

const char* cf_text_effect_get_string(const CF_TextEffect* fx, const char* key, const char* default_val)
{
	return ((TextEffect*)fx)->get_string(key, default_val);
}

void cf_text_get_markup_info(cf_text_markup_info_fn* fn, const char* text, CF_V2 position, int num_chars_to_draw)
{
	s_draw_text(text, position, num_chars_to_draw, false, fn);
}

void cf_draw_push_layer(int layer)
{
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(layer);
}

int cf_draw_pop_layer()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(layer);
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

void cf_draw_push_antialias(bool antialias)
{
	draw->antialias.add(antialias);
	draw->set_aaf();
}

bool cf_draw_pop_antialias()
{
	if (draw->antialias.count() > 1) {
		bool result = draw->antialias.pop();
		draw->set_aaf();
		return result;
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
	draw->set_aaf();
}

float cf_draw_pop_antialias_scale()
{
	if (draw->antialias_scale.count() > 1) {
		float scale = draw->antialias_scale.pop();
		draw->set_aaf();
		return scale;
	} else {
		return draw->antialias_scale.last();
	}
}

float cf_draw_peek_antialias_scale()
{
	return draw->antialias_scale.last();
}

void cf_draw_push_vertex_attributes(float r, float g, float b, float a)
{
	draw->user_params.add(cf_make_color_rgba_f(r, g, b, a));
}

void cf_draw_push_vertex_attributes2(CF_Color attributes)
{
	draw->user_params.add(attributes);
}

CF_Color cf_draw_pop_vertex_attributes()
{
	return draw->user_params.count() > 1 ? draw->user_params.pop() : draw->user_params.last();
}

CF_Color cf_draw_peek_vertex_attributes()
{
	return draw->user_params.last();
}

void cf_set_vertex_callback(CF_VertexFn* vertex_fn)
{
	draw->vertex_fn = vertex_fn;
}

void cf_draw_push_viewport(CF_Rect viewport)
{
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(viewport);
}

CF_Rect cf_draw_pop_viewport()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(viewport);
}

CF_Rect cf_draw_peek_viewport()
{
	return draw->viewports.last();
}

void cf_draw_push_scissor(CF_Rect scissor)
{
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(scissor);
}

CF_Rect cf_draw_pop_scissor()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(scissor);
}

CF_Rect cf_draw_peek_scissor()
{
	return draw->scissors.last();
}

void cf_draw_push_render_state(CF_RenderState render_state)
{
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(render_state);
}

CF_RenderState cf_draw_pop_render_state()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(render_state);
}

CF_RenderState cf_draw_peek_render_state()
{
	return draw->render_states.last();
}

void cf_draw_set_atlas_dimensions(int width_in_pixels, int height_in_pixels)
{
	spritebatch_term(&draw->sb);
	s_init_sb(width_in_pixels, height_in_pixels);
	draw->atlas_dims.x = (float)width_in_pixels;
	draw->atlas_dims.y = (float)height_in_pixels;
	draw->texel_dims.x = 1.0f / draw->atlas_dims.x;
	draw->texel_dims.y = 1.0f / draw->atlas_dims.y;
}

CF_Shader cf_make_draw_shader(const char* path)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_internal(path);
	CF_Shader draw_shd = cf_make_draw_shader_internal(path);
	draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	return draw_shd;
}

CF_Shader cf_make_draw_shader_from_source(const char* src)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_from_source_internal(src);
	CF_Shader draw_shd = cf_make_draw_shader_from_source_internal(src);
	draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	return draw_shd;
}

CF_Shader cf_make_draw_shader_from_bytecode(CF_DrawShaderBytecode bytecode)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_from_bytecode_internal(bytecode.blit_shader);
	CF_Shader draw_shd = cf_make_draw_shader_from_bytecode_internal(bytecode.draw_shader);
	draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	return draw_shd;
}

void cf_draw_push_shader(CF_Shader shader)
{
	CF_ASSERT(shader.id);
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(shader);
}

CF_Shader cf_draw_pop_shader()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(shader);
}

CF_Shader cf_draw_peek_shader()
{
	return draw->shaders.last();
}

// In cute_graphics.cpp.
void cf_material_set_uniform_fs_internal(CF_Material material_handle, const char* block_name, const char* name, void* data, CF_UniformType type, int array_length);

void cf_draw_push_alpha_discard(bool true_enable_alpha_discard)
{
	float alpha_discard = true_enable_alpha_discard ? 1.0f : 0.0f;
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(alpha_discard);
}

static float s_pop_alpha_discard()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(alpha_discard);
}

bool cf_draw_pop_alpha_discard()
{
	float alpha_discard = s_pop_alpha_discard();
	return alpha_discard == 0 ? false : true;
}

bool cf_draw_peek_alpha_discard()
{
	return draw->alpha_discards.last() == 0 ? false : true;
}

void cf_draw_set_texture(const char* name, CF_Texture texture)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.texture = texture;
	u.is_texture = true;
	ADD_UNIFORM(u);
}

void cf_draw_set_uniform(const char* name, void* data, CF_UniformType type, int array_length)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.type = type;
	u.array_length = array_length;
	u.size = s_uniform_size(type) * array_length;
	u.data = cf_arena_alloc(&draw->uniform_arena, u.size);
	CF_MEMCPY(u.data, data, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_set_uniform_int(const char* name, int val)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.type = CF_UNIFORM_TYPE_INT;
	u.array_length = 1;
	u.size = s_uniform_size(CF_UNIFORM_TYPE_INT);
	u.data = cf_arena_alloc(&draw->uniform_arena, u.size);
	CF_ASSERT(u.size == sizeof(val));
	CF_MEMCPY(u.data, &val, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_set_uniform_float(const char* name, float val)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.type = CF_UNIFORM_TYPE_FLOAT;
	u.array_length = 1;
	u.size = s_uniform_size(CF_UNIFORM_TYPE_FLOAT);
	u.data = cf_arena_alloc(&draw->uniform_arena, u.size);
	CF_ASSERT(u.size == sizeof(val));
	CF_MEMCPY(u.data, &val, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_set_uniform_v2(const char* name, CF_V2 val)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.type = CF_UNIFORM_TYPE_FLOAT2;
	u.array_length = 1;
	u.size = s_uniform_size(CF_UNIFORM_TYPE_FLOAT2);
	u.data = cf_arena_alloc(&draw->uniform_arena, u.size);
	CF_ASSERT(u.size == sizeof(val));
	CF_MEMCPY(u.data, &val, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_set_uniform_color(const char* name, CF_Color val)
{
	CF_DrawUniform u;
	u.name = sintern(name);
	u.type = CF_UNIFORM_TYPE_FLOAT4;
	u.array_length = 1;
	u.size = s_uniform_size(CF_UNIFORM_TYPE_FLOAT4);
	u.data = cf_arena_alloc(&draw->uniform_arena, u.size);
	CF_ASSERT(u.size == sizeof(val));
	CF_MEMCPY(u.data, &val, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_canvas(CF_Canvas canvas, CF_V2 position, CF_V2 scale)
{
	CF_Command& cmd = draw->add_cmd();
	cmd.is_canvas = true;
	cmd.canvas = canvas;
	CF_Aabb bb = make_aabb(position, fabsf(scale.x), fabsf(scale.y));
	aabb_verts(cmd.canvas_verts, bb);
	bool flip_x = scale.x < 0;
	bool flip_y = scale.y < 0;
	auto swap = [](v2& a, v2& b) {
		v2 t = a;
		a = b;
		b = t;
	};
	if (flip_x) {
		swap(cmd.canvas_verts[0], cmd.canvas_verts[1]);
		swap(cmd.canvas_verts[2], cmd.canvas_verts[3]);
	}
	if (flip_y) {
		swap(cmd.canvas_verts[0], cmd.canvas_verts[3]);
		swap(cmd.canvas_verts[1], cmd.canvas_verts[2]);
	}
	for (int i = 0; i < 4; ++i) {
		cmd.canvas_verts_posH[i] = mul(draw->mvp, cmd.canvas_verts[i]);
	}
	cmd.canvas_attributes = draw->user_params.last();
}

void static s_blit(CF_Command* cmd, CF_Canvas src, CF_Canvas dst, bool clear_dst)
{
	typedef struct Vertex
	{
		v2 pos;  // World space x/y.
		v2 posH; // posH, homogenous (multiplied by mvp).
		v2 uv;   // UV to read from src.
		CF_Color params;
	} Vertex;

	if (!draw->blit_init) {
		draw->blit_init = true;

		// Create a full-screen quad mesh.
		CF_VertexAttribute attrs[4] = { 0 };
		attrs[0].name = "in_pos";
		attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
		attrs[1].name = "in_posH";
		attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[1].offset = CF_OFFSET_OF(Vertex, posH);
		attrs[2].name = "in_uv";
		attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
		attrs[2].offset = CF_OFFSET_OF(Vertex, uv);
		attrs[3].name = "in_params";
		attrs[3].format = CF_VERTEX_FORMAT_FLOAT4;
		attrs[3].offset = CF_OFFSET_OF(Vertex, params);
		CF_Mesh blit_mesh = cf_make_mesh(sizeof(Vertex) * 1024, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex));
		draw->blit_mesh = blit_mesh;
	}

	// Try and fetch a custom shader supplied by the user, otherwise fallback to the default blit shader.
	CF_Shader* blit = (CF_Shader*)draw->draw_shd_to_blit_shd.try_get(cmd->shader.id);
	if (!blit) {
		CF_ASSERT(app->blit_shader.id);
		blit = (CF_Shader*)&app->blit_shader;
	}

	cf_apply_canvas(dst, clear_dst);

	// Matches index convention from `bb_verts` function.
	v2 verts_world[6] = {
		cmd->canvas_verts[0],
		cmd->canvas_verts[1],
		cmd->canvas_verts[3],
		cmd->canvas_verts[1],
		cmd->canvas_verts[2],
		cmd->canvas_verts[3],
	};
	v2 verts_posH[6] = {
		cmd->canvas_verts_posH[0],
		cmd->canvas_verts_posH[1],
		cmd->canvas_verts_posH[3],
		cmd->canvas_verts_posH[1],
		cmd->canvas_verts_posH[2],
		cmd->canvas_verts_posH[3],
	};
	Vertex verts[6];
	for (int i = 0; i < 6; ++i) {
		verts[i].pos = verts_world[i];
		verts[i].posH = verts_posH[i];
	}
	verts[0].uv = V2(0,1);
	verts[1].uv = V2(1,1);
	verts[2].uv = V2(0,0);
	verts[3].uv = V2(1,1);
	verts[4].uv = V2(1,0);
	verts[5].uv = V2(0,0);

	cf_mesh_update_vertex_data(draw->blit_mesh, verts, 6);
	cf_apply_mesh(draw->blit_mesh);

	// Read pixels from src.
	cf_material_set_texture_fs(draw->material, "u_image", cf_canvas_get_target(src));

	// Apply uniforms.
	CF_CanvasInternal* canvas_internal = (CF_CanvasInternal*)cmd->canvas.id;
	v2 canvas_dims = V2((float)canvas_internal->w, (float)canvas_internal->h);
	cf_material_set_uniform_fs(draw->material, "u_texture_size", &canvas_dims, CF_UNIFORM_TYPE_FLOAT2, 1);
	cf_material_set_uniform_fs(draw->material, "u_alpha_discard", &cmd->alpha_discard, CF_UNIFORM_TYPE_FLOAT, 1);
	
	// Apply render state.
	cf_material_set_render_state(draw->material, cmd->render_state);

	// Apply shader.
	cf_apply_shader(*blit, draw->material);

	// Apply viewport.
	CF_Rect viewport = cmd->viewport;
	if (viewport.w >= 0 && viewport.h >= 0) {
		cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
	}

	// Apply scissor.
	CF_Rect scissor = cmd->scissor;
	if (scissor.w >= 0 && scissor.h >= 0) {
		cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
	}

	// Blit onto dst.
	cf_draw_elements();
}

static void s_process_command(CF_Canvas canvas, CF_Command* cmd, CF_Command* next, bool& clear)
{
	if (cmd->processed) return;
	cmd->processed = true;

	// Apply uniforms.
	CF_DrawUniform* u = &cmd->u;
	if (u->is_texture) {
		material_set_texture_fs(draw->material, u->name, u->texture);
	} else if (u->data) {
		cf_material_set_uniform_fs_internal(draw->material, "shd_uniforms", u->name, u->data, u->type, u->array_length);
	}

	// Blit canvas.
	// ...Incurs an entire extra draw call by itself.
	if (cmd->is_canvas) {
		s_blit(cmd, cmd->canvas, canvas, clear);
		clear = false; // Only clear `canvas` once.
		draw->has_drawn_something = true;
		return;
	}

	// Collate all of the drawable items into the spritebatch.
	if (!cmd->items.count()) return;
	draw->need_flush = true;
	for (int j = 0; j < cmd->items.count(); ++j) {
		spritebatch_push(&draw->sb, cmd->items[j]);
	}

	// Merge with the next command if identical.
	bool same = true;
	if (next) {
		if (next->u.size != cmd->u.size) {
			same = false;
		} else if (next->u.type != cmd->u.type) {
			same = false;
		} else if (next->u.texture.id != cmd->u.texture.id) {
			same = false;
		} else if (next->u.name != cmd->u.name) {
			same = false;
		} else if (CF_MEMCMP(next->u.data, cmd->u.data, next->u.size)) {
			same = false;
		} else if (!(
			next->alpha_discard == cmd->alpha_discard &&
			next->render_state == cmd->render_state &&
			next->scissor == cmd->scissor &&
			next->shader == cmd->shader &&
			next->viewport == cmd->viewport
		)) {
			same = false;
		}
	} else {
		same = false;
	}

	if (!same) {
		// Process the collated drawable items. Might get split up into multiple draw calls depending on
		// the atlas compiler.
		draw->need_flush = false;
		if (!draw->delay_defrag) {
			spritebatch_defrag(&draw->sb);
		}
		spritebatch_flush(&draw->sb);
	}
}

void cf_render_layers_to(CF_Canvas canvas, int layer_lo, int layer_hi, bool clear)
{
	// We will render to this canvas.
	cf_apply_canvas(canvas, clear);

	// Sort the commands by layer first, then by age (to maintain relative ordering).
	std::stable_sort(draw->cmds.begin(), draw->cmds.end(), [](const CF_Command& a, const CF_Command& b) {
		if (a.layer == b.layer) return a.id < b.id;
		else return a.layer < b.layer;
	});

	// Process each rendering command.
	int count = draw->cmds.count();
	for (int i = 0; i < count; ++i) {
		draw->cmd_index = i;
		CF_Command* cmd = &draw->cmds[i];
		CF_Command* next = i + 1 == count ? NULL : draw->cmds + (i + 1);
		if (cmd->layer >= layer_lo && cmd->layer <= layer_hi) {
			s_process_command(canvas, cmd, next, clear);
		} else if (cmd->layer > layer_hi) {
			break;
		}
	}

	// Reset internal state.
	if (clear && !draw->has_drawn_something) {
		cf_clear_canvas(canvas);
	}
	if (draw->need_flush) {
		draw->need_flush = false;
		if (!draw->delay_defrag) {
			spritebatch_defrag(&draw->sb);
		}
		spritebatch_flush(&draw->sb);
	}
	draw->has_drawn_something = false;
	cf_arena_reset(&draw->uniform_arena);
	draw->verts.clear();

	// Remove commands that were processed.
	for (int i = 0; i < draw->cmds.size();) {
		if (draw->cmds[i].processed) {
			draw->cmds.unordered_remove(i);
		} else {
			++i;
		}
	}

	// Ensure there's at least one "default" command for convenience use-cases.
	draw->add_cmd();
}

void cf_render_to(CF_Canvas canvas, bool clear)
{
	cf_render_layers_to(canvas, -INT_MAX, INT_MAX, clear);
}

CF_V2 cf_draw_mul(CF_V2 v)
{
	return mul(draw->cam_stack.last(), v);
}

void cf_draw_transform(CF_M3x2 m)
{
	m = mul(draw->cam_stack.last(), m);
	draw->cam_stack.last() = m;
	draw->mvp = mul(draw->projection, m);
	draw->set_aaf();
}

void cf_draw_translate(float x, float y)
{
	CF_M3x2 m = make_translation(x, y);
	cf_draw_transform(m);
}

void cf_draw_translate_v2(CF_V2 position)
{
	cf_draw_translate(position.x, position.y);
}

void cf_draw_scale(float w, float h)
{
	CF_M3x2 m = make_scale(w, h);
	cf_draw_transform(m);
}

void cf_draw_scale_v2(CF_V2 scale)
{
	cf_draw_scale(scale.x, scale.y);
}

void cf_draw_rotate(float radians)
{
	CF_M3x2 m = make_rotation(radians);
	cf_draw_transform(m);
}

void cf_draw_TSR(CF_V2 position, CF_V2 scale, float radians)
{
	cf_draw_transform(make_transform(position, scale, radians));
}

void cf_draw_TSR_absolute(CF_V2 position, CF_V2 scale, float radians)
{
	CF_M3x2 m = make_transform(position, scale, radians);
	draw->cam_stack.last() = m;
	draw->mvp = mul(draw->projection, m);
	draw->set_aaf();
}

void cf_draw_push()
{
	CF_M3x2 m = draw->cam_stack.last();
	draw->cam_stack.add(m);
}

void cf_draw_pop()
{
	if (draw->cam_stack.size() > 1) {
		draw->cam_stack.pop();
	}
	CF_M3x2 m = draw->cam_stack.last();
	draw->mvp = mul(draw->projection, m);
	draw->set_aaf();
}

CF_M3x2 cf_draw_peek()
{
	return draw->cam_stack.last();
}

void cf_draw_projection(CF_M3x2 projection)
{
	draw->projection = projection;
	draw->mvp = mul(projection, draw->cam_stack.last());
}

CF_V2 cf_world_to_screen(CF_V2 CF_V2)
{
	CF_V2 = mul(draw->mvp, CF_V2);
	CF_V2.x = (CF_V2.x + 1.0f) * (float)app->w * 0.5f;
	CF_V2.y = (1.0f - CF_V2.y) * (float)app->h * 0.5f;
	return CF_V2;
}

CF_V2 cf_screen_to_world(CF_V2 CF_V2)
{
	CF_V2.x = (CF_V2.x / (float)app->w) * 2.0f - 1.0f;
	CF_V2.y = -((CF_V2.y / (float)app->h) * 2.0f - 1.0f);
	CF_V2 = mul(invert(draw->mvp), CF_V2);
	return CF_V2;
}

CF_Aabb cf_screen_bounds_to_world()
{
	float w = (float)app->w;
	float h = (float)app->h;
	v2 lo = cf_screen_to_world(V2(0,h));
	v2 hi = cf_screen_to_world(V2(w,0));
	return make_aabb(lo, hi);
}

CF_TemporaryImage cf_fetch_image(const CF_Sprite* sprite)
{
	draw->delay_defrag = true;

	if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		CF_AtlasSubImage sub_image = draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
		spritebatch_sprite_t s = spritebatch_fetch(&draw->sb, sprite->easy_sprite_id, sprite->w, sprite->h);
		CF_TemporaryImage image;
		image.tex = { sub_image.image_id }; // @JANK - Hijacked to store texture_id and avoid an extra hashtable lookup.
		image.w = sub_image.w;
		image.h = sub_image.h;
		image.u = cf_v2(sub_image.minx, sub_image.miny);
		image.v = cf_v2(sub_image.maxx, sub_image.maxy);
		return image;
	} else {
		uint64_t image_id;
		if (sprite->animation) {
			image_id = sprite->animation->frames[sprite->frame_index].id;
		} else {
			image_id = sprite->easy_sprite_id;
		}
		
		spritebatch_sprite_t s = spritebatch_fetch(&draw->sb, image_id, sprite->w, sprite->h);
		CF_TemporaryImage image;
		image.tex = { s.texture_id };
		image.w = sprite->w;
		image.h = sprite->h;
		v2 inv_dims = V2(1.0f / draw->atlas_dims.x, 1.0f / draw->atlas_dims.y);
		s.minx += inv_dims.x;
		s.maxx -= inv_dims.x;
		s.miny -= inv_dims.y;
		s.maxy += inv_dims.y;
		image.u = cf_v2(s.minx, s.miny);
		image.v = cf_v2(s.maxx, s.maxy);
		return image;
	}
}

CF_Texture cf_register_premade_atlas(const char* png_path, int sub_image_count, CF_AtlasSubImage* sub_images)
{
	CF_Image img = { 0 };
	image_load_png(png_path, &img);
	CF_ASSERT(img.pix);
	CF_TextureParams params = cf_texture_defaults(img.w, img.h);
	params.filter = CF_FILTER_LINEAR;
	CF_Texture texture = cf_make_texture(params);
	cf_texture_update(texture, img.pix, img.w * img.h * sizeof(CF_Pixel));
	Array<spritebatch_premade_sprite_t> premades;
	for (int i = 0; i < sub_image_count; ++i) {
		spritebatch_premade_sprite_t s = { 0 };
		s.image_id = sub_images[i].image_id + CF_PREMADE_ID_RANGE_LO;
		sub_images[i].image_id = texture.id; // @JANK - Hijack this to store texture_id, and avoid an extra hashtable lookup later in sprite_push.
		s.w = sub_images[i].w;
		s.h = sub_images[i].h;
		s.minx = sub_images[i].minx;
		s.maxx = sub_images[i].maxx;
		s.miny = sub_images[i].miny;
		s.maxy = sub_images[i].maxy;
		premades.add(s);
		draw->premade_sub_image_id_to_sub_image.add(s.image_id, sub_images[i]);
	}
	spritebatch_register_premade_atlas(&draw->sb, texture.id, img.w, img.h, sub_image_count, premades.data());
	image_free(&img);
	return texture;
}

CF_Sprite cf_make_premade_sprite(uint64_t image_id)
{
	image_id = image_id + CF_PREMADE_ID_RANGE_LO;
	CF_AtlasSubImage sub_image = draw->premade_sub_image_id_to_sub_image.find(image_id);
	CF_Sprite s = cf_sprite_defaults();
	s.name = "premade_sprite";
	s.easy_sprite_id = image_id;
	s.w = sub_image.w;
	s.h = sub_image.h;
	return s;
}