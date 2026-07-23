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
#include <internal/cute_custom_sprite_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_font_internal.h>
#include <internal/cute_graphics_internal.h>

struct CF_Draw* s_draw;
static const char* s_text_without_markups = NULL;

//#define ATLAS_CACHE_LOG printf
// cute_atlas_cache.h's default ATLAS_CACHE_MALLOC/FREE live inside its outer
// include guard, so the header-only include pulled in transitively above
// (via cute_app_internal.h -> cute_draw_internal.h) already defines them to
// malloc/free before we get here. #undef first so our override doesn't just
// silently lose to that earlier default (and to avoid a macro-redefined
// warning).
#undef ATLAS_CACHE_MALLOC
#undef ATLAS_CACHE_FREE
#define ATLAS_CACHE_MALLOC(size, ctx) cf_alloc(size)
#define ATLAS_CACHE_FREE(ptr, ctx) cf_free(ptr)
#define ATLAS_CACHE_IMPLEMENTATION
#include <cute/cute_atlas_cache.h>

#define CUTE_PNG_IMPLEMENTATION
#define CUTE_PNG_ASSERT CF_ASSERT
#define CUTE_PNG_ALLOC cf_alloc
#define CUTE_PNG_FREE cf_free
#define CUTE_PNG_CALLOC cf_calloc
#define CUTE_PNG_REALLOC cf_realloc
#include <cute/cute_png.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert CF_ASSERT
#define STBTT_malloc(x, u) cf_alloc(x)
#define STBTT_free(x, u) cf_free(x)
#include <stb/stb_truetype.h>

#define IM_ASSERT CF_ASSERT
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>

// Initial design of this API comes from Noel Berry's Blah framework here:
// https://github.com/NoelFB/blah/blob/master/include/blah_draw.h

using namespace Cute;

// Draw colors travel to the GPU as packed half4 (two packHalf2x16 words: rg then ba)
// instead of unorm8, so HDR colors (channels > 1.0) survive to the shader intact.
// Round-to-nearest-even float32 -> float16 bit conversion; handles subnormals, overflow
// to inf, and NaN passthrough.
static CF_INLINE uint16_t s_f32_to_f16(float f)
{
	uint32_t x;
	CF_MEMCPY(&x, &f, sizeof(x));
	uint32_t sign = (x >> 16) & 0x8000u;
	x &= 0x7FFFFFFFu;
	if (x >= 0x47800000u) {
		// Overflow to inf; NaN keeps a nonzero mantissa.
		return (uint16_t)(sign | 0x7C00u | (x > 0x7F800000u ? 0x0200u : 0u));
	}
	if (x < 0x38800000u) {
		// Subnormal half (or zero): shift in the implicit leading 1, round to nearest even.
		if (x < 0x33000000u) return (uint16_t)sign; // Rounds to +-0 (below half of the smallest subnormal).
		uint32_t shift = 126u - (x >> 23); // 14 (largest subnormal) .. 24 (smallest).
		uint32_t mant = (x & 0x7FFFFFu) | 0x800000u;
		uint32_t half = mant >> shift;
		uint32_t rem = mant & ((1u << shift) - 1u);
		uint32_t halfway = 1u << (shift - 1u);
		if (rem > halfway || (rem == halfway && (half & 1u))) ++half;
		return (uint16_t)(sign | half);
	}
	// Normal: rebias the exponent, round the mantissa to nearest even (a carry here
	// correctly bumps the exponent, saturating to inf at the top).
	uint32_t half = (x - 0x38000000u) >> 13;
	uint32_t rem = x & 0x1FFFu;
	if (rem > 0x1000u || (rem == 0x1000u && (half & 1u))) ++half;
	return (uint16_t)(sign | half);
}

// Matches GLSL packHalf2x16: first component in the low 16 bits.
static CF_INLINE uint32_t s_pack_half2(float a, float b)
{
	return (uint32_t)s_f32_to_f16(a) | ((uint32_t)s_f32_to_f16(b) << 16);
}

static CF_INLINE uint32_t s_pack_half_rg(CF_Color c) { return s_pack_half2(c.r, c.g); }
static CF_INLINE uint32_t s_pack_half_ba(CF_Color c) { return s_pack_half2(c.b, c.a); }

ATLAS_CACHE_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CF_UNUSED(udata);
	CF_TextureParams params = cf_texture_defaults(w, h);
	params.filter = CF_FILTER_LINEAR;
	CF_Texture texture = cf_make_texture(params);
	cf_texture_update(texture, pixels, w * h * sizeof(CF_Pixel));
	return texture.id;
}

void cf_destroy_texture_handle(ATLAS_CACHE_U64 texture_id, void* udata)
{
	CF_UNUSED(udata);
	CF_Texture tex;
	tex.id = texture_id;
	cf_destroy_texture(tex);
}

atlas_cache_t* cf_get_draw_atlas_cache()
{
	return &s_draw->atlas_cache;
}

void cf_get_pixels(ATLAS_CACHE_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	CF_UNUSED(udata);
	if (image_id >= CF_ASEPRITE_ID_RANGE_LO && image_id <= CF_ASEPRITE_ID_RANGE_HI) {
		cf_aseprite_cache_get_pixels(image_id, buffer, bytes_to_fill);
	} else if (image_id >= CF_CUSTOM_SPRITE_ID_RANGE_LO && image_id <= CF_CUSTOM_SPRITE_ID_RANGE_HI) {
		cf_custom_sprite_get_pixels(image_id, buffer, bytes_to_fill);
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
		// These are handled externally by the user, so atlas_cache should never ask for pixels.
		// It's assumed premade atlases are generated properly externally.
		CF_ASSERT(!"This should never be hit -- Invalid image_id sent to atlas_cache.");
		CF_MEMSET(buffer, 0, bytes_to_fill);
	} else if (image_id >= CF_PATH_ID_RANGE_LO && image_id <= CF_PATH_ID_RANGE_HI) {
		CF_DrawPathData* pd = s_draw->draw_paths.try_get(image_id);
		if (pd) {
			CF_MEMCPY(buffer, pd->pixels, bytes_to_fill);
		} else {
			CF_MEMSET(buffer, 0, bytes_to_fill);
		}
	} else {
		CF_ASSERT(!"Invalid image_id when attempting to fetch pixels.");
		CF_MEMSET(buffer, 0, bytes_to_fill);
	}
}


// Appends a zero-initialized geometry directly into the current command
// (fill-in-place: no stack struct, no copy; unset fields stay zero).
static CF_INLINE BatchGeometry& s_push_geom()
{
	CF_Command& cmd = s_draw->cmds.last();
	BatchGeometry& g = cmd.geoms.add();
	g.mvp = s_draw->mvp;
	g.blend = s_draw->blends.last();
	return g;
}

// SDF shape recorders route through here so an active CSG shape group
// (cf_draw_shape_group_begin) can stage them as operands instead of emitting commands.
static CF_INLINE BatchGeometry& s_push_shape_geom()
{
	if (s_draw->shape_group_active) {
		BatchGeometry& g = s_draw->group_geoms.add();
		CF_MEMSET(&g, 0, sizeof(g));
		g.mvp = s_draw->mvp;
		g.blend = s_draw->blends.last();
		g.csg_operand = true;
		g.csg_op = s_draw->shape_group_op;
		g.csg_k = s_draw->shape_group_k;
		return g;
	}
	return s_push_geom();
}

//--------------------------------------------------------------------------------------------------
// Tiled renderer. See the comment block in cute_draw_internal.h for an overview.

bool cf_tile_range(float min_x, float min_y, float max_x, float max_y, int tiles_x, int tiles_y, int* x0, int* y0, int* x1, int* y1)
{
	int tx0 = (int)floorf(min_x / (float)CF_TILE_PX);
	int ty0 = (int)floorf(min_y / (float)CF_TILE_PX);
	int tx1 = (int)floorf(max_x / (float)CF_TILE_PX);
	int ty1 = (int)floorf(max_y / (float)CF_TILE_PX);
	if (tx1 < 0 || ty1 < 0 || tx0 >= tiles_x || ty0 >= tiles_y || min_x > max_x || min_y > max_y) return false;
	*x0 = tx0 < 0 ? 0 : tx0;
	*y0 = ty0 < 0 ? 0 : ty0;
	*x1 = tx1 >= tiles_x ? tiles_x - 1 : tx1;
	*y1 = ty1 >= tiles_y ? tiles_y - 1 : ty1;
	return true;
}


void cf_draw_set_tiled_enabled(bool enabled) { s_draw->tiled_mode = enabled ? 2 : 1; }
bool cf_draw_get_tiled_enabled() { return s_draw->tiled_mode == 2; }
bool cf_draw_tiled_available() { return s_draw->tiled_available; }
void cf_draw_set_tiled_auto() { s_draw->tiled_mode = 0; }
void cf_draw_set_tiled_list_budget(uint64_t entries) { s_draw->tiled_list_budget = entries; }

// Cheap pre-scan over a batch to drive the auto heuristics: total footprint in tiles
// and whether any command is a big opaque cover (the case where the tiled path's
// opaque-cover cull wins decisively).
struct CF_TiledBatchStats
{
	uint64_t footprint_tiles;
	bool has_big_opaque;
};

static CF_TiledBatchStats s_tiled_batch_stats(const BatchGeometry* geoms, int start, int end)
{
	CF_TiledBatchStats stats = { 0, false };
	int canvas_w, canvas_h;
	cf_current_canvas_size(&canvas_w, &canvas_h);
	if (canvas_w <= 0 || canvas_h <= 0) return stats;
	float w2 = canvas_w * 0.5f;
	float h2 = canvas_h * 0.5f;
	for (int i = start; i < end; ++i) {
		const BatchGeometry& geom = geoms[i];
		if (geom.csg_operand) continue; // Folded into a preceding CSG head.
		const CF_V2* src = geom.box;
		int nverts = 4;
		bool is_sdf = true;
		bool src_world = true;
		switch (geom.type) {
		case BATCH_GEOMETRY_TYPE_SPRITE: src = geom.shape; src_world = true; is_sdf = false; break;
		case BATCH_GEOMETRY_TYPE_TRI:    src = geom.shape; src_world = true; nverts = 3; is_sdf = false; break;
		case BATCH_GEOMETRY_TYPE_GLYPH:  is_sdf = false; break; // Winding coverage, not a cull-capable SDF.
		default: break;
		}
		float min_x = FLT_MAX, min_y = FLT_MAX, max_x = -FLT_MAX, max_y = -FLT_MAX;
		for (int j = 0; j < nverts; ++j) {
			v2 ndc = src[j];
			if (src_world) CF_MUL_M32_V2(ndc, geom.mvp, src[j]);
			float px = (ndc.x + 1.0f) * w2;
			float py = (1.0f - ndc.y) * h2;
			min_x = cf_min(min_x, px);
			min_y = cf_min(min_y, py);
			max_x = cf_max(max_x, px);
			max_y = cf_max(max_y, py);
		}
		min_x = cf_max(min_x, 0.0f);
		min_y = cf_max(min_y, 0.0f);
		max_x = cf_min(max_x, (float)canvas_w);
		max_y = cf_min(max_y, (float)canvas_h);
		if (min_x >= max_x || min_y >= max_y) continue;
		int tw = (int)((max_x - min_x) / CF_TILE_PX) + 1;
		int th = (int)((max_y - min_y) / CF_TILE_PX) + 1;
		uint64_t tiles = (uint64_t)tw * (uint64_t)th;
		stats.footprint_tiles += tiles;
		if (is_sdf && geom.fill && geom.alpha >= 1.0f && geom.color.a >= 1.0f && tiles >= 64) {
			stats.has_big_opaque = true;
		}
	}
	return stats;
}

// (Re)creates a GPU-written storage buffer at least `size` bytes big.
static void s_tile_ensure_rw_buffer(CF_StorageBuffer* buf, int* cap, int size)
{
	if (*cap >= size) return;
	if (buf->id) cf_destroy_storage_buffer(*buf);
	int new_cap = *cap ? *cap : 16 * 1024;
	while (new_cap < size) new_cap *= 2;
	CF_StorageBufferParams params = cf_storage_buffer_defaults(new_cap);
	params.compute_writable = true;
	params.graphics_readable = true;
	*buf = cf_make_storage_buffer(params);
	*cap = new_cap;
}

void cf_draw_tiled_stats(int* tiled_batches, int* instanced_batches, uint64_t* upload_bytes)
{
	if (tiled_batches) *tiled_batches = s_draw->tiled_batch_count;
	if (instanced_batches) *instanced_batches = s_draw->instanced_batch_count;
	if (upload_bytes) *upload_bytes = s_draw->tiled_upload_bytes;
	s_draw->tiled_batch_count = 0;
	s_draw->instanced_batch_count = 0;
	s_draw->tiled_upload_bytes = 0;
}

static bool s_tiled_batch_eligible(int count)
{
	if (!s_draw->tiled_available || s_draw->tiled_mode == 1) return false;
	if (count < s_draw->tiled_threshold) return false;
	CF_Command& cmd = s_draw->cmds[s_draw->cmd_index];
	// Custom draw shaders need a tile-walk variant (not yet implemented) -- mesh path.
	if (cmd.shader.id != app->draw_shader.id) return false;
	// Viewports remap NDC; the tile walk derives NDC from gl_FragCoord -- mesh path.
	if (cmd.viewport.w >= 0 && cmd.viewport.h >= 0) return false;
	// In-register composition is only equivalent to the default premultiplied src-over state.
	if (CF_MEMCMP(&cmd.render_state, &s_draw->default_render_state, sizeof(CF_RenderState)) != 0) return false;
	return true;
}

// Fixed-function canvas state for a blend-mode run. Colors are premultiplied, which
// makes every mode exact: ADD is ONE/ONE, SCREEN is ONE/(1-src.rgb), and MULTIPLY's
// per-primitive form D*(src.rgb + 1 - src.a) comes from DST_COLOR/(1-src.a). The tiled
// walk pre-composites its whole run in-register and outputs the total gain for
// MULTIPLY, so its dst factor is ZERO instead. Alpha: NORMAL accumulates coverage; the
// other modes leave the canvas alpha untouched.
static CF_RenderState s_blend_run_state(CF_RenderState rs, int blend, bool tiled_walk)
{
	switch (blend) {
	case CF_DRAW_BLEND_ADD:
		rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
		break;
	case CF_DRAW_BLEND_MULTIPLY:
		rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_DST_COLOR;
		rs.blend.rgb_dst_blend_factor = tiled_walk ? CF_BLENDFACTOR_ZERO : CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case CF_DRAW_BLEND_SCREEN:
		rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
		break;
	default:
		return rs;
	}
	rs.blend.enabled = true;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ZERO;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	return rs;
}

static void s_draw_report_tiled(const BatchGeometry* geoms, const CF_PendingUV* uvs, int start, int end, uint64_t texture_id, int texture_w, int texture_h, int blend, bool instanced)
{
	CF_Command& cmd = s_draw->cmds[s_draw->cmd_index];
	int canvas_w, canvas_h;
	cf_current_canvas_size(&canvas_w, &canvas_h);
	if (canvas_w <= 0 || canvas_h <= 0) return;
	int tiles_x = (canvas_w + CF_TILE_PX - 1) / CF_TILE_PX;
	int tiles_y = (canvas_h + CF_TILE_PX - 1) / CF_TILE_PX;
	int tile_count = tiles_x * tiles_y;
	float w2 = canvas_w * 0.5f;
	float h2 = canvas_h * 0.5f;
	uint32_t list_capacity = 0; // GPU binning: exact upper bound (sum of AABB tile-rect areas).
	int max_tile_rows = 0;      // GPU binning: dispatch-y extent for count/scatter.

	Array<CF_TileCmd>& cmds = s_draw->tile_cmds;
	Array<CF_TileV4>& pay = s_draw->tile_payload;
	cmds.clear();
	pay.clear();

	CF_M3x2 last_mvp;
	bool have_mvp = false;
	uint32_t inv_off = 0;
	float ux0 = FLT_MAX, uy0 = FLT_MAX, ux1 = -FLT_MAX, uy1 = -FLT_MAX;

	// Walk the geometry stream range in paint order. Sprite/text atlas uvs come from
	// the per-flush uv table the atlas callbacks filled in.
	for (int k = start; k < end; ++k) {
		const BatchGeometry& geom = geoms[k];
		if (geom.csg_operand) continue; // Consumed by its preceding CSG head.
		const CF_PendingUV* s = NULL;
		if (geom.is_sprite || geom.is_text) {
			s = uvs + k;
			if (s->texture_id == 0) continue; // Never reported by the atlas cache.
		}

		// Coverage polygon in pixel space (top-left origin), matching what the mesh
		// path would have rasterized.
		CF_V2 poly[4];
		int nverts = 4;
		bool src_world = true;
		const CF_V2* src = geom.box;
		switch (geom.type) {
		case BATCH_GEOMETRY_TYPE_SPRITE: src = geom.shape; src_world = true; break; // World quad TL,TR,BR,BL.
		case BATCH_GEOMETRY_TYPE_TRI:    src = geom.shape; src_world = true; nverts = 3; break; // World verts.
		default: break;
		}
		float axmin = 0, aymin = 0, axmax = 0, aymax = 0;
		if (!instanced) {
			axmin = FLT_MAX; aymin = FLT_MAX; axmax = -FLT_MAX; aymax = -FLT_MAX;
		}
		for (int j = 0; j < nverts && !instanced; ++j) {
			v2 ndc = src[j];
			if (src_world) CF_MUL_M32_V2(ndc, geom.mvp, src[j]);
			poly[j].x = (ndc.x + 1.0f) * w2;
			poly[j].y = (1.0f - ndc.y) * h2;
			axmin = cf_min(axmin, poly[j].x);
			aymin = cf_min(aymin, poly[j].y);
			axmax = cf_max(axmax, poly[j].x);
			aymax = cf_max(aymax, poly[j].y);
		}

		CF_TileCmd tc;
		CF_MEMSET(&tc, 0, sizeof(tc));
		// Matrix palette entry: forward mvp (vertex-stage quads) + inverse (tile walk
		// world reconstruction), deduped for consecutive items sharing a camera.
		if (!have_mvp || CF_MEMCMP(&geom.mvp, &last_mvp, sizeof(CF_M3x2)) != 0) {
			last_mvp = geom.mvp;
			have_mvp = true;
			CF_M3x2 inv = cf_invert(geom.mvp);
			inv_off = (uint32_t)pay.count();
			pay.add({ geom.mvp.m.x.x, geom.mvp.m.x.y, geom.mvp.m.y.x, geom.mvp.m.y.y });
			pay.add({ geom.mvp.p.x, geom.mvp.p.y, 0, 0 });
			pay.add({ inv.m.x.x, inv.m.x.y, inv.m.y.x, inv.m.y.y });
			pay.add({ inv.p.x, inv.p.y, 0, 0 });
		}
		tc.inv_mvp = inv_off;
		tc.aabb[0] = axmin;
		tc.aabb[1] = aymin;
		tc.aabb[2] = axmax;
		tc.aabb[3] = aymax;
		tc.color = s_pack_half_rg(geom.color);
		tc.color_ba = s_pack_half_ba(geom.color);
		tc.radius = geom.radius;
		tc.stroke = geom.stroke * 0.5f; // Matches the mesh path's in_shape.y * 0.5 in s_draw_vs.
		tc.aa = geom.aa;
		tc.alpha = geom.alpha;
		tc.fill = geom.fill ? 1.0f : 0.0f;
		tc.n = (float)geom.n;
		tc.user[0] = geom.user_params.r;
		tc.user[1] = geom.user_params.g;
		tc.user[2] = geom.user_params.b;
		tc.user[3] = geom.user_params.a;

		bool is_sdf = false;
		switch (geom.type) {
		case BATCH_GEOMETRY_TYPE_SPRITE:
		{
			CF_V2 q0 = geom.shape[0];
			CF_V2 e1 = geom.shape[1] - q0;
			CF_V2 e2 = geom.shape[3] - q0;
			float d1 = dot(e1, e1);
			float d2 = dot(e2, e2);
			if (d1 == 0 || d2 == 0) continue; // Degenerate, invisible.
			tc.type = geom.is_text ? 1u : 0u;
			tc.payload = (uint32_t)pay.count();
			pay.add({ q0.x, q0.y, e1.x, e1.y });
			pay.add({ e2.x, e2.y, 1.0f / d1, 1.0f / d2 });
			pay.add({ s->minx, s->maxy, s->maxx, s->miny });
			if (geom.is_text) {
				// Per-corner colors as half4 pairs: two vec4s, one corner per vec2.
				CF_TileV4 t0, t1;
				*(uint32_t*)&t0.x = s_pack_half_rg(geom.text_colors[0]);
				*(uint32_t*)&t0.y = s_pack_half_ba(geom.text_colors[0]);
				*(uint32_t*)&t0.z = s_pack_half_rg(geom.text_colors[1]);
				*(uint32_t*)&t0.w = s_pack_half_ba(geom.text_colors[1]);
				*(uint32_t*)&t1.x = s_pack_half_rg(geom.text_colors[2]);
				*(uint32_t*)&t1.y = s_pack_half_ba(geom.text_colors[2]);
				*(uint32_t*)&t1.z = s_pack_half_rg(geom.text_colors[3]);
				*(uint32_t*)&t1.w = s_pack_half_ba(geom.text_colors[3]);
				pay.add(t0);
				pay.add(t1);
			}
		}	break;

		case BATCH_GEOMETRY_TYPE_TRI:
		{
			tc.type = 4u;
			tc.fill = 1.0f;
			tc.payload = (uint32_t)pay.count();
			CF_Color c0 = geom.use_tri_colors ? geom.tri_colors[0] : geom.color;
			CF_Color c1 = geom.use_tri_colors ? geom.tri_colors[1] : geom.color;
			CF_Color c2 = geom.use_tri_colors ? geom.tri_colors[2] : geom.color;
			// Half4 corner colors: c0/c1 fill P2, c2 rides P1's two spare lanes (keeps
			// the attribute vec4s at po+3..5, same as before).
			pay.add({ geom.shape[0].x, geom.shape[0].y, geom.shape[1].x, geom.shape[1].y });
			CF_TileV4 p1;
			p1.x = geom.shape[2].x;
			p1.y = geom.shape[2].y;
			*(uint32_t*)&p1.z = s_pack_half_rg(c2);
			*(uint32_t*)&p1.w = s_pack_half_ba(c2);
			pay.add(p1);
			CF_TileV4 tcol;
			*(uint32_t*)&tcol.x = s_pack_half_rg(c0);
			*(uint32_t*)&tcol.y = s_pack_half_ba(c0);
			*(uint32_t*)&tcol.z = s_pack_half_rg(c1);
			*(uint32_t*)&tcol.w = s_pack_half_ba(c1);
			pay.add(tcol);
			for (int k = 0; k < 3; ++k) {
				CF_Color ta = geom.use_tri_attributes ? geom.tri_attributes[k] : geom.user_params;
				pay.add({ ta.r, ta.g, ta.b, ta.a });
			}
		}	break;

		case BATCH_GEOMETRY_TYPE_GLYPH:
		{
			// Curve glyph: outline-box parallelogram (BL origin, +x/+y edges match the
			// strip's box-fraction encoding) plus the strip's base texel in the atlas.
			CF_V2 q0 = geom.shape[3];
			CF_V2 e1 = geom.shape[2] - q0;
			CF_V2 e2 = geom.shape[0] - q0;
			if ((e1.x == 0 && e1.y == 0) || (e2.x == 0 && e2.y == 0)) continue;
			tc.type = 11u;
			tc.payload = (uint32_t)pay.count();
			pay.add({ q0.x, q0.y, e1.x, e1.y });
			pay.add({ e2.x, e2.y, 1.0f / dot(e1, e1), 1.0f / dot(e2, e2) });
			// The reported uv rect spans the entry's padded rect (atlas_use_border_pixels);
			// +1 steps past the 1px border onto the block's first content texel, and the
			// content width (padded minus borders) drives row wrapping for multi-row
			// path blocks (glyph strips are single-row and never wrap).
			float bx = CF_ROUNDF(cf_min(s->minx, s->maxx) * (float)texture_w) + 1.0f;
			float by = CF_ROUNDF(cf_min(s->miny, s->maxy) * (float)texture_h) + 1.0f;
			float bw = CF_ROUNDF(fabsf(s->maxx - s->minx) * (float)texture_w) - 2.0f;
			pay.add({ bx, by, cf_max(bw, 1.0f), 0 });
			// Per-corner colors (TL,TR,BR,BL), bilerped in the shader by box fraction so
			// text-effect color gradients carry over to the curve path.
			CF_TileV4 g0, g1;
			*(uint32_t*)&g0.x = s_pack_half_rg(geom.text_colors[0]);
			*(uint32_t*)&g0.y = s_pack_half_ba(geom.text_colors[0]);
			*(uint32_t*)&g0.z = s_pack_half_rg(geom.text_colors[1]);
			*(uint32_t*)&g0.w = s_pack_half_ba(geom.text_colors[1]);
			*(uint32_t*)&g1.x = s_pack_half_rg(geom.text_colors[2]);
			*(uint32_t*)&g1.y = s_pack_half_ba(geom.text_colors[2]);
			*(uint32_t*)&g1.z = s_pack_half_rg(geom.text_colors[3]);
			*(uint32_t*)&g1.w = s_pack_half_ba(geom.text_colors[3]);
			pay.add(g0);
			pay.add(g1);
		}	break;

		case BATCH_GEOMETRY_TYPE_QUAD:
		case BATCH_GEOMETRY_TYPE_CIRCLE:
		case BATCH_GEOMETRY_TYPE_CAPSULE:
		case BATCH_GEOMETRY_TYPE_SEGMENT:
		case BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED:
		case BATCH_GEOMETRY_TYPE_TRI_SDF:
		case BATCH_GEOMETRY_TYPE_POLYGON:
		case BATCH_GEOMETRY_TYPE_ARROW:
		case BATCH_GEOMETRY_TYPE_CUSTOM:
		case BATCH_GEOMETRY_TYPE_CSG:
		{
			is_sdf = true;
			switch (geom.type) {
			case BATCH_GEOMETRY_TYPE_QUAD: tc.type = 2u; break;
			case BATCH_GEOMETRY_TYPE_TRI_SDF: tc.type = 5u; break;
			case BATCH_GEOMETRY_TYPE_POLYGON: tc.type = 6u; tc.fill = 1.0f; break;
			case BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED: tc.type = 7u; break;
			case BATCH_GEOMETRY_TYPE_ARROW: tc.type = 8u; break;
			case BATCH_GEOMETRY_TYPE_CUSTOM: tc.type = 9u; break;
			case BATCH_GEOMETRY_TYPE_CSG: tc.type = 10u; break;
			default: tc.type = 3u; break; // Circle/capsule/segment all evaluate as segment SDF.
			}
			tc.payload = (uint32_t)pay.count();
			if (geom.type == BATCH_GEOMETRY_TYPE_POLYGON) {
				pay.add({ geom.shape[0].x, geom.shape[0].y, geom.shape[1].x, geom.shape[1].y });
				pay.add({ geom.shape[2].x, geom.shape[2].y, geom.shape[3].x, geom.shape[3].y });
				pay.add({ geom.shape[4].x, geom.shape[4].y, geom.shape[5].x, geom.shape[5].y });
				pay.add({ geom.shape[6].x, geom.shape[6].y, geom.shape[7].x, geom.shape[7].y });
			} else if (geom.type == BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED) {
				pay.add({ geom.shape[0].x, geom.shape[0].y, geom.shape[1].x, geom.shape[1].y });
				pay.add({ geom.shape[2].x, geom.shape[2].y, geom.shape[3].x, geom.shape[3].y });
				pay.add({ geom.shape[4].x, geom.shape[4].y, 0, 0 });
			} else if (geom.type == BATCH_GEOMETRY_TYPE_CUSTOM) {
				// P0..P3: the 16 user params. P4: pre-padded world bounds for the
				// instanced VS coverage quad.
				pay.add({ geom.shape[0].x, geom.shape[0].y, geom.shape[1].x, geom.shape[1].y });
				pay.add({ geom.shape[2].x, geom.shape[2].y, geom.shape[3].x, geom.shape[3].y });
				pay.add({ geom.shape[4].x, geom.shape[4].y, geom.shape[5].x, geom.shape[5].y });
				pay.add({ geom.shape[6].x, geom.shape[6].y, geom.shape[7].x, geom.shape[7].y });
				pay.add({ geom.box[0].x, geom.box[0].y, geom.box[2].x, geom.box[2].y });
			} else if (geom.type == BATCH_GEOMETRY_TYPE_CSG) {
				// P0: pre-padded composite bounds. Then six vec4s per operand: header
				// (prim, aux, op, k), (radius, 0, 0, 0), and the 8 shape param vec2s.
				// Operands trail the head in the stream.
				pay.add({ geom.box[0].x, geom.box[0].y, geom.box[2].x, geom.box[2].y });
				for (int oi = 0; oi < geom.n; ++oi) {
					const BatchGeometry& og = geoms[k + 1 + oi];
					float prim, aux = 0;
					switch (og.type) {
					case BATCH_GEOMETRY_TYPE_QUAD: prim = 2; break;
					case BATCH_GEOMETRY_TYPE_TRI_SDF: prim = 5; break;
					case BATCH_GEOMETRY_TYPE_POLYGON: prim = 6; aux = (float)og.n; break;
					case BATCH_GEOMETRY_TYPE_ARROW: prim = 8; break;
					case BATCH_GEOMETRY_TYPE_CUSTOM: prim = 9; aux = (float)og.n; break;
					default: prim = 3; break; // Circle/capsule/segment.
					}
					pay.add({ prim, aux, (float)og.csg_op, og.csg_k });
					pay.add({ og.radius, 0, 0, 0 });
					pay.add({ og.shape[0].x, og.shape[0].y, og.shape[1].x, og.shape[1].y });
					pay.add({ og.shape[2].x, og.shape[2].y, og.shape[3].x, og.shape[3].y });
					pay.add({ og.shape[4].x, og.shape[4].y, og.shape[5].x, og.shape[5].y });
					pay.add({ og.shape[6].x, og.shape[6].y, og.shape[7].x, og.shape[7].y });
				}
				k += geom.n;
			} else {
				pay.add({ geom.shape[0].x, geom.shape[0].y, geom.shape[1].x, geom.shape[1].y });
				pay.add({ geom.shape[2].x, geom.shape[2].y, 0, 0 });
			}
		}	break;
		}

		// Opaque-cover cull candidate? Filled SDF shape at full alpha under normal
		// blending (additive/multiply/screen shapes never hide what's beneath).
		// Clipped segments are excluded: their planes can cut mid-tile, so "interior
		// covers the tile" cannot be decided from the SDF alone.
		if (is_sdf && tc.fill == 1.0f && geom.alpha >= 1.0f && geom.color.a >= 1.0f && geom.type != BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED && blend == CF_DRAW_BLEND_NORMAL) {
			tc.opaque = 1.0f;
		}

		if (instanced) {
			// Rasterizer coverage: the instanced VS derives quads from the params, so
			// no CPU binning, culling, or pixel-space AABB is needed at all.
			cmds.add(tc);
			continue;
		}

		// GPU binning: the CPU only needs an exact list-capacity upper bound and
		// offscreen rejection; compute walks AABB tile rects with a per-tile SDF cull.
		int tx0, ty0, tx1, ty1;
		if (!cf_tile_range(axmin, aymin, axmax, aymax, tiles_x, tiles_y, &tx0, &ty0, &tx1, &ty1)) {
			continue; // Fully offscreen.
		}
		list_capacity += (uint32_t)((tx1 - tx0 + 1) * (ty1 - ty0 + 1));
		max_tile_rows = cf_max(max_tile_rows, ty1 - ty0 + 1);
		cmds.add(tc);
		ux0 = cf_min(ux0, axmin);
		uy0 = cf_min(uy0, aymin);
		ux1 = cf_max(ux1, axmax);
		uy1 = cf_max(uy1, aymax);
	}

	if (cmds.count() == 0) return;

	int cmds_bytes = cmds.count() * (int)sizeof(CF_TileCmd);
	int pay_bytes = pay.count() * (int)sizeof(CF_TileV4);

	if (instanced) {
		// Upload the same command/payload buffers the tiled path uses, then draw one
		// instance per command; the VS expands coverage quads GPU-side.
		cf_update_storage_buffer(s_draw->tile_cmds_buf, cmds.data(), cmds_bytes);
		if (pay_bytes) cf_update_storage_buffer(s_draw->tile_payload_buf, pay.data(), pay_bytes);
		CF_Texture atlas = texture_id ? CF_Texture{ texture_id } : s_draw->white_texture;
		cf_material_set_texture_fs(s_draw->material, "u_image", atlas);
		v2 u_texture_size = cf_v2((float)texture_w, (float)texture_h);
		cf_material_set_uniform_fs(s_draw->material, "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
		int alpha_discard = cmd.alpha_discard == 0.0f ? 0 : 1;
		cf_material_set_uniform_fs(s_draw->material, "u_alpha_discard", &alpha_discard, CF_UNIFORM_TYPE_INT, 1);
		int use_smooth_uv = cmd.filter_mode == CF_DRAW_FILTER_SMOOTH ? 0 : 1;
		cf_material_set_uniform_fs(s_draw->material, "u_use_smooth_uv", &use_smooth_uv, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_render_state(s_draw->material, s_blend_run_state(cmd.render_state, blend, false));
		void* sampler_override = (cmd.filter_mode == CF_DRAW_FILTER_NEAREST) ? s_draw->sampler_nearest : s_draw->sampler_linear;
		cf_set_sampler_override(sampler_override);
		cf_apply_mesh(s_draw->corner_mesh);
		cf_apply_shader(cmd.shader, s_draw->material);
		CF_StorageBuffer vs_bufs[2] = { s_draw->tile_cmds_buf, s_draw->tile_payload_buf };
		cf_apply_vs_storage_buffers(vs_bufs, 2);
		// The fragment stage reads the payload too (CSG operand lists).
		CF_StorageBuffer fs_bufs[1] = { s_draw->tile_payload_buf };
		cf_apply_fs_storage_buffers(fs_bufs, 1);
		CF_Rect viewport = cmd.viewport;
		if (viewport.w >= 0 && viewport.h >= 0) {
			cf_apply_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
		}
		CF_Rect scissor = cmd.scissor;
		if (scissor.w >= 0 && scissor.h >= 0) {
			cf_apply_scissor(scissor.x, scissor.y, scissor.w, scissor.h);
		}
		cf_push_gpu_label("instanced_draw");
		cf_draw_elements_instanced(cmds.count());
		cf_pop_gpu_label();
		s_draw->has_drawn_something = true;
		return;
	}

	{
		// Upload commands + payload only; four compute dispatches bin on the GPU.
		if (list_capacity == 0) return;
		cf_update_storage_buffer(s_draw->tile_cmds_buf, cmds.data(), cmds_bytes);
		if (pay_bytes) cf_update_storage_buffer(s_draw->tile_payload_buf, pay.data(), pay_bytes);
		s_tile_ensure_rw_buffer(&s_draw->tile_headers_buf, &s_draw->tile_headers_cap, tile_count * 2 * (int)sizeof(uint32_t));
		s_tile_ensure_rw_buffer(&s_draw->tile_list_buf, &s_draw->tile_list_cap, (int)list_capacity * (int)sizeof(uint32_t));

		v2 canvas_wh = cf_v2((float)canvas_w, (float)canvas_h);
		int cmd_count_i = cmds.count();
		int tile_px = CF_TILE_PX;
		CF_Material mat_cs = s_draw->tile_material_cs;
		cf_material_set_uniform_cs(mat_cs, "u_canvas_wh", &canvas_wh, CF_UNIFORM_TYPE_FLOAT2, 1);
		cf_material_set_uniform_cs(mat_cs, "u_cmd_count", &cmd_count_i, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(mat_cs, "u_tiles_x", &tiles_x, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(mat_cs, "u_tiles_y", &tiles_y, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(mat_cs, "u_tile_px", &tile_px, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(mat_cs, "u_tile_count", &tile_count, CF_UNIFORM_TYPE_INT, 1);

		CF_StorageBuffer ro[2] = { s_draw->tile_cmds_buf, s_draw->tile_payload_buf };
		{
			CF_ComputeDispatch d = cf_compute_dispatch_defaults((tile_count + 255) / 256, 1, 1);
			CF_StorageBuffer rw[1] = { s_draw->tile_headers_buf };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_push_gpu_label("tile_zero");
			cf_dispatch_compute(app->tile_zero_cs, mat_cs, d);
			cf_pop_gpu_label();
		}
		{
			CF_ComputeDispatch d = cf_compute_dispatch_defaults((cmd_count_i + 63) / 64, max_tile_rows, 1);
			CF_StorageBuffer rw[1] = { s_draw->tile_headers_buf };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			d.ro_buffers = ro;
			d.ro_buffer_count = 2;
			cf_push_gpu_label("tile_count");
			cf_dispatch_compute(app->tile_count_cs, mat_cs, d);
			cf_pop_gpu_label();
		}
		{
			CF_ComputeDispatch d = cf_compute_dispatch_defaults(1, 1, 1);
			CF_StorageBuffer rw[1] = { s_draw->tile_headers_buf };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_push_gpu_label("tile_scan");
			cf_dispatch_compute(app->tile_scan_cs, mat_cs, d);
			cf_pop_gpu_label();
		}
		{
			CF_ComputeDispatch d = cf_compute_dispatch_defaults((tile_count + 63) / 64, 1, 1);
			CF_StorageBuffer rw[2] = { s_draw->tile_headers_buf, s_draw->tile_list_buf };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			d.ro_buffers = ro;
			d.ro_buffer_count = 2;
			cf_push_gpu_label("tile_gather");
			cf_dispatch_compute(app->tile_gather_cs, mat_cs, d);
			cf_pop_gpu_label();
		}
		s_draw->tiled_batch_count++;
		s_draw->tiled_upload_bytes += (uint64_t)(cmds_bytes + pay_bytes);
	}

	// Material state, mirroring the mesh path.
	CF_Texture atlas = texture_id ? CF_Texture{ texture_id } : s_draw->white_texture;
	cf_material_set_texture_fs(s_draw->material, "u_image", atlas);
	v2 u_texture_size = cf_v2((float)texture_w, (float)texture_h);
	cf_material_set_uniform_fs(s_draw->material, "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
	v2 u_canvas_wh = cf_v2((float)canvas_w, (float)canvas_h);
	cf_material_set_uniform_fs(s_draw->material, "u_canvas_wh", &u_canvas_wh, CF_UNIFORM_TYPE_FLOAT2, 1);
	int alpha_discard = cmd.alpha_discard == 0.0f ? 0 : 1;
	cf_material_set_uniform_fs(s_draw->material, "u_alpha_discard", &alpha_discard, CF_UNIFORM_TYPE_INT, 1);
	int use_smooth_uv = cmd.filter_mode == CF_DRAW_FILTER_SMOOTH ? 0 : 1;
	cf_material_set_uniform_fs(s_draw->material, "u_use_smooth_uv", &use_smooth_uv, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_fs(s_draw->material, "u_tiles_x", &tiles_x, CF_UNIFORM_TYPE_INT, 1);
	int tile_px = CF_TILE_PX;
	cf_material_set_uniform_fs(s_draw->material, "u_tile_px", &tile_px, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_fs(s_draw->material, "u_blend", &blend, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_render_state(s_draw->material, s_blend_run_state(cmd.render_state, blend, true));

	void* sampler_override = (cmd.filter_mode == CF_DRAW_FILTER_NEAREST) ? s_draw->sampler_nearest : s_draw->sampler_linear;
	cf_set_sampler_override(sampler_override);

	cf_apply_mesh(s_draw->tile_mesh);
	cf_apply_shader(app->tile_shader, s_draw->material);
	CF_StorageBuffer bufs[4] = { s_draw->tile_cmds_buf, s_draw->tile_payload_buf, s_draw->tile_headers_buf, s_draw->tile_list_buf };
	cf_apply_fs_storage_buffers(bufs, 4);

	// Scissor down to the batch's coverage (intersected with any user scissor). The
	// render pass restarts each batch (buffer uploads end it), which resets scissor.
	int sx0 = (int)floorf(ux0);
	int sy0 = (int)floorf(uy0);
	int sx1 = (int)ceilf(ux1) + 1;
	int sy1 = (int)ceilf(uy1) + 1;
	sx0 = sx0 < 0 ? 0 : sx0;
	sy0 = sy0 < 0 ? 0 : sy0;
	sx1 = sx1 > canvas_w ? canvas_w : sx1;
	sy1 = sy1 > canvas_h ? canvas_h : sy1;
	CF_Rect scissor = cmd.scissor;
	if (scissor.w >= 0 && scissor.h >= 0) {
		int ix0 = cf_max(sx0, scissor.x);
		int iy0 = cf_max(sy0, scissor.y);
		int ix1 = cf_min(sx1, scissor.x + scissor.w);
		int iy1 = cf_min(sy1, scissor.y + scissor.h);
		sx0 = ix0; sy0 = iy0; sx1 = ix1; sy1 = iy1;
	}
	if (sx1 <= sx0 || sy1 <= sy0) return;
	cf_apply_scissor(sx0, sy0, sx1 - sx0, sy1 - sy0);

	cf_push_gpu_label("tile_walk");
	cf_draw_elements();
	cf_pop_gpu_label();
	s_draw->has_drawn_something = true;
}

static void s_draw_report(atlas_cache_entry_t* entries, int count, int texture_w, int texture_h, void* udata)
{
	CF_UNUSED(udata);
	// Stash each entry's atlas uvs + texture into the per-flush uv table. Rendering
	// happens after the flush (s_flush_pending_geoms): the stream renders in paint
	// order, splitting into a new draw wherever the bound texture changes, so paint
	// order holds even when sprites span multiple atlas textures.
	for (int i = 0; i < count; ++i) {
		const atlas_cache_entry_t* s = entries + i;
		CF_PendingUV& uv = s_draw->pending_uvs[(int)s->udata];
		uv.texture_id = s->texture_id;
		uv.minx = s->minx;
		uv.miny = s->miny;
		uv.maxx = s->maxx;
		uv.maxy = s->maxy;
		uv.tex_w = texture_w;
		uv.tex_h = texture_h;
	}
}

// Routes one paint-ordered run of the stream to the tiled or instanced path.
static void s_draw_report_range(const BatchGeometry* geoms, const CF_PendingUV* uvs, int start, int end, uint64_t texture_id, int texture_w, int texture_h, int blend)
{
	int total = end - start;
	if (total <= 0) return;
	if (s_tiled_batch_eligible(total)) {
		// Auto: the instanced path (rasterizer coverage) wins at moderate overdraw;
		// tiled wins decisively when its opaque-cover cull can engage (up to ~9x on
		// stacked opaque scenes). Route tiled only when a big opaque cover exists --
		// which requires normal blending (other modes never hide what's beneath).
		CF_TiledBatchStats stats = s_tiled_batch_stats(geoms, start, end);
		bool take = s_draw->tiled_mode == 0 ? (stats.has_big_opaque && blend == CF_DRAW_BLEND_NORMAL) : true;
		// Bin lists are sized as the sum of per-command tile footprints, and the gather
		// walks every command per touched tile -- a pathological batch (thousands of
		// screen-covering commands) would demand an unbounded list buffer and an
		// O(tiles x cmds) dispatch. Past the budget, the instanced path handles the
		// batch instead (it is O(cmds) regardless of footprint).
		if (take && stats.footprint_tiles > s_draw->tiled_list_budget) take = false;
		if (take) {
			s_draw_report_tiled(geoms, uvs, start, end, texture_id, texture_w, texture_h, blend, false);
			return;
		}
	}
	s_draw->instanced_batch_count++;
	if (!s_draw->instanced_available) return; // Draw shader failed to compile; nothing can render.
	s_draw_report_tiled(geoms, uvs, start, end, texture_id, texture_w, texture_h, blend, true);
}

//--------------------------------------------------------------------------------------------------
// Hidden API called by CF_App.

static void s_init_atlas_cache(int w, int h)
{
	atlas_cache_config_t config;
	atlas_cache_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.ticks_to_decay_texture = 100000;
	config.batch_callback = s_draw_report;
	config.get_pixels_callback = cf_get_pixels;
	config.generate_texture_callback = cf_generate_texture_handle;
	config.delete_texture_callback = cf_destroy_texture_handle;
	config.allocator_context = NULL;
	config.lonely_buffer_count_till_flush = 0;
	config.atlas_height_in_pixels = h;
	config.atlas_width_in_pixels = w;
	s_draw->atlas_dims = V2((float)w, (float)h);

	if (atlas_cache_init(&s_draw->atlas_cache, &config, NULL)) {
		CF_FREE(s_draw);
		s_draw = NULL;
		CF_ASSERT(false);
	}
}

void CF_Draw::reset_cam()
{
	cam_stack.clear();
	cam_stack.add(cf_make_identity());
	mvp = projection;
	s_draw->set_aaf();
}

// Sets the anti-alias factor, the width of roughly one pixel scaled.
// This factor remains constant-size despite zooming in/out with the camera.
void CF_Draw::set_aaf()
{
	float inv_cam_scale = 1.0f / len(s_draw->cam_stack.last().m.y);
	float scale = s_draw->antialias.last();
	// The canvas is now rasterized at `pixel_scale` device pixels per logical unit,
	// so divide by it here to keep the AA band one device pixel wide (instead of
	// one logical unit wide, which would now span multiple device pixels).
	aaf = scale * inv_cam_scale / app->pixel_scale;
}

void cf_make_draw()
{
	s_draw = CF_NEW(CF_Draw);
	s_draw->path_image_id_gen = CF_PATH_ID_RANGE_LO;
	s_draw->projection = ortho_2d(0, 0, (float)app->w, (float)app->h);
	s_draw->reset_cam();
	s_draw->uniform_arena = cf_make_arena(32, CF_MB);

	// Shaders.
	s_draw->shaders.add(app->draw_shader);

	// Material.
	s_draw->material = cf_make_material();
	CF_RenderState state = cf_render_state_defaults();
	state.blend.enabled = true;
	state.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.rgb_op = CF_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.alpha_op = CF_BLEND_OP_ADD;
	s_draw->render_states.add(state);
	cf_material_set_render_state(s_draw->material, state);

	// AtlasCacheer.
	s_init_atlas_cache(2048, 2048);

	// Create samplers for filter mode switching.
	s_draw->sampler_nearest = cf_create_draw_sampler(CF_FILTER_NEAREST);
	s_draw->sampler_linear = cf_create_draw_sampler(CF_FILTER_LINEAR);

	// 1x1 white texture bound as u_image for shape-only draws (shapes bypass the
	// atlas_cache, so there may be no atlas texture in the batch at all).
	{
		CF_TextureParams tp = cf_texture_defaults(1, 1);
		s_draw->white_texture = cf_make_texture(tp);
		CF_Pixel white = cf_make_pixel_rgba(255, 255, 255, 255);
		cf_texture_update(s_draw->white_texture, &white, sizeof(white));
	}

	// Command renderer resources (see cute_draw_internal.h). The instanced path only
	// needs the draw shader + cmds/payload buffers (on GLES those buffers are emulated
	// as textures backend-side); the tiled path additionally needs the walk shader and
	// the binning compute shaders.
	s_draw->default_render_state = state;
	s_draw->instanced_available = app->draw_shader.id != 0;
	if (s_draw->instanced_available) {
		// Corner-index mesh for the instanced command-fed path (two triangles of a
		// quad; corner 3 doubles as the degenerate vertex for raw triangles).
		Array<CF_VertexAttribute> corner_attrs;
		corner_attrs.add({ .name = "in_corner", .format = CF_VERTEX_FORMAT_FLOAT, .offset = 0 });
		s_draw->corner_mesh = cf_make_mesh(6 * sizeof(float), corner_attrs.data(), corner_attrs.count(), sizeof(float));
		float corners[6] = { 0, 3, 1, 1, 3, 2 };
		cf_mesh_update_vertex_data(s_draw->corner_mesh, corners, 6);
		CF_StorageBufferParams sb_params = cf_storage_buffer_defaults(16 * 1024);
		sb_params.graphics_readable = true;
		s_draw->tile_cmds_buf = cf_make_storage_buffer(sb_params);
		s_draw->tile_payload_buf = cf_make_storage_buffer(sb_params);
	}
	s_draw->tiled_available = s_draw->instanced_available && app->tile_shader.id != 0 &&
		app->tile_zero_cs.id && app->tile_count_cs.id && app->tile_scan_cs.id &&
		app->tile_gather_cs.id;
	if (s_draw->tiled_available) {
		Array<CF_VertexAttribute> tile_attrs;
		tile_attrs.add({ .name = "in_posH", .format = CF_VERTEX_FORMAT_FLOAT2, .offset = 0 });
		s_draw->tile_mesh = cf_make_mesh(3 * sizeof(CF_V2), tile_attrs.data(), tile_attrs.count(), sizeof(CF_V2));
		CF_V2 fullscreen_tri[3] = { cf_v2(-1, -1), cf_v2(3, -1), cf_v2(-1, 3) };
		cf_mesh_update_vertex_data(s_draw->tile_mesh, fullscreen_tri, 3);
		// GPU-written buffers for the binning dispatches.
		CF_StorageBufferParams sb_params = cf_storage_buffer_defaults(16 * 1024);
		sb_params.graphics_readable = true;
		sb_params.compute_writable = true;
		s_draw->tile_headers_buf = cf_make_storage_buffer(sb_params);
		s_draw->tile_list_buf = cf_make_storage_buffer(sb_params);
		s_draw->tile_headers_cap = 16 * 1024;
		s_draw->tile_list_cap = 16 * 1024;
		s_draw->tile_material_cs = cf_make_material();
	}

	// Create an initial draw command.
	s_draw->add_cmd();
}

void cf_destroy_draw()
{
	if (s_draw->blit_init) {
		cf_destroy_mesh(s_draw->blit_mesh);
	}
	if (s_draw->instanced_available) {
		cf_destroy_mesh(s_draw->corner_mesh);
		cf_destroy_storage_buffer(s_draw->tile_cmds_buf);
		cf_destroy_storage_buffer(s_draw->tile_payload_buf);
	}
	if (s_draw->tiled_available) {
		cf_destroy_mesh(s_draw->tile_mesh);
		cf_destroy_storage_buffer(s_draw->tile_headers_buf);
		cf_destroy_storage_buffer(s_draw->tile_list_buf);
		if (s_draw->tile_material_cs.id) cf_destroy_material(s_draw->tile_material_cs);
	}
	cf_destroy_draw_sampler(s_draw->sampler_nearest);
	cf_destroy_draw_sampler(s_draw->sampler_linear);
	cf_destroy_texture(s_draw->white_texture);
	atlas_cache_term(&s_draw->atlas_cache);
	cf_destroy_material(s_draw->material);
	s_draw->~CF_Draw();
	CF_FREE(s_draw);
}

//--------------------------------------------------------------------------------------------------

void cf_draw_sprite(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);
	atlas_cache_entry_t s = { };
	BatchGeometry& g = s_push_geom();

	// Changes to atlas_cache_internal_push_sprite() to support 9 slice sprites now requires all sprites to include
	// local sprite UVs, having minx/miny being 0 and maxx/maxy being 1 will draw the entire full sprite texture
	// to support what this did previously.
	s.minx = 0;
	s.miny = 0;
	s.maxx = 1;
	s.maxy = 1;

	bool apply_border_scale = true;
	if (sprite->id != CF_SPRITE_ID_INVALID) {
		if (sprite->blend_index > 0) {
			CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
			const char* anim_name = sprite->animation_name;
			const CF_Animation* anim = anim_name ? map_get(asset->animations, anim_name) : NULL;
			int global_frame = sprite->frame_index + (anim ? anim->frame_offset : 0);
			s.image_id = asset->blend_frame_ids[sprite->blend_index][global_frame];
		} else {
			s.image_id = sprite->_image_id;
		}
	} else if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		CF_AtlasSubImage sub_image = s_draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
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
	g.type = BATCH_GEOMETRY_TYPE_SPRITE;

	v2 offset = sprite->offset - (sprite->id != CF_SPRITE_ID_INVALID ? sprite->_pivot : V2(0,0));
	v2 p = cf_add(sprite->transform.p, cf_mul(offset, sprite->scale));

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

	CF_M3x2 m = s_draw->mvp;
	g.shape[0] = quad[0];
	g.shape[1] = quad[1];
	g.shape[2] = quad[2];
	g.shape[3] = quad[3];
	g.is_sprite = true;
	g.color = premultiply(color_white());
	g.alpha = sprite->opacity;
	g.user_params = s_draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

static ATLAS_CACHE_U64 s_sprite_image_id(const CF_Sprite* sprite)
{
	if (sprite->id != CF_SPRITE_ID_INVALID) {
		if (sprite->blend_index > 0) {
			CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
			const char* anim_name = sprite->animation_name;
			const CF_Animation* anim = anim_name ? map_get(asset->animations, anim_name) : NULL;
			int global_frame = sprite->frame_index + (anim ? anim->frame_offset : 0);
			return asset->blend_frame_ids[sprite->blend_index][global_frame];
		}
		return sprite->_image_id;
	}
	return sprite->easy_sprite_id;
}

// Remap local 0..1 UVs into a premade atlas sub-image's absolute UVs.
static void s_remap_premade_uvs(CF_AtlasSubImage sub, CF_V2* uv0, CF_V2* uv1, int count)
{
	float dx = sub.maxx - sub.minx;
	float dy = sub.maxy - sub.miny;
	for (int i = 0; i < count; ++i) {
		uv0[i].x = dx * uv0[i].x + sub.minx;
		uv0[i].y = dy * uv0[i].y + sub.miny;
		uv1[i].x = dx * uv1[i].x + sub.minx;
		uv1[i].y = dy * uv1[i].y + sub.miny;
	}
}

void cf_draw_sprite_9_slice(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);

	// No center patch — fall back to a normal sprite draw (ase, easy, and premade).
	CF_Aabb center_patch = sprite->_center_patch;
	if (center_patch.min.x == 0.0f && center_patch.min.y == 0.0f &&
		center_patch.max.x == 0.0f && center_patch.max.y == 0.0f) {
		cf_draw_sprite(sprite);
		return;
	}

	ATLAS_CACHE_U64 image_id = s_sprite_image_id(sprite);
	bool is_premade = sprite->id == CF_SPRITE_ID_INVALID
		&& sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO
		&& sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI;
	CF_AtlasSubImage premade_sub = { 0 };
	if (is_premade) {
		premade_sub = s_draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
	}

	// Center patch edges in Aseprite pixel space (origin top-left of sprite, Y down):
	// min = top-left of center, max = bottom-right of center.
	float left = center_patch.min.x;
	float right = center_patch.max.x;
	float min_y = center_patch.min.y;
	float max_y = center_patch.max.y;

	CF_V2 center_uv0 = cf_v2(left / sprite->w, min_y / sprite->h);
	CF_V2 center_uv1 = cf_v2(right / sprite->w, max_y / sprite->h);

	// Horizontal border thicknesses from each sprite edge to the center.
	float left_border = left;
	float right_border = sprite->w - right;
	// Vertical strip sizes fed into geometry (same mapping as the previous top/bottom locals).
	float strip_from_max_y = sprite->h - max_y;
	float strip_from_min_y = min_y;

	CF_V2 uvs0[] = {
		// top row
		cf_v2(0           , center_uv1.y),
		cf_v2(center_uv0.x, center_uv1.y),
		cf_v2(center_uv1.x, center_uv1.y),
		// middle row
		cf_v2(0           , center_uv0.y),
		cf_v2(center_uv0.x, center_uv0.y),
		cf_v2(center_uv1.x, center_uv0.y),
		// bottom row
		cf_v2(0           , 0.0f),
		cf_v2(center_uv0.x, 0.0f),
		cf_v2(center_uv1.x, 0.0f),
	};

	CF_V2 uvs1[] = {
		// top row
		cf_v2(center_uv0.x, 1.0f),
		cf_v2(center_uv1.x, 1.0f),
		cf_v2(1.0f        , 1.0f),

		// middle row
		cf_v2(center_uv0.x, center_uv1.y),
		cf_v2(center_uv1.x, center_uv1.y),
		cf_v2(1.0f        , center_uv1.y),

		// bottom row
		cf_v2(center_uv0.x, center_uv0.y),
		cf_v2(center_uv1.x, center_uv0.y),
		cf_v2(1.0f        , center_uv0.y),
	};

	if (is_premade) {
		s_remap_premade_uvs(premade_sub, uvs0, uvs1, 9);
	}

	// inner pieces needs to be scaled down by the sprite scale since we're operating in local quad space
	// otherwise we end up with just a normal scaled up sprite instead of a 9 slice one
	float full_width   = CF_FABSF(sprite->w * sprite->scale.x);
	float full_height  = CF_FABSF(sprite->h * sprite->scale.y);
	float inner_left   = left_border / full_width;
	float inner_right  = right_border / full_width;
	float inner_top    = strip_from_max_y / full_height;
	float inner_bottom = strip_from_min_y / full_height;

	CF_V2 quads[9][4] = {
		// top row
		{
			{ -0.5f             ,  0.5f                },
			{ -0.5f + inner_left,  0.5f                },
			{ -0.5f + inner_left,  0.5f - inner_top    },
			{ -0.5f             ,  0.5f - inner_top    },
		},
		{
			{ -0.5f + inner_left ,  0.5f               },
			{  0.5f - inner_right,  0.5f               },
			{  0.5f - inner_right,  0.5f - inner_top   },
			{ -0.5f + inner_left ,  0.5f - inner_top   },
		},
		{
			{  0.5f - inner_right,  0.5f               },
			{  0.5f              ,  0.5f               },
			{  0.5f              ,  0.5f - inner_top   },
			{  0.5f - inner_right,  0.5f - inner_top   },
		},
		// middle row
		{
			{ -0.5f              ,  0.5f - inner_top    },
			{ -0.5f + inner_left ,  0.5f - inner_top    },
			{ -0.5f + inner_left , -0.5f + inner_bottom },
			{ -0.5f              , -0.5f + inner_bottom },
		},
		{
			{ -0.5f + inner_left ,  0.5f - inner_top    },
			{  0.5f - inner_right,  0.5f - inner_top    },
			{  0.5f - inner_right, -0.5f + inner_bottom },
			{ -0.5f + inner_left , -0.5f + inner_bottom },
		},
		{
			{  0.5f - inner_right,  0.5f - inner_top    },
			{  0.5f              ,  0.5f - inner_top    },
			{  0.5f              , -0.5f + inner_bottom },
			{  0.5f - inner_right, -0.5f + inner_bottom },
		},
		// bottom row
		{
			{ -0.5f              , -0.5f + inner_bottom },
			{ -0.5f + inner_left , -0.5f + inner_bottom },
			{ -0.5f + inner_left , -0.5f                },
			{ -0.5f              , -0.5f                },
		},
		{
			{ -0.5f + inner_left , -0.5f + inner_bottom },
			{  0.5f - inner_right, -0.5f + inner_bottom },
			{  0.5f - inner_right, -0.5f                },
			{ -0.5f + inner_left , -0.5f                },
		},
		{
			{  0.5f - inner_right, -0.5f + inner_bottom },
			{  0.5f              , -0.5f + inner_bottom },
			{  0.5f              , -0.5f                },
			{  0.5f - inner_right, -0.5f                },
		},
	};

	v2 offset = sprite->offset - sprite->_pivot;
	v2 p = cf_add(sprite->transform.p, cf_mul_v2(offset, sprite->scale));
	v2 scale = V2(sprite->scale.x * sprite->w, sprite->scale.y * sprite->h);

	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 3; ++x) {
			atlas_cache_entry_t s = { 0 };
			BatchGeometry& g = s_push_geom();
			int index = x + y * 3;
			s.minx = uvs0[index].x;
			s.miny = uvs0[index].y;
			s.maxx = uvs1[index].x;
			s.maxy = uvs1[index].y;

			s.w = sprite->w ;
			s.h = sprite->h ;

			g.type = BATCH_GEOMETRY_TYPE_SPRITE;
			CF_V2* quad = quads[index];

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

			CF_M3x2 m = s_draw->mvp;
			g.shape[0] = quad[0];
			g.shape[1] = quad[1];
			g.shape[2] = quad[2];
			g.shape[3] = quad[3];
			g.is_sprite = true;
			g.color = premultiply(color_white());
			g.alpha = sprite->opacity;
			g.user_params = s_draw->user_params.last();
			s.image_id = image_id;
			DRAW_PUSH_ITEM(s);
		}
	}
}

void cf_draw_sprite_9_slice_tiled(const CF_Sprite* sprite)
{
	CF_ASSERT(sprite);

	// No center patch — fall back to a normal sprite draw (ase, easy, and premade).
	CF_Aabb center_patch = sprite->_center_patch;
	if (center_patch.min.x == 0.0f && center_patch.min.y == 0.0f &&
		center_patch.max.x == 0.0f && center_patch.max.y == 0.0f) {
		cf_draw_sprite(sprite);
		return;
	}

	ATLAS_CACHE_U64 image_id = s_sprite_image_id(sprite);
	bool is_premade = sprite->id == CF_SPRITE_ID_INVALID
		&& sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO
		&& sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI;
	CF_AtlasSubImage premade_sub = { 0 };
	if (is_premade) {
		premade_sub = s_draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
	}

	// Center patch edges in Aseprite pixel space (origin top-left of sprite, Y down):
	// min = top-left of center, max = bottom-right of center.
	float left = center_patch.min.x;
	float right = center_patch.max.x;
	float min_y = center_patch.min.y;
	float max_y = center_patch.max.y;

	CF_V2 center_uv0 = cf_v2(left / sprite->w, min_y / sprite->h);
	CF_V2 center_uv1 = cf_v2(right / sprite->w, max_y / sprite->h);

	// Horizontal border thicknesses from each sprite edge to the center.
	float left_border = left;
	float right_border = sprite->w - right;
	// Vertical strip sizes fed into geometry (same mapping as the previous top/bottom locals).
	float strip_from_max_y = sprite->h - max_y;
	float strip_from_min_y = min_y;

	CF_V2 uvs0[] = {
		// top row
		cf_v2(0           , center_uv1.y),
		cf_v2(center_uv0.x, center_uv1.y),
		cf_v2(center_uv1.x, center_uv1.y),
		// middle row
		cf_v2(0           , center_uv0.y),
		cf_v2(center_uv0.x, center_uv0.y),
		cf_v2(center_uv1.x, center_uv0.y),
		// bottom row
		cf_v2(0           , 0.0f),
		cf_v2(center_uv0.x, 0.0f),
		cf_v2(center_uv1.x, 0.0f),
	};

	CF_V2 uvs1[] = {
		// top row
		cf_v2(center_uv0.x, 1.0f),
		cf_v2(center_uv1.x, 1.0f),
		cf_v2(1.0f        , 1.0f),
		// middle row
		cf_v2(center_uv0.x, center_uv1.y),
		cf_v2(center_uv1.x, center_uv1.y),
		cf_v2(1.0f        , center_uv1.y),
		// bottom row
		cf_v2(center_uv0.x, center_uv0.y),
		cf_v2(center_uv1.x, center_uv0.y),
		cf_v2(1.0f        , center_uv0.y),
	};

	if (is_premade) {
		s_remap_premade_uvs(premade_sub, uvs0, uvs1, 9);
	}

	// inner pieces needs to be scaled down by the sprite scale since we're operating in local quad space
	// otherwise we end up with just a normal scaled up sprite instead of a 9 slice one
	float full_width   = CF_FABSF(sprite->w * sprite->scale.x);
	float full_height  = CF_FABSF(sprite->h * sprite->scale.y);
	float inner_left   = left_border / full_width;
	float inner_right  = right_border / full_width;
	float inner_top    = strip_from_max_y / full_height;
	float inner_bottom = strip_from_min_y / full_height;

	// tiled sizes in local space
	CF_V2 side_tiled_size = V2(	(center_patch.max.x - center_patch.min.x) / full_width,
								(center_patch.max.y - center_patch.min.y) / full_height);

	CF_V2 quads[9][4] = {
		// top row
		{
			{ -0.5f             ,  0.5f               },
			{ -0.5f + inner_left,  0.5f               },
			{ -0.5f + inner_left,  0.5f - inner_top   },
			{ -0.5f             ,  0.5f - inner_top   },
		},
		{
			{ -0.5f + inner_left ,  0.5f              },
			{  0.5f - inner_right,  0.5f              },
			{  0.5f - inner_right,  0.5f - inner_top  },
			{ -0.5f + inner_left ,  0.5f - inner_top  },
		},
		{
			{  0.5f - inner_right,  0.5f              },
			{  0.5f              ,  0.5f              },
			{  0.5f              ,  0.5f - inner_top  },
			{  0.5f - inner_right,  0.5f - inner_top  },
		},
		// middle row
		{
			{ -0.5f              ,  0.5f - inner_top   },
			{ -0.5f + inner_left ,  0.5f - inner_top   },
			{ -0.5f + inner_left , -0.5f + inner_bottom},
			{ -0.5f              , -0.5f + inner_bottom},
		},
		{
			{ -0.5f + inner_left ,  0.5f - inner_top   },
			{  0.5f - inner_right,  0.5f - inner_top   },
			{  0.5f - inner_right, -0.5f + inner_bottom},
			{ -0.5f + inner_left , -0.5f + inner_bottom},
		},
		{
			{  0.5f - inner_right,  0.5f - inner_top   },
			{  0.5f              ,  0.5f - inner_top   },
			{  0.5f              , -0.5f + inner_bottom},
			{  0.5f - inner_right, -0.5f + inner_bottom},
		},
		// bottom row
		{
			{ -0.5f              , -0.5f + inner_bottom},
			{ -0.5f + inner_left , -0.5f + inner_bottom},
			{ -0.5f + inner_left , -0.5f               },
			{ -0.5f              , -0.5f               },
		},
		{
			{ -0.5f + inner_left , -0.5f + inner_bottom},
			{  0.5f - inner_right, -0.5f + inner_bottom},
			{  0.5f - inner_right, -0.5f               },
			{ -0.5f + inner_left , -0.5f               },
		},
		{
			{  0.5f - inner_right, -0.5f + inner_bottom},
			{  0.5f              , -0.5f + inner_bottom},
			{  0.5f              , -0.5f               },
			{  0.5f - inner_right, -0.5f               },
		},
	};

	v2 offset = sprite->offset - sprite->_pivot;
	v2 p = cf_add(sprite->transform.p, cf_mul_v2(offset, sprite->scale));
	v2 scale = V2(sprite->scale.x * sprite->w, sprite->scale.y * sprite->h);

	auto push_quad = [&sprite, &image_id, &offset, &p, &scale](CF_V2* quad, CF_V2 uv0, CF_V2 uv1) {
		atlas_cache_entry_t s = { };
	BatchGeometry& g = s_push_geom();
		s.minx = uv0.x;
		s.miny = uv0.y;
		s.maxx = uv1.x;
		s.maxy = uv1.y;

		s.w = sprite->w;
		s.h = sprite->h;

		g.type = BATCH_GEOMETRY_TYPE_SPRITE;

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

		CF_M3x2 m = s_draw->mvp;
		g.shape[0] = quad[0];
		g.shape[1] = quad[1];
		g.shape[2] = quad[2];
		g.shape[3] = quad[3];
		g.is_sprite = true;
		g.color = premultiply(color_white());
		g.alpha = sprite->opacity;
		g.user_params = s_draw->user_params.last();
		s.image_id = image_id;
		DRAW_PUSH_ITEM(s);
	};

	auto push_tiled_quad_x = [&push_quad, &side_tiled_size](CF_V2* quad, CF_V2 uv0, CF_V2 uv1) {
		CF_V2 q0 = quad[0];
		CF_V2 q1 = quad[3];
		float current = quad[0].x;
		float end = quad[1].x;
		CF_V2 quad_increment = V2(side_tiled_size.x, 0);
		float increment = quad_increment.x;

		while (current + increment < end)
		{
			CF_V2 tiled_quad[] = {
				q0,
				cf_add(q0, quad_increment),
				cf_add(q1, quad_increment),
				q1,
			};

			push_quad(tiled_quad, uv0, uv1);
			q0 = cf_add(q0, quad_increment);
			q1 = cf_add(q1, quad_increment);
			current += increment;
		}

		// draw remainder
		if (current < end)
		{
			float scale = (end - current) / increment;
			uv1.x = uv0.x + (uv1.x - uv0.x) * scale;

			CF_V2 tiled_quad[] = {
				q0,
				cf_add(q0, cf_mul_v2_f(quad_increment, scale)),
				cf_add(q1, cf_mul_v2_f(quad_increment, scale)),
				q1,
			};

			push_quad(tiled_quad, uv0, uv1);
		}
	};

	auto push_tiled_quad_y = [&push_quad, &side_tiled_size](CF_V2* quad, CF_V2 uv0, CF_V2 uv1) {
		CF_V2 q0 = quad[3];
		CF_V2 q1 = quad[2];
		float current = quad[3].y;
		float end = quad[0].y;
		CF_V2 quad_increment = V2(0, side_tiled_size.y);
		float increment = side_tiled_size.y;

		while (current + increment < end)
		{
			CF_V2 tiled_quad[] = {
				cf_add(q0, quad_increment),
				cf_add(q1, quad_increment),
				q1,
				q0,
			};

			push_quad(tiled_quad, uv0, uv1);
			q0 = cf_add(q0, quad_increment);
			q1 = cf_add(q1, quad_increment);
			current += increment;
		}

		// draw remainder
		if (current < end)
		{
			float scale = (end - current) / increment;
			uv1.y = uv0.y + (uv1.y - uv0.y) * scale;

			CF_V2 tiled_quad[] = {
				cf_add(q0, cf_mul_v2_f(quad_increment, scale)),
				cf_add(q1, cf_mul_v2_f(quad_increment, scale)),
				q1,
				q0,
			};

			push_quad(tiled_quad, uv0, uv1);
		}
	};

	// push corners and center
	push_quad(quads[0], uvs0[0], uvs1[0]);
	push_quad(quads[2], uvs0[2], uvs1[2]);
	push_quad(quads[4], uvs0[4], uvs1[4]);
	push_quad(quads[6], uvs0[6], uvs1[6]);
	push_quad(quads[8], uvs0[8], uvs1[8]);
	// push sides
	push_tiled_quad_x(quads[1], uvs0[1], uvs1[1]);
	push_tiled_quad_y(quads[3], uvs0[3], uvs1[3]);
	push_tiled_quad_y(quads[5], uvs0[5], uvs1[5]);
	push_tiled_quad_x(quads[7], uvs0[7], uvs1[7]);
}

void cf_draw_prefetch(const CF_Sprite* sprite)
{
	if (sprite->id != CF_SPRITE_ID_INVALID) {
		CF_SpriteAsset* asset = cf_sprite_get_asset(sprite->id);
		CF_Animation** anim_vals = map_items(asset->animations);
		for (int i = 0; i < map_size(asset->animations); ++i) {
			CF_Animation* animation = anim_vals[i];
			for (int j = 0; j < asize(animation->frames); ++j) {
				CF_Frame* frame = animation->frames + j;
				atlas_cache_prefetch(&s_draw->atlas_cache, frame->id, sprite->w, sprite->h);
			}
		}
	} else if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		atlas_cache_prefetch(&s_draw->atlas_cache, sprite->easy_sprite_id, sprite->w, sprite->h);
	} else {
		atlas_cache_prefetch(&s_draw->atlas_cache, sprite->easy_sprite_id, sprite->w, sprite->h);
	}
}

static void s_draw_quad(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float stroke, float radius, bool fill)
{
	float aaf = s_draw->aaf;
	BatchGeometry& g = s_push_shape_geom();
	g.type = BATCH_GEOMETRY_TYPE_QUAD;

	v2 u = norm(p1 - p0);
	v2 v = skew(u);
	v2 he = V2(distance(p1, p0), distance(p3, p0)) * 0.5f;
	v2 c = ((p0 + p1) * 0.5f + (p2 + p3) * 0.5f) * 0.5f;
	v2 inflate = V2(stroke+radius+aaf,stroke+radius+aaf);
	p0 = p0 - u * inflate - v * inflate;
	p1 = p1 + u * inflate - v * inflate;
	p2 = p2 + u * inflate + v * inflate;
	p3 = p3 - u * inflate + v * inflate;

	g.box[0] = p0;
	g.box[1] = p1;
	g.box[2] = p2;
	g.box[3] = p3;
	g.shape[0] = c;
	g.shape[1] = he;
	g.shape[2] = u;
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = radius;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = aaf;
	g.user_params = s_draw->user_params.last();
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
	float aaf = s_draw->aaf;
	BatchGeometry& g = s_push_shape_geom();
	g.type = BATCH_GEOMETRY_TYPE_CIRCLE;

	v2 rr = V2(radius, radius);
	v2 inflate = V2(stroke+aaf, stroke+aaf);
	CF_Aabb bb = make_aabb(position - (rr+inflate), position + (rr+inflate));
	cf_aabb_verts(g.box, bb);
	g.shape[0] = position;
	g.shape[1] = position;
	g.shape[2] = position;
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = radius;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = aaf;
	g.user_params = s_draw->user_params.last();
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


static void s_draw_capsule(v2 a, v2 b, float stroke, float radius, bool fill)
{
	BatchGeometry& g = s_push_shape_geom();
	g.type = BATCH_GEOMETRY_TYPE_CAPSULE;

	float cap_pad = radius + stroke + s_draw->aaf;
	v2 cap_mn = V2(cf_min(a.x, b.x) - cap_pad, cf_min(a.y, b.y) - cap_pad);
	v2 cap_mx = V2(cf_max(a.x, b.x) + cap_pad, cf_max(a.y, b.y) + cap_pad);
	g.box[0] = cap_mn;
	g.box[1] = V2(cap_mx.x, cap_mn.y);
	g.box[2] = cap_mx;
	g.box[3] = V2(cap_mn.x, cap_mx.y);
	g.shape[0] = a;
	g.shape[1] = b;
	g.shape[2] = a;
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = radius;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();
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


static void s_draw_tri(v2 a, v2 b, v2 c, float stroke, float radius, bool fill)
{
	BatchGeometry& g = s_push_shape_geom();

	// A CSG group needs a distance function, so force the SDF triangle variant there.
	if (stroke > 0 || radius > 0 || !fill || s_draw->antialias.last() || s_draw->shape_group_active) {
		g.type = BATCH_GEOMETRY_TYPE_TRI_SDF;
		float tri_pad = radius + stroke + s_draw->aaf;
	v2 tri_mn = V2(cf_min(a.x, cf_min(b.x, c.x)) - tri_pad, cf_min(a.y, cf_min(b.y, c.y)) - tri_pad);
	v2 tri_mx = V2(cf_max(a.x, cf_max(b.x, c.x)) + tri_pad, cf_max(a.y, cf_max(b.y, c.y)) + tri_pad);
	g.box[0] = tri_mn;
	g.box[1] = V2(tri_mx.x, tri_mn.y);
	g.box[2] = tri_mx;
	g.box[3] = V2(tri_mn.x, tri_mx.y);
		g.shape[0] = a;
		g.shape[1] = b;
		g.shape[2] = c;
	} else {
		g.type = BATCH_GEOMETRY_TYPE_TRI;
		g.shape[0] = a;
		g.shape[1] = b;
		g.shape[2] = c;
	}

	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = radius;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();

	// Per-vertex triangle colors.
	g.use_tri_colors = s_draw->tri_colors0.count() > 1;
	if (g.use_tri_colors) {
		g.tri_colors[0] = premultiply(s_draw->tri_colors0.last());
		g.tri_colors[1] = premultiply(s_draw->tri_colors1.last());
		g.tri_colors[2] = premultiply(s_draw->tri_colors2.last());
	}

	// Per-vertex triangle attributes.
	g.use_tri_attributes = s_draw->tri_attributes0.count() > 1;
	if (g.use_tri_attributes) {
		g.tri_attributes[0] = s_draw->tri_attributes0.last();
		g.tri_attributes[1] = s_draw->tri_attributes1.last();
		g.tri_attributes[2] = s_draw->tri_attributes2.last();
	}

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
		return;
	} else if (count == 2) {
		cf_draw_line(pts[0], pts[1], thickness);
		return;
	}

	// One clipped-capsule command per segment: SDF = distance to the segment, coverage
	// cut by hard bisector planes at interior joints. The bisector partition assigns
	// every pixel to exactly one segment (translucent strokes never double-blend), and
	// round joins/caps fall out of the capsule SDF clamping to the shared joint point,
	// where neighboring bodies agree on distance. No joint triangulation, no case
	// analysis -- the coverage quad is a loose capsule OBB and the planes do the exact
	// work per pixel.
	CF_Color color = premultiply(s_draw->colors.last());
	CF_Color user_params = s_draw->user_params.last();
	float aaf = s_draw->aaf;

	// Bisector of two segment directions; perpendicular split for exact 180 folds.
	auto bisect = [](v2 da, v2 db) {
		v2 sum = da + db;
		float len2 = dot(sum, sum);
		return len2 > 1.0e-12f ? sum / sqrtf(len2) : skew(db);
	};

	int seg_count = loop ? count : count - 1;
	v2 d_first = norm(pts[1] - pts[0]);
	v2 d_prev = loop ? norm(pts[0] - pts[count - 1]) : V2(0, 0);
	v2 d_cur = d_first;
	for (int i = 0; i < seg_count; ++i) {
		int i1 = i + 1 == count ? 0 : i + 1;
		v2 a = pts[i];
		v2 b = pts[i1];
		bool has_prev = loop || i > 0;
		bool has_next = loop || i + 1 < seg_count;
		v2 d_next = d_cur;
		if (has_next) {
			if (i + 1 == seg_count) {
				d_next = d_first;
			} else {
				int i2 = i1 + 1 == count ? 0 : i1 + 1;
				d_next = norm(pts[i2] - pts[i1]);
			}
		}

		BatchGeometry& g = s_push_geom();
		g.type = BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED;
		g.color = color;
		g.alpha = 1.0f;
		g.radius = radius;
		g.stroke = 0;
		g.fill = true;
		g.aa = aaf;
		g.user_params = user_params;

		// Plane 0 (strict) keeps the far side of the start joint's bisector; plane 1
		// (inclusive) keeps the near side of the end joint's bisector, so the shared
		// boundary belongs to exactly one body. Disabled planes pass everything.
		if (has_prev) {
			v2 n0 = -bisect(d_prev, d_cur);
			g.shape[2] = n0;
			g.shape[4].x = dot(n0, a);
		} else {
			g.shape[2] = V2(0, 0);
			g.shape[4].x = 1.0f;
		}
		if (has_next) {
			v2 n1 = bisect(d_cur, d_next);
			g.shape[3] = n1;
			g.shape[4].y = dot(n1, b);
		} else {
			g.shape[3] = V2(0, 0);
			g.shape[4].y = 1.0f;
		}

		g.shape[0] = a;
		g.shape[1] = b;
		float body_pad = radius + s_draw->aaf;
		v2 body_mn = V2(cf_min(a.x, b.x) - body_pad, cf_min(a.y, b.y) - body_pad);
		v2 body_mx = V2(cf_max(a.x, b.x) + body_pad, cf_max(a.y, b.y) + body_pad);
		g.box[0] = body_mn;
		g.box[1] = V2(body_mx.x, body_mn.y);
		g.box[2] = body_mx;
		g.box[3] = V2(body_mn.x, body_mx.y);

		d_prev = d_cur;
		d_cur = d_next;
	}
}

void cf_draw_polygon_fill(const CF_V2* points, int count, float chubbiness)
{
	CF_ASSERT(count >= 3 && count <= 8);
	BatchGeometry& g = s_push_shape_geom();

	g.type = BATCH_GEOMETRY_TYPE_POLYGON;
	CF_Aabb bb = expand(make_aabb(points, count), s_draw->aaf+chubbiness);
	CF_V2 box[4];
	aabb_verts(box, bb);
	g.box[0] = box[0];
	g.box[1] = box[1];
	g.box[2] = box[2];
	g.box[3] = box[3];
	g.n = count;
	for (int i = 0; i < count; ++i) {
		g.shape[i] = points[i];
	}

	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = chubbiness;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();
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
	s_draw->temp.ensure_capacity(iters);
	s_draw->temp.clear();
	float step = 1.0f / (float)iters;
	s_draw->temp.add(a);
	for (int i = 1; i < iters; ++i) {
		CF_V2 p = cf_bezier(a, c0, b, i * step);
		s_draw->temp.add(p);
	}
	s_draw->temp.add(b);
	cf_draw_polyline(s_draw->temp.data(), s_draw->temp.count(), thickness, false);
}

void cf_draw_bezier_line2(CF_V2 a, CF_V2 c0, CF_V2 c1, CF_V2 b, int iters, float thickness)
{
	s_draw->temp.ensure_capacity(iters);
	s_draw->temp.clear();
	float step = 1.0f / (float)iters;
	s_draw->temp.add(a);
	for (int i = 1; i < iters; ++i) {
		CF_V2 p = cf_bezier2(a, c0, c1, b, i * step);
		s_draw->temp.add(p);
	}
	s_draw->temp.add(b);
	cf_draw_polyline(s_draw->temp.data(), s_draw->temp.count(), thickness, false);
}

void cf_draw_arrow(CF_V2 a, CF_V2 b, float thickness, float arrow_width)
{
	// One command: capsule shaft unioned with the triangular head inside a single SDF
	// (distance_arrow), so translucent arrows never double-blend at the seam.
	BatchGeometry& g = s_push_shape_geom();
	g.type = BATCH_GEOMETRY_TYPE_ARROW;
	float pad = cf_max(thickness * 0.5f, arrow_width) + s_draw->aaf;
	v2 mn = V2(cf_min(a.x, b.x) - pad, cf_min(a.y, b.y) - pad);
	v2 mx = V2(cf_max(a.x, b.x) + pad, cf_max(a.y, b.y) + pad);
	g.box[0] = mn;
	g.box[1] = V2(mx.x, mn.y);
	g.box[2] = mx;
	g.box[3] = V2(mn.x, mx.y);
	g.shape[0] = a;
	g.shape[1] = b;
	g.shape[2] = V2(thickness * 0.5f, arrow_width);
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = 0;
	g.stroke = 0;
	g.fill = true;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();
}

//--------------------------------------------------------------------------------------------------
// Custom user-registered SDF shapes.

CF_CustomShape cf_make_custom_shape(const char* sdf_src)
{
	CF_CustomShape result = { 0 };
	if (!s_draw->instanced_available) {
		fprintf(stderr, "cf_make_custom_shape: requires the command renderer (compute-capable backend).\n");
		return result;
	}
	s_draw->custom_shape_srcs.add(String(sdf_src));

	// Stitch every registered snippet into custom_shapes.shd: rename each sdf() to
	// sdf_<i> via the preprocessor, then generate the per-command dispatcher.
	String src;
	src.append("struct ShapeParams\n{\n\tvec2 a, b, c, d, e, f, g, h;\n\tvec4 attributes;\n};\n\n");
	for (int i = 0; i < s_draw->custom_shape_srcs.count(); ++i) {
		src.fmt_append("#define sdf sdf_%d\n", i);
		src.append(s_draw->custom_shape_srcs[i].c_str());
		src.append("\n#undef sdf\n\n");
	}
	src.append("float custom_sdf(int shape_id, vec2 p, ShapeParams s)\n{\n");
	for (int i = 0; i < s_draw->custom_shape_srcs.count(); ++i) {
		src.fmt_append("\tif (shape_id == %d) return sdf_%d(p, s);\n", i, i);
	}
	src.append("\treturn 3.402823e38;\n}\n");

	CF_Shader old_draw = app->draw_shader;
	if (!cf_recompile_draw_pipelines(src.c_str())) {
		s_draw->custom_shape_srcs.pop();
		fprintf(stderr, "cf_make_custom_shape: failed to compile the custom shape sdf() snippet.\n");
		return result;
	}

	// The default draw shader handle changed; patch the shader stack and any
	// already-recorded commands referencing the old handle.
	for (int i = 0; i < s_draw->shaders.count(); ++i) {
		if (s_draw->shaders[i].id == old_draw.id) s_draw->shaders[i] = app->draw_shader;
	}
	for (int i = 0; i < s_draw->cmds.count(); ++i) {
		if (s_draw->cmds[i].shader.id == old_draw.id) s_draw->cmds[i].shader = app->draw_shader;
	}

	result.id = (uint32_t)s_draw->custom_shape_srcs.count();
	return result;
}

static void s_draw_custom_shape(CF_CustomShape shape, CF_Aabb bounds, float stroke, bool fill, const float* params, int param_count)
{
	if (!shape.id || (int)shape.id > s_draw->custom_shape_srcs.count()) return;
	BatchGeometry& g = s_push_shape_geom();
	g.type = BATCH_GEOMETRY_TYPE_CUSTOM;
	float pad = stroke + s_draw->aaf;
	v2 mn = bounds.min - V2(pad, pad);
	v2 mx = bounds.max + V2(pad, pad);
	g.box[0] = mn;
	g.box[1] = V2(mx.x, mn.y);
	g.box[2] = mx;
	g.box[3] = V2(mn.x, mx.y);
	float* dst = &g.shape[0].x;
	CF_MEMSET(dst, 0, sizeof(float) * 16);
	if (params && param_count > 0) {
		CF_MEMCPY(dst, params, sizeof(float) * cf_min(param_count, 16));
	}
	g.n = (int)shape.id - 1; // Registry dispatch index.
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = 0;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();
}

void cf_draw_custom_shape(CF_CustomShape shape, CF_Aabb bounds, float thickness, const float* params, int param_count)
{
	s_draw_custom_shape(shape, bounds, thickness, false, params, param_count);
}

void cf_draw_custom_shape_fill(CF_CustomShape shape, CF_Aabb bounds, const float* params, int param_count)
{
	s_draw_custom_shape(shape, bounds, 0, true, params, param_count);
}

//--------------------------------------------------------------------------------------------------
// CSG shape groups: compose existing shape calls with boolean ops into one command.

void cf_draw_shape_group_begin()
{
	CF_ASSERT(!s_draw->shape_group_active);
	s_draw->shape_group_active = true;
	s_draw->shape_group_op = CF_SHAPE_OP_UNION;
	s_draw->shape_group_k = 0;
	s_draw->group_geoms.clear();
}

void cf_draw_shape_group_op(CF_ShapeOp op, float smoothing)
{
	s_draw->shape_group_op = (int)op;
	s_draw->shape_group_k = cf_max(smoothing, 0.0f);
}

static void s_draw_shape_group_end(float stroke, bool fill)
{
	if (!s_draw->shape_group_active) return;
	s_draw->shape_group_active = false;
	int count = s_draw->group_geoms.count();
	if (count == 0) return;

	// Composite bounds: union of the operands' (already stroke/aa padded) boxes.
	// Subtract/intersect only shrink the shape. Smooth blending can bulge outward by up
	// to k/4 near where surfaces meet, and the composite's own stroke extends past the
	// operand surfaces, so pad for both.
	v2 mn = V2(FLT_MAX, FLT_MAX), mx = V2(-FLT_MAX, -FLT_MAX);
	float max_k = 0;
	for (int i = 0; i < count; ++i) {
		const BatchGeometry& og = s_draw->group_geoms[i];
		for (int j = 0; j < 4; ++j) {
			mn = cf_min(mn, og.box[j]);
			mx = cf_max(mx, og.box[j]);
		}
		max_k = cf_max(max_k, og.csg_k);
	}
	float pad = stroke + s_draw->aaf + max_k * 0.25f;
	mn = mn - V2(pad, pad);
	mx = mx + V2(pad, pad);

	BatchGeometry& g = s_push_geom();
	g.type = BATCH_GEOMETRY_TYPE_CSG;
	g.box[0] = mn;
	g.box[1] = V2(mx.x, mn.y);
	g.box[2] = mx;
	g.box[3] = V2(mn.x, mx.y);
	g.n = count;
	g.color = premultiply(s_draw->colors.last());
	g.alpha = 1.0f;
	g.radius = 0;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = s_draw->aaf;
	g.user_params = s_draw->user_params.last();
	// Operands share the head's record space; use the first operand's camera snapshot
	// (camera changes mid-group are unsupported).
	g.mvp = s_draw->group_geoms[0].mvp;

	// Operands trail the head in the same geometry stream so they inherit the command's
	// lifetime (layered rendering can hold commands across flushes).
	CF_Command& cmd = s_draw->cmds.last();
	for (int i = 0; i < count; ++i) {
		cmd.geoms.add(s_draw->group_geoms[i]);
	}
	s_draw->group_geoms.clear();
}

void cf_draw_shape_group_end()
{
	s_draw_shape_group_end(0, true);
}

void cf_draw_shape_group_end_stroked(float thickness)
{
	s_draw_shape_group_end(thickness, false);
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

	// Measured x-height (top of a lowercase 'x'), for strikethrough placement at the
	// true lowercase center. Fonts without an 'x' fall back to a metrics estimate.
	font->x_height = 0;
	int x_index = stbtt_FindGlyphIndex(&font->info, 'x');
	if (x_index) {
		int gx0, gy0, gx1, gy1;
		if (stbtt_GetGlyphBox(&font->info, x_index, &gx0, &gy0, &gx1, &gy1)) {
			font->x_height = gy1;
		}
	}

	// Build the legacy kerning table, keyed by glyph index (as stb provides it). Used
	// as a fallback in cf_font_get_kern for fonts whose GPOS table isn't in a layout
	// stb's minimal parser understands, even though real kern data still exists here.
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

float cf_font_scale_for_pixel_height(CF_Font* font, float pixel_height)
{
	return stbtt_ScaleForPixelHeight(&font->info, pixel_height);
}

CF_INLINE uint64_t cf_glyph_key(int cp, float font_size, int blur)
{
	int k0 = cp;
	int k1 = (int)(font_size * 1000.0f);
	// Clamp here to match the clamp applied in s_render -- this keeps the cache key
	// consistent with the blur actually rendered, and keeps k2 safely within its 8-bit
	// field below regardless of what a caller passes to cf_push_font_blur().
	int k2 = clamp(blur, 0, 20);
	// Quantize pixel_scale (device pixels per logical unit) into 8 bits at 1/64 granularity
	// (range 0..~4.0x) so glyphs rasterized at different pixel densities -- e.g. a window
	// dragged between a 1x and 2x monitor -- don't alias onto the same cache entry. Clamp
	// before casting so an unexpectedly large density saturates instead of wrapping back
	// into a low, colliding bucket.
	int k3 = (int)clamp(app->pixel_scale * 64.0f, 0.0f, 255.0f);
	uint64_t key = ((uint64_t)k0 & 0xFFFFFFFFULL) << 32 | ((uint64_t)k1 & 0xFFFFULL) << 16 | ((uint64_t)k2 & 0xFFULL) << 8 | ((uint64_t)k3 & 0xFFULL);
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
	// Rasterize at physical resolution (bitmap dimensions are in device pixels) but keep the
	// quad geometry and advance in logical units, so text stays pixel-scale-invariant in
	// position/size and only gets sharper as `pixel_scale` increases.
	float pixel_scale = app->pixel_scale;

	// Create glyph quad.
	blur = clamp(blur, 0, 20);
	int pad = (int)CF_ROUNDF((blur + 2) * pixel_scale);
	float scale = stbtt_ScaleForPixelHeight(&font->info, font_size * pixel_scale);
	int xadvance, lsb, x0, y0, x1, y1;
	stbtt_GetGlyphHMetrics(&font->info, glyph->index, &xadvance, &lsb);
	stbtt_GetGlyphBitmapBox(&font->info, glyph->index, scale, scale, &x0, &y0, &x1, &y1);
	int w = x1 - x0 + pad*2;
	int h = y1 - y0 + pad*2;
	glyph->w = w;
	glyph->h = h;
	glyph->q0 = V2((float)x0, -(float)(y0 + h)) / pixel_scale; // Swapped y. Logical units.
	glyph->q1 = V2((float)(x0 + w), -(float)y0) / pixel_scale; // Swapped y. Logical units.
	glyph->xadvance = xadvance * scale / pixel_scale;
	glyph->rendered = true;

	// Glyphs with no ink (spaces, etc.) have nothing to rasterize -- layout metrics
	// above are still valid and needed, but skip allocating a bitmap/atlas slot for
	// them entirely, and skip drawing them (see `visible` check in s_draw_text).
	if (!glyph->visible) return;

	// Render glyph.
	uint8_t* pixels_1bpp = (uint8_t*)CF_CALLOC(w * h);
	CF_DEFER(CF_FREE(pixels_1bpp));
	stbtt_MakeGlyphBitmap(&font->info, pixels_1bpp + pad * w + pad, w - pad*2, h - pad*2, w, scale, scale, glyph->index);
	//s_save("glyph.png", pixels_1bpp, w, h);

	// Apply blur. `blur` is a logical-space radius; scale it to device pixels to
	// match the bitmap's rasterization resolution, so the blur looks the same
	// logical size regardless of pixel_scale (pad above already reserved room
	// for this device-space radius).
	int device_blur = (int)CF_ROUNDF(blur * pixel_scale);
	if (device_blur) s_blur(pixels_1bpp, w, h, w, device_blur);
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
	if (glyph->rendered) return glyph;

	// Render the glyph if it exists in the font, but is not yet rendered.
	s_render(font, glyph, font_size, blur);
	return glyph;
}

//--------------------------------------------------------------------------------------------------
// Curve text (cf_push_text_curves): glyph outlines cached as quadratic Beziers in an
// atlas strip instead of rasterized bitmaps. Scale-free -- one strip per glyph index
// serves every font size, zoom, and rotation.

// Emit one quadratic; straight lines become degenerate quads {a, b, b} so storage and
// the GPU winding classifier stay uniform (no per-segment type branching).
static CF_INLINE void s_curve_quad(Array<CF_V2>& cps, v2 a, v2 b, v2 c)
{
	cps.add(a);
	cps.add(b);
	cps.add(c);
}

// One quadratic approximating a cubic via the midpoint construction. Callers split the
// cubic first so the approximation error stays far below the strip's quantization step.
static CF_INLINE void s_cubic_as_quad(Array<CF_V2>& cps, v2 p0, v2 c0, v2 c1, v2 p1)
{
	v2 b = ((c0 + c1) * 3.0f - (p0 + p1)) * 0.25f;
	s_curve_quad(cps, p0, b, p1);
}

// Cubics only appear in CFF-flavored fonts. A fixed 4-way de Casteljau split keeps the
// per-quarter midpoint approximation visually exact at any zoom we care about.
static void s_cubic_to_quads(Array<CF_V2>& cps, v2 p0, v2 c0, v2 c1, v2 p1)
{
	v2 q0 = (p0 + c0) * 0.5f;
	v2 q1 = (c0 + c1) * 0.5f;
	v2 q2 = (c1 + p1) * 0.5f;
	v2 r0 = (q0 + q1) * 0.5f;
	v2 r1 = (q1 + q2) * 0.5f;
	v2 m = (r0 + r1) * 0.5f;
	v2 h0[4] = { p0, q0, r0, m };
	v2 h1[4] = { m, r1, q2, p1 };
	for (int half = 0; half < 2; ++half) {
		const v2* h = half == 0 ? h0 : h1;
		v2 a0 = (h[0] + h[1]) * 0.5f;
		v2 a1 = (h[1] + h[2]) * 0.5f;
		v2 a2 = (h[2] + h[3]) * 0.5f;
		v2 b0 = (a0 + a1) * 0.5f;
		v2 b1 = (a1 + a2) * 0.5f;
		v2 hm = (b0 + b1) * 0.5f;
		s_cubic_as_quad(cps, h[0], a0, b0, hm);
		s_cubic_as_quad(cps, hm, b1, a2, h[3]);
	}
}

// The atlas is 2048 wide with a 1px border per entry; one row per strip caps curves per
// glyph. Latin runs 10-60, dense CJK a few hundred -- over-budget outlines fall back to
// the rasterized atlas path per glyph.
#define CF_CURVE_GLYPH_MAX_STRIP_W 2040

static void s_build_glyph_curves(CF_Font* font, CF_CurveGlyph* cg, int glyph_index)
{
	stbtt_vertex* verts = NULL;
	int nverts = stbtt_GetGlyphShape(&font->info, glyph_index, &verts);
	if (nverts <= 0) return; // No outline (whitespace).

	Array<CF_V2> cps;
	v2 pen = V2(0,0);
	v2 start = V2(0,0);
	for (int i = 0; i < nverts; ++i) {
		const stbtt_vertex& v = verts[i];
		v2 p = V2((float)v.x, (float)v.y);
		switch (v.type) {
		case STBTT_vmove:
			// stb closes contours before each move, but guard against unclosed ones.
			if (i && (pen.x != start.x || pen.y != start.y)) s_curve_quad(cps, pen, start, start);
			start = p;
			break;
		case STBTT_vline:
			if (pen.x != p.x || pen.y != p.y) s_curve_quad(cps, pen, p, p);
			break;
		case STBTT_vcurve:
			s_curve_quad(cps, pen, V2((float)v.cx, (float)v.cy), p);
			break;
		case STBTT_vcubic:
			s_cubic_to_quads(cps, pen, V2((float)v.cx, (float)v.cy), V2((float)v.cx1, (float)v.cy1), p);
			break;
		}
		pen = p;
	}
	if (nverts && (pen.x != start.x || pen.y != start.y)) s_curve_quad(cps, pen, start, start);
	stbtt_FreeShape(&font->info, verts);

	int curve_count = cps.count() / 3;
	int strip_w = curve_count * 3;
	if (curve_count == 0 || strip_w > CF_CURVE_GLYPH_MAX_STRIP_W) return;

	// Quantization box: bounds over every control point (off-curve included, so nothing
	// clamps). Shared endpoints quantize identically, keeping contours watertight.
	v2 mn = cps[0];
	v2 mx = cps[0];
	for (int i = 1; i < cps.count(); ++i) {
		mn = V2(cf_min(mn.x, cps[i].x), cf_min(mn.y, cps[i].y));
		mx = V2(cf_max(mx.x, cps[i].x), cf_max(mx.y, cps[i].y));
	}
	v2 size = V2(cf_max(mx.x - mn.x, 1.0f), cf_max(mx.y - mn.y, 1.0f));

	// Encode: one RGBA8 texel per control point, 16-bit fixed-point x/y as fractions of
	// the box. Precision is box/65536 in font units -- far below a pixel at any sane zoom.
	CF_Pixel* px = (CF_Pixel*)CF_ALLOC(strip_w * sizeof(CF_Pixel));
	for (int i = 0; i < cps.count(); ++i) {
		uint32_t qx = (uint32_t)CF_ROUNDF((cps[i].x - mn.x) * (65535.0f / size.x));
		uint32_t qy = (uint32_t)CF_ROUNDF((cps[i].y - mn.y) * (65535.0f / size.y));
		qx = qx > 65535 ? 65535 : qx;
		qy = qy > 65535 ? 65535 : qy;
		CF_Pixel p;
		p.r = (uint8_t)(qx & 255);
		p.g = (uint8_t)(qx >> 8);
		p.b = (uint8_t)(qy & 255);
		p.a = (uint8_t)(qy >> 8);
		px[i] = p;
	}

	cg->image_id = app->font_image_id_gen++;
	app->font_pixels.insert(cg->image_id, px);
	font->image_ids.add(cg->image_id); // Font destruction frees the strip like any glyph bitmap.
	cg->curve_count = curve_count;
	cg->strip_w = strip_w;
	cg->box_min = mn;
	cg->box_max = mn + size;
}

CF_CurveGlyph* cf_font_get_glyph_curves(CF_Font* font, int codepoint)
{
	int glyph_index = stbtt_FindGlyphIndex(&font->info, codepoint);
	if (!glyph_index) glyph_index = 0xFFFD;
	CF_CurveGlyph* cg = font->curve_glyphs.try_get((uint64_t)glyph_index);
	if (cg) return cg;
	cg = font->curve_glyphs.insert((uint64_t)glyph_index);
	CF_MEMSET(cg, 0, sizeof(*cg));
	s_build_glyph_curves(font, cg, glyph_index);
	return cg;
}

//--------------------------------------------------------------------------------------------------
// Vector paths (cf_draw_path_begin/end): user Bezier paths baked into atlas blocks and
// rendered by the same per-pixel winding/stroke machinery as curve-text glyphs. Unlike
// glyph strips, paths may span multiple texel rows (see cf_glyph_cp's row wrapping).

static Array<CF_V2> s_path_cps;
static bool s_path_building = false;
static bool s_path_contour_open = false;
static v2 s_path_pen;
static v2 s_path_start;

static void s_path_close_contour()
{
	if (s_path_contour_open && (s_path_pen.x != s_path_start.x || s_path_pen.y != s_path_start.y)) {
		s_curve_quad(s_path_cps, s_path_pen, s_path_start, s_path_start);
		s_path_pen = s_path_start;
	}
	s_path_contour_open = false;
}

void cf_draw_path_begin()
{
	CF_ASSERT(!s_path_building);
	s_path_building = true;
	s_path_contour_open = false;
	s_path_cps.clear();
	s_path_pen = V2(0,0);
	s_path_start = V2(0,0);
}

void cf_draw_path_move_to(CF_V2 p)
{
	CF_ASSERT(s_path_building);
	s_path_close_contour();
	s_path_pen = p;
	s_path_start = p;
	s_path_contour_open = true;
}

void cf_draw_path_line_to(CF_V2 p)
{
	CF_ASSERT(s_path_building);
	if (s_path_pen.x != p.x || s_path_pen.y != p.y) {
		s_curve_quad(s_path_cps, s_path_pen, p, p);
	}
	s_path_pen = p;
	s_path_contour_open = true;
}

void cf_draw_path_quad_to(CF_V2 c, CF_V2 p)
{
	CF_ASSERT(s_path_building);
	s_curve_quad(s_path_cps, s_path_pen, c, p);
	s_path_pen = p;
	s_path_contour_open = true;
}

void cf_draw_path_cubic_to(CF_V2 c0, CF_V2 c1, CF_V2 p)
{
	CF_ASSERT(s_path_building);
	s_cubic_to_quads(s_path_cps, s_path_pen, c0, c1, p);
	s_path_pen = p;
	s_path_contour_open = true;
}

void cf_draw_path_close()
{
	CF_ASSERT(s_path_building);
	s_path_close_contour();
}

CF_DrawPath cf_draw_path_end()
{
	CF_ASSERT(s_path_building);
	s_path_building = false;
	s_path_close_contour();

	CF_DrawPath result = { 0 };
	int curve_count = s_path_cps.count() / 3;
	if (curve_count == 0) return result;

	// Quantization box over every control point (same scheme as glyph strips).
	v2 mn = s_path_cps[0];
	v2 mx = s_path_cps[0];
	for (int i = 1; i < s_path_cps.count(); ++i) {
		mn = V2(cf_min(mn.x, s_path_cps[i].x), cf_min(mn.y, s_path_cps[i].y));
		mx = V2(cf_max(mx.x, s_path_cps[i].x), cf_max(mx.y, s_path_cps[i].y));
	}
	v2 size = V2(cf_max(mx.x - mn.x, 1e-4f), cf_max(mx.y - mn.y, 1e-4f));

	// Encode into a texel block: one RGBA8 texel per control point, row-major. Rows wrap
	// at a fixed width; the shader fetch handles curves that straddle a row boundary.
	int texels = curve_count * 3;
	int strip_w = texels < 1020 ? texels : 1020;
	int strip_h = (texels + strip_w - 1) / strip_w;
	CF_Pixel* px = (CF_Pixel*)CF_ALLOC(strip_w * strip_h * sizeof(CF_Pixel));
	CF_MEMSET(px, 0, strip_w * strip_h * sizeof(CF_Pixel));
	for (int i = 0; i < s_path_cps.count(); ++i) {
		uint32_t qx = (uint32_t)CF_ROUNDF((s_path_cps[i].x - mn.x) * (65535.0f / size.x));
		uint32_t qy = (uint32_t)CF_ROUNDF((s_path_cps[i].y - mn.y) * (65535.0f / size.y));
		qx = qx > 65535 ? 65535 : qx;
		qy = qy > 65535 ? 65535 : qy;
		CF_Pixel p;
		p.r = (uint8_t)(qx & 255);
		p.g = (uint8_t)(qx >> 8);
		p.b = (uint8_t)(qy & 255);
		p.a = (uint8_t)(qy >> 8);
		px[i] = p;
	}

	uint64_t id = s_draw->path_image_id_gen++;
	CF_DrawPathData pd;
	pd.pixels = px;
	pd.curve_count = curve_count;
	pd.strip_w = strip_w;
	pd.strip_h = strip_h;
	pd.box_min = mn;
	pd.box_max = mn + size;
	s_draw->draw_paths.insert(id, pd);
	result.id = id;
	return result;
}

void cf_destroy_path(CF_DrawPath path)
{
	CF_DrawPathData* pd = s_draw->draw_paths.try_get(path.id);
	if (!pd) return;
	CF_FREE(pd->pixels);
	s_draw->draw_paths.remove(path.id);
}

static void s_draw_path(CF_DrawPath path, float stroke, bool fill)
{
	CF_DrawPathData* pd = s_draw->draw_paths.try_get(path.id);
	if (!pd) return;
	float aaf = s_draw->aaf;

	atlas_cache_entry_t s = { };
	s.minx = 0;
	s.miny = 0;
	s.maxx = 1;
	s.maxy = 1;
	s.image_id = path.id;
	s.w = pd->strip_w;
	s.h = pd->strip_h;

	BatchGeometry& g = s_push_geom();
	g.type = BATCH_GEOMETRY_TYPE_GLYPH;
	g.n = pd->curve_count;
	v2 o0 = pd->box_min;
	v2 o1 = pd->box_max;
	g.shape[0] = V2(o0.x, o1.y);
	g.shape[1] = V2(o1.x, o1.y);
	g.shape[2] = V2(o1.x, o0.y);
	g.shape[3] = V2(o0.x, o0.y);
	float pad = stroke + aaf;
	g.box[0] = V2(o0.x - pad, o1.y + pad);
	g.box[1] = V2(o1.x + pad, o1.y + pad);
	g.box[2] = V2(o1.x + pad, o0.y - pad);
	g.box[3] = V2(o0.x - pad, o0.y - pad);
	g.color = premultiply(s_draw->colors.last());
	for (int i = 0; i < 4; ++i) g.text_colors[i] = g.color;
	g.alpha = 1.0f;
	g.stroke = stroke;
	g.fill = fill;
	g.aa = aaf;
	g.is_text = true; // Routes the pending-uv/atlas flow (path blocks are atlas entries).
	g.user_params = s_draw->user_params.last();
	DRAW_PUSH_ITEM(s);
}

void cf_draw_path_fill(CF_DrawPath path)
{
	s_draw_path(path, 0, true);
}

void cf_draw_path(CF_DrawPath path, float thickness)
{
	s_draw_path(path, thickness, false);
}

//--------------------------------------------------------------------------------------------------
// Retained draw lists (cf_make_draw_list): record a static snapshot of draw calls once,
// replay it per frame under the current camera. Recording happens in list-local space
// (identity camera + projection), so replay composes the caller's transform onto every
// stored command; replay re-pushes the recorded atlas entries so sprite/text uvs always
// resolve against the live atlas (defrag/decay safe).

CF_DrawList cf_make_draw_list()
{
	CF_DrawListData* data = CF_NEW(CF_DrawListData);
	uint64_t id = s_draw->draw_list_id_gen++;
	s_draw->draw_lists.insert(id, data);
	CF_DrawList result = { id };
	return result;
}

static void s_draw_list_free_uniforms(CF_DrawListData* data)
{
	for (int i = 0; i < data->uniform_blocks.count(); ++i) {
		CF_FREE(data->uniform_blocks[i]);
	}
	data->uniform_blocks.clear();
}

void cf_destroy_draw_list(CF_DrawList list)
{
	CF_DrawListData** data = s_draw->draw_lists.try_get(list.id);
	if (!data) return;
	CF_ASSERT(s_draw->recording_list != *data);
	s_draw_list_free_uniforms(*data);
	(*data)->~CF_DrawListData();
	CF_FREE(*data);
	s_draw->draw_lists.remove(list.id);
}

// Replayed geometry keeps its record-time coverage quad, whose AA inflation was
// computed under the identity recording camera (roughly one world unit). When
// the replay camera zooms out, the AA band widens in list-local units and the
// fringe would clip against the stale quad -- harmless on the instanced path
// (its vertex shader re-expands coverage from shape + aa), but the tiled walk
// bins and rasterizes straight from the quad, visibly dimming subpixel shapes
// (twinkling stars). Re-expand the quad by the extra band width along its own
// edge axes, exact for the parallelogram quads every SDF shape records.
static void s_replay_inflate_quad(BatchGeometry* g, float extra)
{
	if (g->is_sprite || g->is_text || g->csg_operand) return;
	if (g->type == BATCH_GEOMETRY_TYPE_SPRITE || g->type == BATCH_GEOMETRY_TYPE_TRI) return;
	CF_V2 u = g->box[1] - g->box[0];
	CF_V2 v = g->box[3] - g->box[0];
	float ul = len(u);
	float vl = len(v);
	if (ul <= 1.0e-6f || vl <= 1.0e-6f) return;
	CF_V2 du = u * (extra / ul);
	CF_V2 dv = v * (extra / vl);
	g->box[0] = g->box[0] - du - dv;
	g->box[1] = g->box[1] + du - dv;
	g->box[2] = g->box[2] + du + dv;
	g->box[3] = g->box[3] - du + dv;
}

void cf_draw_list_begin(CF_DrawList list)
{
	CF_DrawListData** data = s_draw->draw_lists.try_get(list.id);
	CF_ASSERT(data);
	CF_ASSERT(!s_draw->recording_list);
	if (!data) return;
	s_draw->recording_list = *data;
	s_draw->recording_mark = s_draw->cmds.count();
	// Record in list-local space so replay can compose any camera on top.
	cf_draw_push();
	s_draw->cam_stack.last() = cf_make_identity();
	s_draw->projection = cf_make_identity();
	s_draw->mvp = cf_make_identity();
	s_draw->set_aaf();
	s_draw->add_cmd();
}

void cf_draw_list_end()
{
	CF_DrawListData* data = s_draw->recording_list;
	CF_ASSERT(data);
	if (!data) return;
	s_draw->recording_list = NULL;
	s_draw_list_free_uniforms(data);
	data->cmds.clear();
	for (int i = s_draw->recording_mark; i < s_draw->cmds.count(); ++i) {
		CF_Command& c = s_draw->cmds[i];
		CF_ASSERT(!c.is_canvas); // Canvas blits reference mutable textures; not retainable.
		if (c.is_canvas) continue;
		// Skip state-only churn (empty commands from stack pushes during recording).
		if (c.geoms.count() == 0 && !c.geoms_ref && c.items.count() == 0 && !c.u.data && !c.u.is_texture) continue;
		CF_Command copy = c;
		if (c.geoms_ref) {
			// A nested replay recorded into this list: resolve the borrowed geometry to
			// an owned copy so lists never reference each other's storage.
			copy.geoms = *c.geoms_ref;
			copy.geoms_ref = NULL;
			for (int j = 0; j < copy.geoms.count(); ++j) {
				BatchGeometry& g = copy.geoms[j];
				CF_MUL_M32_M32(g.mvp, c.replay_mvp, g.mvp);
				float extra = g.aa * (c.replay_aa_scale - 1.0f);
				g.aa *= c.replay_aa_scale;
				if (extra > 0) s_replay_inflate_quad(&g, extra);
			}
		}
		if (copy.u.data) {
			// Uniform data lives in the per-frame arena; the list owns its own copy.
			void* block = CF_ALLOC(copy.u.size);
			CF_MEMCPY(block, copy.u.data, copy.u.size);
			copy.u.data = block;
			data->uniform_blocks.add(block);
		}
		data->cmds.add(copy);
	}
	s_draw->cmds.set_count(s_draw->recording_mark);
	cf_draw_pop(); // Restores camera, projection, mvp, and aaf.
}

void cf_draw_list(CF_DrawList list)
{
	CF_DrawListData** data_ptr = s_draw->draw_lists.try_get(list.id);
	if (!data_ptr) return;
	CF_DrawListData* data = *data_ptr;
	// Replays borrow the list's geometry (no deep copy): the collate step flattens
	// geoms_ref into the pending stream, composing the replay transform and rescaling
	// the AA band (recorded under an identity camera) during its one copy.
	float inv_cam_scale = 1.0f / len(s_draw->cam_stack.last().m.y);
	for (int i = 0; i < data->cmds.count(); ++i) {
		const CF_Command& src = data->cmds[i];
		CF_Command& c = s_draw->add_cmd();
		c.layer = src.layer;
		c.scissor = src.scissor;
		c.viewport = src.viewport;
		c.alpha_discard = src.alpha_discard;
		c.filter_mode = src.filter_mode;
		c.render_state = src.render_state;
		c.shader = src.shader;
		c.u = src.u;
		c.items = src.items;
		c.geoms_ref = &src.geoms;
		c.replay_mvp = s_draw->mvp;
		c.replay_aa_scale = inv_cam_scale;
	}
	// Reopen a command carrying the caller's current state for subsequent draws.
	s_draw->add_cmd();
}

float cf_font_get_kern(CF_Font* font, float font_size, int code0, int code1)
{
	// Prefer GPOS -- stb's codepoint API converts to glyph indices internally and
	// reads GPOS pair-adjustment tables, which is where most modern fonts keep
	// kerning. But stb's GPOS parser only understands a narrow set of lookup
	// formats: some fonts (e.g. this project's bundled Calibri) have a GPOS table
	// stb can't read, yet still carry real data in the legacy `kern` table. Since
	// stb only consults `kern` when there's no GPOS table at all, fall back to our
	// own glyph-index lookup into `kerning` (built in cf_make_font_from_data) when
	// the GPOS path comes up empty.
	int advance = stbtt_GetCodepointKernAdvance(&font->info, code0, code1);
	if (!advance) {
		int g0 = stbtt_FindGlyphIndex(&font->info, code0);
		int g1 = stbtt_FindGlyphIndex(&font->info, code1);
		int* val = font->kerning.try_get(CF_KERN_KEY(g0, g1));
		if (val) advance = *val;
	}
	return advance * stbtt_ScaleForPixelHeight(&font->info, font_size);
}

void cf_push_font(const char* font)
{
	s_draw->fonts.add(sintern(font));
}

const char* cf_pop_font()
{
	if (s_draw->fonts.count() > 1) {
		return s_draw->fonts.pop();
	} else {
		return s_draw->fonts.last();
	}
}

const char* cf_peek_font()
{
	return s_draw->fonts.last();
}

void cf_push_font_size(float size)
{
	s_draw->font_sizes.add(size);
}

float cf_pop_font_size()
{
	if (s_draw->font_sizes.count() > 1) {
		return s_draw->font_sizes.pop();
	} else {
		return s_draw->font_sizes.last();
	}
}

float cf_peek_font_size()
{
	return s_draw->font_sizes.last();
}

void cf_push_font_blur(int blur)
{
	s_draw->blurs.add(blur);
}

int cf_pop_font_blur()
{
	if (s_draw->blurs.count() > 1) {
		return s_draw->blurs.pop();
	} else {
		return s_draw->blurs.last();
	}
}

int cf_peek_font_blur()
{
	return s_draw->blurs.last();
}

void cf_push_text_wrap_width(float width)
{
	s_draw->text_wrap_widths.add(width);
}

float cf_pop_text_wrap_width()
{
	if (s_draw->text_wrap_widths.count() > 1) {
		return s_draw->text_wrap_widths.pop();
	} else {
		return s_draw->text_wrap_widths.last();
	}
}

float cf_peek_text_wrap_width()
{
	return s_draw->text_wrap_widths.last();
}

void cf_push_text_vertical_layout(bool layout_vertically)
{
	s_draw->vertical.add(layout_vertically);
}

bool cf_pop_text_vertical_layout()
{
	if (s_draw->vertical.count() > 1) {
		return s_draw->vertical.pop();
	} else {
		return s_draw->vertical.last();
	}
}

bool cf_peek_text_vertical_layout()
{
	return s_draw->vertical.last();
}

void cf_push_text_id(uint64_t id)
{
	s_draw->text_ids.add(id);
}

uint64_t cf_pop_text_id()
{
	if (s_draw->text_ids.count() > 1) {
		return s_draw->text_ids.pop();
	} else {
		return s_draw->text_ids.last();
	}
}

uint64_t cf_peek_text_id()
{
	return s_draw->text_ids.last();
}

void cf_push_text_effect_active(bool text_effects_on)
{
	s_draw->text_effects.add(text_effects_on);
}

bool cf_pop_text_effect_active()
{
	if (s_draw->text_effects.count() > 1) {
		return s_draw->text_effects.pop();
	} else {
		return s_draw->text_effects.last();
	}
}

bool cf_peek_text_effect_active()
{
	return s_draw->text_effects.last();
}

void cf_push_text_curves(bool curves_on)
{
	s_draw->text_curves.add(curves_on);
}

bool cf_pop_text_curves()
{
	if (s_draw->text_curves.count() > 1) {
		return s_draw->text_curves.pop();
	} else {
		return s_draw->text_curves.last();
	}
}

bool cf_peek_text_curves()
{
	return s_draw->text_curves.last();
}

void cf_push_text_stroke(float stroke)
{
	s_draw->text_strokes.add(stroke);
}

float cf_pop_text_stroke()
{
	if (s_draw->text_strokes.count() > 1) {
		return s_draw->text_strokes.pop();
	} else {
		return s_draw->text_strokes.last();
	}
}

float cf_peek_text_stroke()
{
	return s_draw->text_strokes.last();
}

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render = true, cf_text_markup_info_fn* markups = NULL);

// Draws collected per-glyph strike/underline segments. Contiguous segments with the
// same thickness and color merge into one polyline: the polyline's bisector partition
// assigns every pixel to exactly one body, so the overlapping round caps at glyph
// boundaries never double-blend (visible as dots on translucent or AA'd lines). Wavy
// lines connect through each junction instead of stair-stepping.
static void s_draw_strike_lines(Cute::Array<CF_Strike>& lines)
{
	Array<CF_V2> pts;
	int i = 0;
	while (i < lines.count()) {
		const CF_Strike& run = lines[i];
		int j = i;
		while (j + 1 < lines.count()) {
			const CF_Strike& next = lines[j + 1];
			if (next.thickness != run.thickness) break;
			if (CF_MEMCMP(&next.color, &run.color, sizeof(next.color)) != 0) break;
			// Contiguity in x (kerning nudges endpoints a little either way).
			if (fabsf(next.p0.x - lines[j].p1.x) > cf_max(2.0f, run.thickness)) break;
			++j;
		}
		pts.clear();
		pts.add(run.p0);
		for (int k = i + 1; k <= j; ++k) pts.add(lines[k].p0);
		pts.add(lines[j].p1);
		cf_draw_push_color(run.color);
		cf_draw_polyline(pts.data(), pts.count(), run.thickness, false);
		cf_draw_pop_color();
		i = j + 1;
	}
}

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

// Resolve the active font name + size for a sanitized-string glyph index by walking
// markup codes that cover that index. Later (typically nested/inner) overrides win.
// Base font/size are the currently pushed stack values passed in by the caller.
static void s_resolve_text_style(CF_ParsedTextState* text_state, int glyph_index, const char** font_name, float* font_size)
{
	if (!text_state) return;

	// Interned keys for the built-in <font> effect and its parameters.
	static const char* s_font_effect = NULL;
	static const char* s_name_key = NULL;
	static const char* s_font_key = NULL;
	static const char* s_size_key = NULL;
	if (!s_font_effect) {
		s_font_effect = sintern("font");
		s_name_key = sintern("name");
		s_font_key = sintern("font");
		s_size_key = sintern("size");
	}

	// Codes are sorted by index_in_string at parse time, so we can stop once starts
	// pass our glyph. Active codes are applied in start order so the last one wins.
	for (int i = 0; i < text_state->codes.count(); ++i) {
		CF_TextCode* code = text_state->codes + i;
		if (glyph_index < code->index_in_string) break;
		if (glyph_index >= code->index_in_string + code->glyph_count) continue;

		CF_TextEffectDef* def = app->text_effect_defs.try_find(code->effect_name);
		if (def && def->font_name && cf_font_get(def->font_name)) {
			*font_name = def->font_name;
		}

		if (code->effect_name == s_font_effect) {
			const CF_TextCodeVal* name_val = code->params.try_find(s_name_key);
			if (!name_val) name_val = code->params.try_find(s_font_key);
			if (name_val && name_val->type == CF_TEXT_CODE_VAL_TYPE_STRING && name_val->u.string) {
				if (cf_font_get(name_val->u.string)) {
					*font_name = name_val->u.string;
				}
			}
			const CF_TextCodeVal* size_val = code->params.try_find(s_size_key);
			if (size_val && size_val->type == CF_TEXT_CODE_VAL_TYPE_NUMBER) {
				*font_size = (float)size_val->u.number;
			}
		}
	}
}

static const char* s_find_end_of_line(CF_ParsedTextState* text_state, bool do_effects, const char* text, int start_index, float wrap_width)
{
	const char* base_font_name = s_draw->fonts.last();
	float base_font_size = s_draw->font_sizes.last();
	int blur = s_draw->blurs.last();
	CF_Font* base_font = cf_font_get(base_font_name);
	float x = 0;
	const char* start_of_word = 0;
	float word_w = 0;
	int cp;
	int cp_prev = 0;
	// Mirrors the main render loop's hit_newline gating, so wrap points land where
	// the kerned text will actually break.
	bool at_line_start = true;
	int index = start_index;
	const char* prev_font_name = NULL;
	float prev_font_size = 0;

	while (*text) {
		const char* text_prev = text;
		text = cf_string_decode_UTF8(text, &cp);

		const char* font_name = base_font_name;
		float font_size = base_font_size;
		if (do_effects) {
			s_resolve_text_style(text_state, index, &font_name, &font_size);
		}
		CF_Font* font = cf_font_get(font_name);
		if (!font) font = base_font;
		CF_Glyph* glyph = cf_font_get_glyph(font, cp, font_size, blur);
		if (!glyph) {
			++index;
			continue;
		}

		if (cp == '\n') {
			x = 0;
			word_w = 0;
			start_of_word = 0;
			at_line_start = true;
			prev_font_name = NULL;
			++index;
			continue;
		} else if (cp == '\r') {
			++index;
			continue;
		} else {
			float kern = 0;
			if (!at_line_start && cp_prev && prev_font_name == font_name && prev_font_size == font_size) {
				kern = cf_font_get_kern(font, font_size, cp_prev, cp);
			}
			float advance = glyph->xadvance + kern;
			if (s_is_space(cp)) {
				x += word_w + advance;
				word_w = 0;
				start_of_word = 0;
			} else {
				if (!start_of_word) {
					start_of_word = text_prev;
				}
				if (x + word_w + advance < wrap_width) {
					word_w += advance;
				} else {
					if (word_w + advance < wrap_width) {
						// Put entire word on the next line.
						return start_of_word;
					} else {
						// Word itself does not fit on one line, so just cut it here.
						return text;
					}
				}
			}
			cp_prev = cp;
			prev_font_name = font_name;
			prev_font_size = font_size;
			at_line_start = false;
			++index;
		}
	}

	return text + 1;
}

// Scan a single visual line [text, end_of_line) and report its vertical metrics: the
// tallest ascent, deepest descent, and largest line-height across the per-glyph active
// styles (base stack, or a <font size=...> / mapped-font override resolved the same way
// as drawing). Used so a line's baseline, its line-to-line advance, and the measured
// text box all follow the tallest style present on the line -- keeping large size spans
// inside the reported bounds and stopping successive lines from colliding. Results are
// in logical units (each face's metric is pre-scaled by its own pixel-height scale).
static void s_measure_line_vmetrics(CF_ParsedTextState* text_state, bool do_effects, const char* base_font_name, CF_Font* base_font, float base_font_size, const char* text, int start_index, const char* end_of_line, float* out_ascent, float* out_descent, float* out_line_height)
{
	float base_scale = stbtt_ScaleForPixelHeight(&base_font->info, base_font_size);
	float max_ascent = base_font->ascent * base_scale;
	float min_descent = base_font->descent * base_scale; // Descent is negative; smaller == deeper.
	float max_line_height = base_font->line_height * base_scale;

	int index = start_index;
	while (*text && text < end_of_line) {
		int cp;
		text = cf_string_decode_UTF8(text, &cp);
		if (cp == '\n') break;

		const char* font_name = base_font_name;
		float font_size = base_font_size;
		if (do_effects) s_resolve_text_style(text_state, index, &font_name, &font_size);
		CF_Font* font = cf_font_get(font_name);
		if (!font) font = base_font;

		float s = stbtt_ScaleForPixelHeight(&font->info, font_size);
		max_ascent = max(max_ascent, font->ascent * s);
		min_descent = min(min_descent, font->descent * s);
		max_line_height = max(max_line_height, font->line_height * s);
		++index;
	}

	*out_ascent = max_ascent;
	*out_descent = min_descent;
	*out_line_height = max_line_height;
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
	int next(bool trim = true) { if (trim) ltrim(); int cp; in = cf_string_decode_UTF8(in, &cp); return cp; }
	int peek(bool trim = true) { if (trim) ltrim(); int cp; cf_string_decode_UTF8(in, &cp); return cp; }
	void skip(bool trim = true) { if (trim) ltrim(); int cp; in = cf_string_decode_UTF8(in, &cp); }
	bool expect(int ch) { int cp = next(); if (cp != ch) { return false; } return true; }
	bool try_next(int ch, bool trim = true) { if (trim) ltrim(); int cp; const char* next = cf_string_decode_UTF8(in, &cp); if (cp == ch) { in = next; return true; } return false; }
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
	while (!s->done()) {
		int cp = s->peek(false);
		if (!s_is_hex_alphanum(cp)) {
			break;
		} else {
			string.append(cp);
			s->skip(false);
		}
	}
	uint32_t rgba = 0x000000FF;
	if (!string.empty()) {
		rgba = (uint32_t)string.to_hex();
	}
	CF_Color result = cf_make_color_rgba(
		(uint8_t)((rgba >> 24) & 0xFF),
		(uint8_t)((rgba >> 16) & 0xFF),
		(uint8_t)((rgba >> 8) & 0xFF),
		(uint8_t)(rgba & 0xFF)
	);
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
		// A nameless tag (e.g. "<>") yields an empty String whose c_str() is NULL;
		// sintern(NULL) would crash, so intern only a real name (matches s_parse_code_val).
		String name_str = s_parse_code_name(s);
		const char* name = !name_str.empty() ? sintern(name_str.c_str()) : NULL;
		if (first) {
			first = false;
			code.effect_name = name;
			code.fn = NULL;
			CF_TextEffectDef* def = app->text_effect_defs.try_find(name);
			if (def) code.fn = def->fn;
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
		float h = fx->font_size / 20.0f;
		h = (float)fx->get_number("strike", (double)h);
		fx->strike_thickness = h;
	}
	return true;
}

static bool s_text_fx_underline(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;
	if (!s_is_space(fx->character) || fx->character == ' ') {
		float h = fx->font_size / 20.0f;
		h = (float)fx->get_number("underline", (double)h);
		fx->underline_thickness = h;
	}
	return true;
}

static CF_Color s_gradient_corner(TextEffect* fx, const char* corner, const char* edge_a, const char* edge_b)
{
	CF_Color c = fx->color;
	if (fx->has(corner)) return fx->get_color(corner, c);
	bool a = fx->has(edge_a);
	bool b = fx->has(edge_b);
	if (a && b) return cf_color_lerp(fx->get_color(edge_a, c), fx->get_color(edge_b, c), 0.5f);
	if (a) return fx->get_color(edge_a, c);
	if (b) return fx->get_color(edge_b, c);
	return c;
}

static bool s_text_fx_gradient(CF_TextEffect* fx_ptr)
{
	TextEffect* fx = (TextEffect*)fx_ptr;

	CF_Color tl = s_gradient_corner(fx, "topleft",     "top",    "left");
	CF_Color tr = s_gradient_corner(fx, "topright",    "top",    "right");
	CF_Color br = s_gradient_corner(fx, "bottomright", "bottom", "right");
	CF_Color bl = s_gradient_corner(fx, "bottomleft",  "bottom", "left");

	float n = (float)fx->glyph_count;
	float t_left = (float)fx->index_into_effect / n;
	float t_right = (float)(fx->index_into_effect + 1) / n;

	fx->use_colors = true;
	fx->colors[0] = cf_color_lerp(tl, tr, t_left);   // glyph TL
	fx->colors[1] = cf_color_lerp(tl, tr, t_right);  // glyph TR
	fx->colors[2] = cf_color_lerp(bl, br, t_right);  // glyph BR
	fx->colors[3] = cf_color_lerp(bl, br, t_left);   // glyph BL
	return true;
}

// Font/size for the built-in <font> effect are applied during layout (see
// s_resolve_text_style); the callback itself is a no-op so the markup is valid.
static bool s_text_fx_font(CF_TextEffect* fx_ptr)
{
	CF_UNUSED(fx_ptr);
	return true;
}

static bool s_text_fx_stub(CF_TextEffect* fx_ptr)
{
	CF_UNUSED(fx_ptr);
	return true;
}

static CF_TextEffectDef* s_get_or_create_text_effect_def(const char* name)
{
	name = sintern(name);
	CF_TextEffectDef* def = app->text_effect_defs.try_find(name);
	if (!def) {
		def = app->text_effect_defs.insert(name);
	}
	return def;
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
		text_effect_register("underline", s_text_fx_underline);
		text_effect_register("gradient", s_text_fx_gradient);
		text_effect_register("font", s_text_fx_font);
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
	// Sort by start index, then by open order so nested spans that share a start
	// index still apply outer→inner (last override wins in s_resolve_text_style).
	std::stable_sort(text_state->codes.begin(), text_state->codes.end(),
		[](const CF_TextCode& a, const CF_TextCode& b) {
			if (a.index_in_string != b.index_in_string) {
				return a.index_in_string < b.index_in_string;
			}
			return a.parse_order < b.parse_order;
		}
	);
	text_state->sanitized = s->sanitized;
}

static v2 s_draw_text(const char* text, CF_V2 position, int text_length, bool render, cf_text_markup_info_fn* markups)
{
	const char* base_font_name = s_draw->fonts.last();
	CF_Font* base_font = cf_font_get(base_font_name);
	CF_ASSERT(base_font);
	if (!base_font) return V2(0,0);

	// Text id can be custom or based on text's content
	uint64_t text_id = s_draw->text_ids.last();
	uint64_t text_hash = fnv1a(text, (int)CF_STRLEN(text) + 1);
	if (text_id == 0) { text_id = text_hash; }

	// Effect state is key'd by text id
	CF_TextEffectState* effect_state = app->text_effect_states.try_find(text_id);
	if (!effect_state) {
		effect_state = app->text_effect_states.insert(text_id);
	}

	// Text state is key'd by text's content
	CF_ParsedTextState* text_state = app->parsed_text_states.try_find(text_hash);
	if (!text_state) {
		text_state = app->parsed_text_states.insert(text_hash);
		s_parse_codes(text_state, text);
	}

	if (render || markups) {
		if (!effect_state->alive) {
			effect_state->elapsed += CF_DELTA_TIME;
		}
		effect_state->alive = true;
		text_state->alive = true;
	}

	// Use the sanitized string for rendering. This excludes all text codes.
	bool do_effects = s_draw->text_effects.last();
	if (do_effects) {
		text = text_state->sanitized.c_str();
	}

	// Gather up all state required for rendering.
	// Base font/size seed each line's metrics; the active line then adopts the tallest
	// per-glyph style on it (see s_measure_line_vmetrics) so large <font size=...> spans
	// lower the baseline, grow the advance, and stay inside the measured box.
	// Per-glyph font/size may differ via text-effect font overrides (see s_resolve_text_style).
	float base_font_size = s_draw->font_sizes.last();
	int blur = s_draw->blurs.last();
	float wrap_w = s_draw->text_wrap_widths.last();
	float scale = stbtt_ScaleForPixelHeight(&base_font->info, base_font_size);
	float line_height = base_font->line_height * scale;
	// Vertical metrics for the line currently being laid out. cur_line_height is the
	// finishing line's advance; line_extra_ascent lowers the baseline by any ascent past
	// the base face. Both default to the base face (used for empty lines with no glyphs).
	float cur_line_height = line_height;
	float line_extra_ascent = 0;
	int cp_prev = 0;
	int cp = 0;
	const char* end_of_line = NULL;
	float h = (base_font->ascent + base_font->descent) * scale;
	float w = base_font->width * scale;
	const char* prev_glyph_font_name = NULL;
	float prev_glyph_font_size = 0;

	// @NOTE -- Not 100% sure snapping to pixel is the best thing here, but it really does make
	// text rendering feel a lot more robust, especially for nearest-neighbor rendering. Snap on
	// the physical-pixel grid (not the logical-point grid) so HiDPI text doesn't jitter relative
	// to unsnapped shapes -- at pixel_scale 2 a logical-point snap only lands on even device pixels.
	float pixel_scale = app->pixel_scale;
	float x = CF_ROUNDF(position.x * pixel_scale) / pixel_scale;
	float initial_y = CF_ROUNDF((position.y - base_font->ascent * scale) * pixel_scale) / pixel_scale;
	float y = initial_y;
	float max_x = x;
	// Extend the height by descent to include spaces below the baseline.
	// e.g: Characters such as "g", "y"...
	float min_y = y + base_font->descent * scale;

	int index = 0;
	int code_index = 0;
	int newline_count = 0;

	// Called whenever text-effects need to be spawned, before going to the next glyph.
	auto effect_spawn = [&]() {
		while (code_index < text_state->codes.count()) {
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
			} else {
				break;
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
		text = cf_string_decode_UTF8(text, &cp);
		effect_cleanup();
		++index;
	};

	bool vertical = s_draw->vertical.last();

	auto advance_to_next_glyph = [&](CF_Glyph* last_glyph, float advance) {
		// Max bound covers the entire glyph without kerning so we use the glyph's
		// logical ink width (q1.x - q0.x) instead of xadvance. Note glyph->w is the
		// *physical* rasterized bitmap width (scaled by pixel_scale, used for the
		// sprite's source-texture size) -- not the same unit as the logical `x`
		// cursor here, so it can't be used directly for this bound.
		max_x = max(max_x, x + (last_glyph->q1.x - last_glyph->q0.x));
		if (vertical) {
			min_y = min(min_y, y + base_font->descent * scale);

			y -= line_height;
		} else {
			// Prefer the (possibly effect-modified) advance so layout matches rendering.
			x += advance;
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
			y -= cur_line_height;

			// Base-descent floor for the new line; the line's own scan lowers min_y
			// further if it turns out taller. Also covers a trailing empty line.
			min_y = min(min_y, y + base_font->descent * scale);
		}
		hit_newline = true;
		prev_glyph_font_name = NULL;
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
		text = cf_string_decode_UTF8(text, &cp);
		++index;
		CF_DEFER(effect_cleanup());

		if (cp == '\n') {
			// Force the next line to recompute its boundary + vertical metrics.
			end_of_line = NULL;
			apply_newline();
			continue;
		}

		// Word wrapping logic. Uses the same per-glyph font/size resolution as drawing
		// so bold/italic (or any font override) wraps at the correct visual width.
		if (!end_of_line) {
			end_of_line = s_find_end_of_line(text_state, do_effects, prev_text, index - 1, wrap_w);

			// Establish this visual line's vertical metrics from the tallest active style
			// on it, then lower the baseline by any extra ascent so large size spans fit
			// under the previous line. min_y is extended for the line's deepest descent so
			// measurement covers it. Horizontal only; vertical text keeps base-face columns.
			if (!vertical) {
				float line_ascent, line_descent, line_line_height;
				s_measure_line_vmetrics(text_state, do_effects, base_font_name, base_font, base_font_size, prev_text, index - 1, end_of_line, &line_ascent, &line_descent, &line_line_height);
				cur_line_height = line_line_height;
				line_extra_ascent = line_ascent - base_font->ascent * scale;
				min_y = min(min_y, (y - line_extra_ascent) + line_descent);
			}
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

		// Active face for this glyph (base stack, or a text-effect font override).
		const char* active_font_name = base_font_name;
		float active_font_size = base_font_size;
		if (do_effects) {
			s_resolve_text_style(text_state, index - 1, &active_font_name, &active_font_size);
		}
		CF_Font* active_font = cf_font_get(active_font_name);
		if (!active_font) active_font = base_font;

		atlas_cache_entry_t s = { };
		// Text stages its geometry locally instead of filling in place: user text-effect
		// callbacks run mid-glyph and may themselves draw (which would reallocate the
		// geoms array under a live reference), and visibility is only final after
		// effects run -- only visible glyphs append to the stream.
		BatchGeometry g = { };
		g.blend = s_draw->blends.last();
		s.minx = 0; // 9-slice implementation expects these to be defaulted to 0..1.
		s.miny = 0;
		s.maxx = 1;
		s.maxy = 1;
		CF_Glyph* glyph = cf_font_get_glyph(active_font, cp, active_font_size, blur);
		if (!glyph) {
			continue;
		}

		// Advance the cursor by the pair kern so it accumulates across the line and is
		// reflected in measurement + bounds, not just the visible glyph. Horizontal only;
		// hit_newline guards against kerning across line breaks. Skip kerning across a
		// font or size change -- pair kern tables are face-specific.
		if (!vertical && !hit_newline && cp_prev && prev_glyph_font_name == active_font_name && prev_glyph_font_size == active_font_size) {
			x += cf_font_get_kern(active_font, active_font_size, cp_prev, cp);
		}

		// Prepare a sprite struct for rendering.
		float xadvance = glyph->xadvance;
		// This line's baseline, lowered by any ascent past the base face so a taller
		// span sits under the previous line instead of overflowing above it. Equal to y
		// for base-height lines, so ordinary text is unaffected.
		float baseline_y = y - line_extra_ascent;
		if (render || markups) {
			bool visible = glyph->visible;
			s.image_id = glyph->image_id;
			s.w = glyph->w;
			s.h = glyph->h;
			g.type = BATCH_GEOMETRY_TYPE_SPRITE;
			g.alpha = 1.0f;
			CF_Color color = s_draw->colors.last();

			// Account for atlas_cache's 1-pixel atlas border. The border is 1 *device*
			// pixel (glyph bitmaps are rasterized at pixel_scale resolution), but q0/q1
			// are in logical units, so the pad must shrink by pixel_scale to match --
			// otherwise on HiDPI the quad is stretched past the glyph's actual coverage.
			v2 pad = V2(1,1) / app->pixel_scale;
			v2 q0 = glyph->q0 + V2(x,baseline_y) - pad;
			v2 q1 = glyph->q1 + V2(x,baseline_y) + pad;

			// Curve text path (the default; a pushed stroke also forces it): fetch the
			// outline strip up front so layout stays identical (metrics still come from
			// the rasterized glyph) and only rendering swaps. Blurred text keeps the
			// raster path -- blur is a bitmap-space effect.
			CF_CurveGlyph* cg = NULL;
			if ((s_draw->text_curves.last() || s_draw->text_strokes.last() > 0) && visible && blur == 0) {
				cg = cf_font_get_glyph_curves(active_font, cp);
				if (cg->curve_count == 0) cg = NULL; // No outline, or over strip budget: atlas path.
			}
			v2 pre_q0 = q0;
			v2 pre_q1 = q1;

			// Apply any active custom text effects.
			bool use_corner_colors = false;
			CF_Color corner_colors[4] = {};
			float pre_fx_q0_y = q0.y;
			for (int i = 0; i < text_state->effects.count();) {
				TextEffect* effect = text_state->effects + i;
				CF_TextEffectFn* fn = effect->fn;
				bool keep_going = true;
				if (fn) {
					effect->text_without_markups = text_state->sanitized.c_str();
					effect->character = cp;
					effect->index_into_effect = index - effect->index_into_string - 1;
					effect->center = V2(x + xadvance*0.5f, baseline_y + h*0.25f);
					effect->q0 = q0;
					effect->q1 = q1;
					effect->w = s.w;
					effect->h = s.h;
					effect->color = color;
					effect->use_colors = false;
					effect->opacity = g.alpha;
					effect->xadvance = xadvance;
					effect->visible = visible;
					effect->font_size = active_font_size;
					effect->on_begin = effect->on_start();
					keep_going = fn(effect);
					q0 = effect->q0;
					q1 = effect->q1;
					color = effect->color;
					if (effect->use_colors) {
						use_corner_colors = true;
						for (int j = 0; j < 4; ++j) corner_colors[j] = effect->colors[j];
						g.text_colors[0] = premultiply(effect->colors[0]);
						g.text_colors[1] = premultiply(effect->colors[1]);
						g.text_colors[2] = premultiply(effect->colors[2]);
						g.text_colors[3] = premultiply(effect->colors[3]);
					}
					g.alpha = effect->opacity;
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

			// Collect deferred strikes/underlines using final position/color. Both ride
			// the same CF_Strike line machinery; only the height differs (mid-height vs
			// just under the baseline).
			for (int i = 0; i < text_state->effects.count(); ++i) {
				TextEffect* effect = text_state->effects + i;
				CF_Color line_color = use_corner_colors
					? cf_color_lerp(cf_color_lerp(corner_colors[0], corner_colors[1], 0.5f), cf_color_lerp(corner_colors[3], corner_colors[2], 0.5f), 0.5f)
					: color;
				// Line heights come from the active face so <font>/<size> spans place
				// correctly: strikes run through the lowercase center (half x-height),
				// underlines sit a fraction of the descent below the baseline.
				float line_scale = stbtt_ScaleForPixelHeight(&active_font->info, active_font_size);
				if (effect->strike_thickness > 0) {
					float strike_up = active_font->x_height > 0 ? active_font->x_height * line_scale * 0.5f : h * 0.25f;
					float y_mid = baseline_y + strike_up + (q0.y - pre_fx_q0_y);
					CF_Strike strike;
					strike.p0 = V2(x, y_mid);
					strike.p1 = V2(x + xadvance, y_mid);
					strike.thickness = effect->strike_thickness;
					strike.color = line_color;
					strike.color.a *= g.alpha;
					s_draw->strikes.add(strike);
					effect->strike_thickness = 0;
				}
				if (effect->underline_thickness > 0) {
					float y_under = baseline_y + active_font->descent * line_scale * 0.35f + (q0.y - pre_fx_q0_y);
					CF_Strike strike;
					strike.p0 = V2(x, y_under);
					strike.p1 = V2(x + xadvance, y_under);
					strike.thickness = effect->underline_thickness;
					strike.color = line_color;
					strike.color.a *= g.alpha;
					s_draw->underlines.add(strike);
					effect->underline_thickness = 0;
				}
			}

			// Actually render the sprite.
			if (visible && render) {
				CF_M3x2 m = s_draw->mvp;
				if (cg) {
					// The outline box sits at fixed fractions of the unmodified glyph
					// quad; mapping those fractions through the effect-modified quad
					// carries wave/scale text effects over to the curve path.
					float su = stbtt_ScaleForPixelHeight(&active_font->info, active_font_size);
					v2 o0 = V2(x, baseline_y) + cg->box_min * su;
					v2 o1 = V2(x, baseline_y) + cg->box_max * su;
					v2 inv = V2(1.0f / (pre_q1.x - pre_q0.x), 1.0f / (pre_q1.y - pre_q0.y));
					v2 f0 = V2((o0.x - pre_q0.x) * inv.x, (o0.y - pre_q0.y) * inv.y);
					v2 f1 = V2((o1.x - pre_q0.x) * inv.x, (o1.y - pre_q0.y) * inv.y);
					v2 e = q1 - q0;
					o0 = V2(q0.x + f0.x * e.x, q0.y + f0.y * e.y);
					o1 = V2(q0.x + f1.x * e.x, q0.y + f1.y * e.y);
					float aaf = s_draw->aaf;
					float text_stroke = s_draw->text_strokes.last();
					g.type = BATCH_GEOMETRY_TYPE_GLYPH;
					g.n = cg->curve_count;
					g.shape[0] = V2(o0.x, o1.y);
					g.shape[1] = V2(o1.x, o1.y);
					g.shape[2] = V2(o1.x, o0.y);
					g.shape[3] = V2(o0.x, o0.y);
					float cpad = text_stroke + aaf;
					g.box[0] = V2(o0.x - cpad, o1.y + cpad);
					g.box[1] = V2(o1.x + cpad, o1.y + cpad);
					g.box[2] = V2(o1.x + cpad, o0.y - cpad);
					g.box[3] = V2(o0.x - cpad, o0.y - cpad);
					g.stroke = text_stroke;
					g.fill = text_stroke <= 0;
					g.aa = aaf;
					s.image_id = cg->image_id;
					s.w = cg->strip_w;
					s.h = 1;
				} else {
					g.shape[0] = V2(q0.x, q1.y);
					g.shape[1] = V2(q1.x, q1.y);
					g.shape[2] = V2(q1.x, q0.y);
					g.shape[3] = V2(q0.x, q0.y);
				}
				g.color = premultiply(color);
				if (!use_corner_colors) {
					CF_Color flat = g.color;
					for (int j = 0; j < 4; ++j) g.text_colors[j] = flat;
				}
				g.is_text = true;
				BatchGeometry& pushed = s_push_geom();
				CF_M3x2 mvp = pushed.mvp;
				pushed = g;
				pushed.mvp = mvp;
				DRAW_PUSH_ITEM(s);
			}
		}

		advance_to_next_glyph(glyph, xadvance);
		prev_glyph_font_name = active_font_name;
		prev_glyph_font_size = active_font_size;
		hit_newline = false;
	}

	// If text_length is less than the sanitized length, there may be some left
	// over effect that is not cleaned up
	text_state->effects.clear();

	if (render) {
		// Draw strike/underline lines just after the text, merged into polyline runs.
		s_draw_strike_lines(s_draw->strikes);
		s_draw_strike_lines(s_draw->underlines);
	}
	s_draw->strikes.clear();
	s_draw->underlines.clear();

	return V2(max_x - position.x, position.y - min_y);
}

static void s_get_text_without_markups(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	s_text_without_markups = fx->text_without_markups;
}

void cf_draw_text(const char* text, CF_V2 position, int text_length)
{
	s_draw_text(text, position, text_length);
}

void cf_text_effect_register(const char* name, CF_TextEffectFn* fn)
{
	CF_TextEffectDef* def = s_get_or_create_text_effect_def(name);
	def->fn = fn;
}

void cf_text_effect_set_font(const char* effect_name, const char* font_name)
{
	CF_TextEffectDef* def = s_get_or_create_text_effect_def(effect_name);
	def->font_name = sintern(font_name);
	// Auto-register a no-op callback so the markup is valid without a custom effect.
	if (!def->fn) {
		def->fn = s_text_fx_stub;
	}
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

const char* cf_text_without_markups(const char* text)
{
	s_text_without_markups = NULL;
	s_draw_text(text, V2(0,0), -1, false, s_get_text_without_markups);
	// The callback may not be invoked if there is no markup
	return s_text_without_markups != NULL ? s_text_without_markups : text;
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
	return s_draw->layers.last();
}

void cf_draw_push_color(CF_Color c)
{
	s_draw->colors.add(c);
}

CF_Color cf_draw_pop_color()
{
	if (s_draw->colors.count() > 1) {
		return s_draw->colors.pop();
	} else {
		return s_draw->colors.last();
	}
}

CF_Color cf_draw_peek_color()
{
	return s_draw->colors.last();
}

void cf_draw_push_shape_aa(float aa)
{
	s_draw->antialias.add(aa);
	s_draw->set_aaf();
}

float cf_draw_pop_shape_aa()
{
	if (s_draw->antialias.count() > 1) {
		float result = s_draw->antialias.pop();
		s_draw->set_aaf();
		return result;
	} else {
		return s_draw->antialias.last();
	}
}

float cf_draw_peek_shape_aa()
{
	return s_draw->antialias.last();
}

void cf_draw_push_vertex_attributes(float r, float g, float b, float a)
{
	s_draw->user_params.add(cf_make_color_rgba_f(r, g, b, a));
}

void cf_draw_push_vertex_attributes2(CF_Color attributes)
{
	s_draw->user_params.add(attributes);
}

CF_Color cf_draw_pop_vertex_attributes()
{
	return s_draw->user_params.count() > 1 ? s_draw->user_params.pop() : s_draw->user_params.last();
}

CF_Color cf_draw_peek_vertex_attributes()
{
	return s_draw->user_params.last();
}

void cf_draw_push_tri_colors(CF_Color c0, CF_Color c1, CF_Color c2)
{
	s_draw->tri_colors0.add(c0);
	s_draw->tri_colors1.add(c1);
	s_draw->tri_colors2.add(c2);
}

void cf_draw_pop_tri_colors()
{
	if (s_draw->tri_colors0.count() > 1) {
		s_draw->tri_colors0.pop();
		s_draw->tri_colors1.pop();
		s_draw->tri_colors2.pop();
	}
}

void cf_draw_peek_tri_colors(CF_Color* c0, CF_Color* c1, CF_Color* c2)
{
	if (c0) *c0 = s_draw->tri_colors0.last();
	if (c1) *c1 = s_draw->tri_colors1.last();
	if (c2) *c2 = s_draw->tri_colors2.last();
}

void cf_draw_push_tri_attributes(CF_Color a0, CF_Color a1, CF_Color a2)
{
	s_draw->tri_attributes0.add(a0);
	s_draw->tri_attributes1.add(a1);
	s_draw->tri_attributes2.add(a2);
}

void cf_draw_pop_tri_attributes()
{
	if (s_draw->tri_attributes0.count() > 1) {
		s_draw->tri_attributes0.pop();
		s_draw->tri_attributes1.pop();
		s_draw->tri_attributes2.pop();
	}
}

void cf_draw_peek_tri_attributes(CF_Color* a0, CF_Color* a1, CF_Color* a2)
{
	if (a0) *a0 = s_draw->tri_attributes0.last();
	if (a1) *a1 = s_draw->tri_attributes1.last();
	if (a2) *a2 = s_draw->tri_attributes2.last();
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
	return s_draw->viewports.last();
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
	return s_draw->scissors.last();
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
	return s_draw->render_states.last();
}

void cf_draw_set_atlas_dimensions(int width_in_pixels, int height_in_pixels)
{
	atlas_cache_term(&s_draw->atlas_cache);
	s_init_atlas_cache(width_in_pixels, height_in_pixels);
	s_draw->atlas_dims.x = (float)width_in_pixels;
	s_draw->atlas_dims.y = (float)height_in_pixels;
	s_draw->texel_dims.x = 1.0f / s_draw->atlas_dims.x;
	s_draw->texel_dims.y = 1.0f / s_draw->atlas_dims.y;
}

CF_Shader cf_make_draw_shader(const char* path)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_internal(path);
	CF_Shader draw_shd = cf_make_draw_shader_internal(path);
	s_draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	if (draw_shd.id) s_draw->shader_paths.add(draw_shd.id, sintern(path));
	return draw_shd;
}

CF_Shader cf_make_draw_shader_from_source(const char* src)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_from_source_internal(src);
	CF_Shader draw_shd = cf_make_draw_shader_from_source_internal(src);
	s_draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	return draw_shd;
}

CF_Shader cf_make_draw_shader_from_bytecode(CF_DrawShaderBytecode bytecode)
{
	// Also make an attached blit shader to apply when drawing canvases.
	CF_Shader blit_shd = cf_make_draw_blit_shader_from_bytecode_internal(bytecode.blit_shader);
	CF_Shader draw_shd = cf_make_draw_shader_from_bytecode_internal(bytecode.draw_shader);
	s_draw->draw_shd_to_blit_shd.add(draw_shd.id, blit_shd.id);
	return draw_shd;
}

bool cf_shader_reload(CF_Shader* shader)
{
	const char** path_ptr = s_draw->shader_paths.try_find(shader->id);
	if (!path_ptr) return false;
	const char* path = *path_ptr;

	CF_Shader new_shd = cf_make_draw_shader(path);
	if (!new_shd.id) return false;

	cf_destroy_shader(*shader);
	*shader = new_shd;
	return true;
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
	return s_draw->shaders.last();
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
	return s_draw->alpha_discards.last() == 0 ? false : true;
}

void cf_draw_push_filter(CF_DrawFilterMode filter_mode)
{
	PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(filter_mode);
}

static CF_DrawFilterMode s_pop_filter_mode()
{
	POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(filter_mode);
}

CF_DrawFilterMode cf_draw_pop_filter()
{
	return s_pop_filter_mode();
}

CF_DrawFilterMode cf_draw_peek_filter()
{
	return s_draw->filter_modes.last();
}

// Blend modes are recorded per drawable (no command split at record time); the flush
// splits paint-ordered runs at mode changes and applies each run's exact fixed-function
// state, so ordering holds across mode changes and canvas compositing stays correct.
void cf_draw_push_blend(CF_DrawBlend blend)
{
	s_draw->blends.add((int)blend);
}

CF_DrawBlend cf_draw_pop_blend()
{
	if (s_draw->blends.count() > 1) {
		return (CF_DrawBlend)s_draw->blends.pop();
	} else {
		return (CF_DrawBlend)s_draw->blends.last();
	}
}

CF_DrawBlend cf_draw_peek_blend()
{
	return (CF_DrawBlend)s_draw->blends.last();
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
	u.data = cf_arena_alloc(&s_draw->uniform_arena, u.size);
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
	u.data = cf_arena_alloc(&s_draw->uniform_arena, u.size);
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
	u.data = cf_arena_alloc(&s_draw->uniform_arena, u.size);
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
	u.data = cf_arena_alloc(&s_draw->uniform_arena, u.size);
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
	u.data = cf_arena_alloc(&s_draw->uniform_arena, u.size);
	CF_ASSERT(u.size == sizeof(val));
	CF_MEMCPY(u.data, &val, u.size);
	ADD_UNIFORM(u);
}

void cf_draw_canvas(CF_Canvas canvas, CF_V2 position, CF_V2 scale)
{
	CF_Command& cmd = s_draw->add_cmd();
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
		CF_MUL_M32_V2(cmd.canvas_verts_posH[i], s_draw->mvp, cmd.canvas_verts[i]);
	}
	cmd.canvas_attributes = s_draw->user_params.last();

	// Ensure subsequent draw items don't land on this canvas command.
	// Without this, any draws after cf_draw_canvas that don't trigger a new
	// command (e.g. no layer/shader/state change) silently append to the canvas
	// command's items array, which s_process_command ignores for canvas blits.
	s_draw->add_cmd();
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

	if (!s_draw->blit_init) {
		s_draw->blit_init = true;

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
		s_draw->blit_mesh = blit_mesh;
	}

	// Try and fetch a custom shader supplied by the user, otherwise fallback to the default blit shader.
	CF_Shader* blit = (CF_Shader*)s_draw->draw_shd_to_blit_shd.try_get(cmd->shader.id);
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
		verts[i].params = cmd->canvas_attributes;
	}
	verts[0].uv = V2(0,1);
	verts[1].uv = V2(1,1);
	verts[2].uv = V2(0,0);
	verts[3].uv = V2(1,1);
	verts[4].uv = V2(1,0);
	verts[5].uv = V2(0,0);

	cf_mesh_update_vertex_data(s_draw->blit_mesh, verts, 6);
	cf_apply_mesh(s_draw->blit_mesh);

	// Read pixels from src.
	cf_material_set_texture_fs(s_draw->material, "u_image", cf_canvas_get_target(src));

	// Apply uniforms.
	int w, h;
	cf_canvas_get_size(cmd->canvas, &w, &h);
	v2 canvas_dims = V2((float)w, (float)h);
	cf_material_set_uniform_fs(s_draw->material, "u_texture_size", &canvas_dims, CF_UNIFORM_TYPE_FLOAT2, 1);
	int alpha_discard = cmd->alpha_discard == 0.0f ? 0 : 1;
	cf_material_set_uniform_fs(s_draw->material, "u_alpha_discard", &alpha_discard, CF_UNIFORM_TYPE_INT, 1);
	// u_use_smooth_uv: 0 = apply shader smooth_uv function, 1 = use plain v_uv (hardware filtering only)
	int use_smooth_uv = cmd->filter_mode == CF_DRAW_FILTER_SMOOTH ? 0 : 1;
	cf_material_set_uniform_fs(s_draw->material, "u_use_smooth_uv", &use_smooth_uv, CF_UNIFORM_TYPE_INT, 1);

	// Apply render state.
	cf_material_set_render_state(s_draw->material, cmd->render_state);

	// Set sampler filter based on filter mode.
	void* sampler_override = (cmd->filter_mode == CF_DRAW_FILTER_NEAREST) ? s_draw->sampler_nearest : s_draw->sampler_linear;
	cf_set_sampler_override(sampler_override);

	// Apply shader.
	cf_apply_shader(*blit, s_draw->material);

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

static void s_draw_report_range(const BatchGeometry* geoms, const CF_PendingUV* uvs, int start, int end, uint64_t texture_id, int texture_w, int texture_h);

// Runs after every atlas_cache_flush (which filled the per-flush uv table via the
// callbacks): render the collated stream in paint order, splitting into a new draw
// wherever the bound atlas texture changes. Shapes are texture-agnostic and ride
// whichever run they fall in.
static void s_flush_pending_geoms()
{
	const BatchGeometry* geoms = s_draw->pending_geoms.data();
	const CF_PendingUV* uvs = s_draw->pending_uvs.data();
	int n = s_draw->pending_geoms.count();
	int start = 0;
	uint64_t run_tex = 0;
	int run_w = 1, run_h = 1;
	int run_blend = n ? geoms[0].blend : 0;
	for (int i = 0; i < n; ++i) {
		const BatchGeometry& g = geoms[i];
		if (g.csg_operand) continue; // Rides with its CSG head.
		// Blend mode changes split the stream: each run renders with its mode's exact
		// fixed-function canvas state, and run sequencing preserves paint order.
		if (g.blend != run_blend) {
			s_draw_report_range(geoms, uvs, start, i, run_tex, run_w, run_h, run_blend);
			start = i;
			run_tex = 0;
			run_w = run_h = 1;
			run_blend = g.blend;
		}
		if (!(g.is_sprite || g.is_text)) continue;
		if (uvs[i].texture_id == 0) continue;
		if (run_tex == 0) {
			run_tex = uvs[i].texture_id;
			run_w = uvs[i].tex_w;
			run_h = uvs[i].tex_h;
		} else if (uvs[i].texture_id != run_tex) {
			s_draw_report_range(geoms, uvs, start, i, run_tex, run_w, run_h, run_blend);
			start = i;
			run_tex = uvs[i].texture_id;
			run_w = uvs[i].tex_w;
			run_h = uvs[i].tex_h;
		}
	}
	s_draw_report_range(geoms, uvs, start, n, run_tex, run_w, run_h, run_blend);
	s_draw->pending_geoms.clear();
	s_draw->pending_uvs.clear();
}

static void s_process_command(CF_Canvas canvas, CF_Command* cmd, CF_Command* next, bool& clear)
{
	if (cmd->processed) return;
	cmd->processed = true;

	// Apply uniforms.
	CF_DrawUniform* u = &cmd->u;
	if (u->is_texture) {
		material_set_texture_fs(s_draw->material, u->name, u->texture);
	} else if (u->data) {
		cf_material_set_uniform_fs_internal(s_draw->material, "shd_uniforms", u->name, u->data, u->type, u->array_length);
	}

	// Blit canvas.
	// ...Incurs an entire extra draw call by itself.
	if (cmd->is_canvas) {
		// Flush any accumulated geometry before the blit.
		if (s_draw->need_flush) {
			s_draw->need_flush = false;
			if (!s_draw->delay_defrag) {
				atlas_cache_defrag(&s_draw->atlas_cache);
			}
			atlas_cache_flush(&s_draw->atlas_cache);
			s_flush_pending_geoms();
		}
		s_blit(cmd, cmd->canvas, canvas, clear);
		clear = false; // Only clear `canvas` once.
		s_draw->has_drawn_something = true;
		return;
	}

	// Collate the drawable items: all geometry appends to the flush-ordered stream;
	// sprites/text additionally push a small atlas entry to the atlas_cache whose seq
	// is rebased to index the stream (commands were layer-sorted, so the rebase
	// happens here, not at record time). Draw list replays borrow their list's geometry
	// (geoms_ref) and compose the replay transform during this one copy.
	const Cute::Array<BatchGeometry>* src_geoms = cmd->geoms_ref ? cmd->geoms_ref : &cmd->geoms;
	if (src_geoms->count()) {
		s_draw->need_flush = true;
		int base = s_draw->pending_geoms.count();
		for (int i = 0; i < src_geoms->count(); ++i) {
			s_draw->pending_geoms.add((*src_geoms)[i]);
			if (cmd->geoms_ref) {
				BatchGeometry& g = s_draw->pending_geoms.last();
				CF_MUL_M32_M32(g.mvp, cmd->replay_mvp, g.mvp);
				float extra = g.aa * (cmd->replay_aa_scale - 1.0f);
				g.aa *= cmd->replay_aa_scale;
				if (extra > 0) s_replay_inflate_quad(&g, extra);
			}
			CF_PendingUV uv = { 0 };
			s_draw->pending_uvs.add(uv);
		}
		for (int i = 0; i < cmd->items.count(); ++i) {
			atlas_cache_entry_t sp = cmd->items[i];
			sp.udata += (ATLAS_CACHE_U64)base;
			atlas_cache_push(&s_draw->atlas_cache, sp);
		}
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
			next->filter_mode == cmd->filter_mode &&
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

	if (!same && s_draw->need_flush) {
		// Process the collated drawable items. Might get split up into multiple draw calls depending on
		// the atlas compiler.
		s_draw->need_flush = false;
		if (!s_draw->delay_defrag) {
			atlas_cache_defrag(&s_draw->atlas_cache);
		}
		atlas_cache_flush(&s_draw->atlas_cache);
		s_flush_pending_geoms();
	}
}

void cf_render_layers_to(CF_Canvas canvas, int layer_lo, int layer_hi, bool clear)
{
	// We will render to this canvas.
	cf_apply_canvas(canvas, clear);

	// Uniform-only commands (no geometry, not canvas blits) inherit the layer of their
	// next draw command. This keeps set_texture/set_uniform grouped with the draw_sprite
	// calls that depend on them through the layer sort.
	{
		int next_draw_layer = 0;
		for (int i = s_draw->cmds.count() - 1; i >= 0; i--) {
			CF_Command& cmd = s_draw->cmds[i];
			if (cmd.geoms.count() || cmd.geoms_ref || cmd.is_canvas) {
				next_draw_layer = cmd.layer;
			} else {
				cmd.layer = next_draw_layer;
			}
		}
	}

	// Sort the commands by layer first, then by age (to maintain relative ordering).
	std::stable_sort(s_draw->cmds.begin(), s_draw->cmds.end(), [](const CF_Command& a, const CF_Command& b) {
		if (a.layer == b.layer) return a.id < b.id;
		else return a.layer < b.layer;
	});

	// Process each rendering command.
	int count = s_draw->cmds.count();
	for (int i = 0; i < count; ++i) {
		s_draw->cmd_index = i;
		CF_Command* cmd = &s_draw->cmds[i];
		CF_Command* next = i + 1 == count ? NULL : s_draw->cmds + (i + 1);
		if (cmd->layer >= layer_lo && cmd->layer <= layer_hi) {
			s_process_command(canvas, cmd, next, clear);
		} else if (cmd->layer > layer_hi) {
			break;
		}
	}

	// Reset internal state.
	if (clear && !s_draw->has_drawn_something) {
		cf_clear_canvas(canvas);
	}
	if (s_draw->need_flush) {
		s_draw->need_flush = false;
		if (!s_draw->delay_defrag) {
			atlas_cache_defrag(&s_draw->atlas_cache);
		}
		atlas_cache_flush(&s_draw->atlas_cache);
		s_flush_pending_geoms();
	}
	s_draw->has_drawn_something = false;
	cf_arena_reset(&s_draw->uniform_arena);

	// Remove commands that were processed.
	for (int i = 0; i < s_draw->cmds.size();) {
		if (s_draw->cmds[i].processed) {
			s_draw->cmds.unordered_remove(i);
		} else {
			++i;
		}
	}

	// Ensure there's at least one "default" command for convenience use-cases.
	s_draw->add_cmd();
}

void cf_render_to(CF_Canvas canvas, bool clear)
{
	cf_render_layers_to(canvas, -INT_MAX, INT_MAX, clear);
}

CF_V2 cf_draw_mul(CF_V2 v)
{
	CF_V2 r;
	CF_MUL_M32_V2(r, s_draw->cam_stack.last(), v);
	return r;
}

void cf_draw_transform(CF_M3x2 m)
{
	CF_MUL_M32_M32(m, s_draw->cam_stack.last(), m);
	s_draw->cam_stack.last() = m;
	CF_MUL_M32_M32(s_draw->mvp, s_draw->projection, m);
	s_draw->set_aaf();
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
	CF_M3x2 m;
	CF_MAKE_TSR(m, position, scale, radians);
	CF_MUL_M32_M32(m, s_draw->cam_stack.last(), m);
	s_draw->cam_stack.last() = m;
	CF_MUL_M32_M32(s_draw->mvp, s_draw->projection, m);
	s_draw->set_aaf();
}

void cf_draw_TSR_absolute(CF_V2 position, CF_V2 scale, float radians)
{
	CF_M3x2 m;
	CF_MAKE_TSR(m, position, scale, radians);
	s_draw->cam_stack.last() = m;
	CF_MUL_M32_M32(s_draw->mvp, s_draw->projection, m);
	s_draw->set_aaf();
}

void cf_draw_push()
{
	CF_M3x2 m = s_draw->cam_stack.last();
	s_draw->cam_stack.add(m);
	s_draw->projection_stack.add(s_draw->projection);
}

void cf_draw_pop()
{
	if (s_draw->cam_stack.size() > 1) {
		s_draw->cam_stack.pop();
	}
	if (s_draw->projection_stack.size() > 0) {
		s_draw->projection = s_draw->projection_stack.last();
		s_draw->projection_stack.pop();
	}
	CF_M3x2 m = s_draw->cam_stack.last();
	CF_MUL_M32_M32(s_draw->mvp, s_draw->projection, m);
	s_draw->set_aaf();
}

CF_M3x2 cf_draw_peek()
{
	return s_draw->cam_stack.last();
}

void cf_draw_projection(CF_M3x2 projection)
{
	s_draw->projection = projection;
	CF_MUL_M32_M32(s_draw->mvp, projection, s_draw->cam_stack.last());
}

CF_V2 cf_world_to_screen(CF_V2 CF_V2)
{
	CF_MUL_M32_V2(CF_V2, s_draw->mvp, CF_V2);
	CF_V2.x = (CF_V2.x + 1.0f) * (float)app->w * 0.5f;
	CF_V2.y = (1.0f - CF_V2.y) * (float)app->h * 0.5f;
	return CF_V2;
}

CF_V2 cf_screen_to_world(CF_V2 CF_V2)
{
	CF_V2.x = (CF_V2.x / (float)app->w) * 2.0f - 1.0f;
	CF_V2.y = -((CF_V2.y / (float)app->h) * 2.0f - 1.0f);
	CF_V2 = mul(invert(s_draw->mvp), CF_V2);
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
	s_draw->delay_defrag = true;

	if (sprite->easy_sprite_id >= CF_PREMADE_ID_RANGE_LO && sprite->easy_sprite_id <= CF_PREMADE_ID_RANGE_HI) {
		CF_AtlasSubImage sub_image = s_draw->premade_sub_image_id_to_sub_image.find(sprite->easy_sprite_id);
		atlas_cache_entry_t s = atlas_cache_fetch(&s_draw->atlas_cache, sprite->easy_sprite_id, sprite->w, sprite->h);
		CF_TemporaryImage image;
		image.tex = { sub_image.image_id }; // @JANK - Hijacked to store texture_id and avoid an extra hashtable lookup.
		image.w = sub_image.w;
		image.h = sub_image.h;
		image.u = cf_v2(sub_image.minx, sub_image.miny);
		image.v = cf_v2(sub_image.maxx, sub_image.maxy);
		return image;
	} else {
		uint64_t image_id;
		if (sprite->id != CF_SPRITE_ID_INVALID) {
			image_id = sprite->_image_id;
		} else {
			image_id = sprite->easy_sprite_id;
		}

		atlas_cache_entry_t s = atlas_cache_fetch(&s_draw->atlas_cache, image_id, sprite->w, sprite->h);
		CF_TemporaryImage image;
		image.tex = { s.texture_id };
		image.w = sprite->w;
		image.h = sprite->h;
		v2 inv_dims = V2(1.0f / s_draw->atlas_dims.x, 1.0f / s_draw->atlas_dims.y);
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
	Array<atlas_cache_premade_entry_t> premades;
	for (int i = 0; i < sub_image_count; ++i) {
		atlas_cache_premade_entry_t s = { 0 };
		s.image_id = sub_images[i].image_id + CF_PREMADE_ID_RANGE_LO;
		sub_images[i].image_id = texture.id; // @JANK - Hijack this to store texture_id, and avoid an extra hashtable lookup later in sprite_push.
		s.w = sub_images[i].w;
		s.h = sub_images[i].h;
		s.minx = sub_images[i].minx;
		s.maxx = sub_images[i].maxx;
		s.miny = sub_images[i].miny;
		s.maxy = sub_images[i].maxy;
		premades.add(s);
		s_draw->premade_sub_image_id_to_sub_image.add(s.image_id, sub_images[i]);
	}
	atlas_cache_register_premade_atlas(&s_draw->atlas_cache, texture.id, img.w, img.h, sub_image_count, premades.data());
	image_free(&img);
	return texture;
}

CF_Sprite cf_make_premade_sprite(uint64_t image_id)
{
	image_id = image_id + CF_PREMADE_ID_RANGE_LO;
	CF_AtlasSubImage sub_image = s_draw->premade_sub_image_id_to_sub_image.find(image_id);
	CF_Sprite s = cf_sprite_defaults();
	s.name = "premade_sprite";
	s.easy_sprite_id = image_id;
	s.w = sub_image.w;
	s.h = sub_image.h;
	return s;
}
