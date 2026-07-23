/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DRAW_INTERNAL_H
#define CF_DRAW_INTERNAL_H

#include <cute_array.h>
#include <cute_math.h>
#include <cute_string.h>
#include <cute_draw.h>
#include <cute_graphics.h>

#include <float.h>

extern struct CF_Draw* s_draw;

enum BatchGeometryType : int
{
	BATCH_GEOMETRY_TYPE_TRI,
	BATCH_GEOMETRY_TYPE_TRI_SDF,
	BATCH_GEOMETRY_TYPE_QUAD,
	BATCH_GEOMETRY_TYPE_SPRITE,
	BATCH_GEOMETRY_TYPE_CIRCLE,
	BATCH_GEOMETRY_TYPE_CAPSULE,
	BATCH_GEOMETRY_TYPE_SEGMENT,
	BATCH_GEOMETRY_TYPE_POLYGON,

	// Polyline body: one capsule SDF clipped by hard bisector planes at interior
	// joints. The bisector partition assigns every pixel to exactly one segment
	// (translucent strokes never double-blend), and round joins/caps fall out of the
	// capsule SDF clamping to the joint point -- no joint triangulation.
	// shape[0]=a, shape[1]=b, shape[2]=plane0 n, shape[3]=plane1 n, shape[4]=(d0, d1).
	// Mask keeps dot(n0,p)-d0 < 0 (strict) and dot(n1,p)-d1 <= 0, so the shared
	// boundary belongs to exactly one side. Disabled planes: n=(0,0), d=1.
	BATCH_GEOMETRY_TYPE_SEGMENT_CLIPPED,

	// Directed arrow: capsule shaft unioned with a triangular head in one SDF command
	// (no double-blend at the shaft/head seam). shape[0]=a, shape[1]=b,
	// shape[2]=(shaft radius, head length/half-width).
	BATCH_GEOMETRY_TYPE_ARROW,

	// User-registered SDF (cf_make_custom_shape). shape[0..7] carry the 16 user params
	// verbatim, n is the registry dispatch index, box holds the caller's pre-padded
	// world bounds (also uploaded as payload P4 for the instanced VS coverage quad).
	BATCH_GEOMETRY_TYPE_CUSTOM,

	// CSG shape group head (cf_draw_shape_group_begin/end): one command compositing the
	// `n` operand geoms that immediately follow it in the geometry stream (each tagged
	// csg_operand with its own csg_op/csg_k). box holds the pre-padded union bounds.
	BATCH_GEOMETRY_TYPE_CSG,

	// Curve text glyph (cf_push_text_curves): rendered per-pixel from the glyph's
	// quadratic Bezier outline instead of a rasterized atlas bitmap. The curves live in
	// an atlas strip (see CF_CurveGlyph); the atlas item's uv rect locates it. shape[0..3]
	// hold the outline box's world quad (TL,TR,BR,BL like sprites), n is the curve count,
	// box the padded coverage quad. stroke > 0 renders an outline instead of a fill.
	BATCH_GEOMETRY_TYPE_GLYPH,
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Color color; // Premultiplied. Stays float end-to-end so HDR channels (> 1.0) survive.
	CF_V2 box[4]; // World-space AABB coverage corners.
	int n; // Only needed/used for polygon.
	CF_V2 shape[8];
	float alpha;
	float radius;
	float stroke;
	float aa;
	bool is_text;
	bool is_sprite;
	bool fill;
	bool use_tri_colors;      // Per-vertex colors for triangles.
	bool use_tri_attributes;  // Per-vertex attributes for triangles.
	bool csg_operand;         // Operand of a preceding CSG head; skipped by direct walks.
	int csg_op;               // CF_ShapeOp folding this operand into the running distance.
	float csg_k;              // Smoothing constant for csg_op (0 = hard min/max).
	int blend;                // CF_DrawBlend. Runs split at changes; see s_flush_pending_geoms.
	CF_Color user_params;
	// Text glyphs and raw triangles are mutually exclusive; overlay their extras.
	union {
		struct {
			CF_Color text_colors[4]; // Per-corner: TL, TR, BR, BL (for text glyphs). Premultiplied.
		};
		struct {
			CF_Color tri_colors[3];      // Per-vertex colors (only for BATCH_GEOMETRY_TYPE_TRI). Premultiplied.
			CF_Color tri_attributes[3];  // Per-vertex attributes (only for BATCH_GEOMETRY_TYPE_TRI).
		};
	};
	CF_M3x2 mvp; // Record-time camera transform. The command paths invert this on the
	             // GPU to recover world space per-pixel for SDF evaluation.
};

// The atlas cache's opaque per-entry `udata` carries an index back into the
// unified geometry stream (CF_Command::geoms). Atlas uvs come back via the callback.
#define ATLAS_CACHE_ASSERT CF_ASSERT
#include <cute/cute_atlas_cache.h>

struct CF_Strike
{
	CF_V2 p0, p1;
	float thickness;
	CF_Color color;
};

// Baked vector path (cf_draw_path_end): quadratic Beziers encoded into an RGBA8 atlas
// block exactly like curve-text glyph strips (3 texels per curve, 16-bit fixed-point
// coords as fractions of box_min..box_max), except paths may span multiple rows.
// Keyed by image_id (the public CF_DrawPath id) in CF_Draw::draw_paths.
struct CF_DrawPathData
{
	CF_Pixel* pixels;
	int curve_count;
	int strip_w; // Texel dimensions of the encoded block.
	int strip_h;
	CF_V2 box_min; // Path bounds in local units, the quantization box.
	CF_V2 box_max;
};

// Per-flush sprite/text atlas record, parallel to CF_Draw::pending_geoms. Filled by the
// atlas_cache callbacks; texture_id stays 0 for shapes.
struct CF_PendingUV
{
	uint64_t texture_id;
	float minx, miny, maxx, maxy;
	int tex_w, tex_h;
};

//--------------------------------------------------------------------------------------------------
// Command renderer (see s_draw_report_tiled in cute_draw.cpp).
//
// Every drawable uploads one compact command plus a small payload. Two GPU paths share
// that upload: the instanced path expands coverage quads in the vertex shader and
// rasterizes (best at moderate overdraw), while the tiled path bins commands into
// per-16px-tile lists with compute and composites in-register per pixel (best when its
// opaque-cover cull engages). Auto routing picks per batch; both preserve paint order.

#define CF_TILE_PX 16 // Tile size in pixels. Must be even (keeps 2x2 fragment quads within one tile).

// Mirrors the `Cmd` struct in the s_tile_fs builtin shader (std430, five vec4s).
// Colors travel as packed half4 (two packHalf2x16 words: rg then ba) instead of unorm8
// so HDR draw colors reach the shader intact.
struct CF_TileCmd
{
	float aabb[4];    // Pixel-space bounds, top-left origin: min.xy, max.xy.
	uint32_t type;    // Shape type id, 0-11 (see s_tile_fs / s_inst_vs).
	uint32_t color;   // packHalf2x16(premultiplied rg); ba rides in color_ba below.
	uint32_t payload; // Offset into the payload buffer, in vec4 units.
	uint32_t inv_mvp; // Offset of the inverse mvp (2 vec4s) in the payload buffer. SDF shapes only.
	float radius, stroke, aa, alpha;
	float fill, n, opaque; // opaque: filled SDF shape at full alpha AND normal blend -- opaque-cover cull candidate.
	uint32_t color_ba; // packHalf2x16(premultiplied ba); shader reads it via floatBitsToUint(misc.w).
	float user[4]; // User params (ShaderParams.attributes for custom draw shaders).
};

struct CF_TileV4 { float x, y, z, w; };

// Pure helpers, unit-tested in test/test_draw_tiled.cpp. CF_API so the tests still
// link when CF builds as a shared library.

// Inclusive tile bounds covering a pixel-space AABB. Returns false when fully outside the grid.
CF_API bool CF_CALL cf_tile_range(float min_x, float min_y, float max_x, float max_y, int tiles_x, int tiles_y, int* x0, int* y0, int* x1, int* y1);

// Runtime toggles for tests, samples, and perf comparison. The setters force a path;
// cf_draw_set_tiled_auto restores the default per-batch heuristics (tiled only when
// opaque-cover culling looks profitable; GPU binning for big-footprint batches).
CF_API void CF_CALL cf_draw_set_tiled_enabled(bool enabled);
CF_API bool CF_CALL cf_draw_get_tiled_enabled();
CF_API bool CF_CALL cf_draw_tiled_available();
CF_API void CF_CALL cf_draw_set_tiled_auto();

// Overrides the tiled batch footprint budget (bin list entries; see CF_Draw). Tests use
// a tiny budget to exercise the instanced fallback.
CF_API void CF_CALL cf_draw_set_tiled_list_budget(uint64_t entries);

// Returns and resets per-interval counters: batches drawn via the tile walk, batches
// drawn via the instanced path, and bytes uploaded. Call once per frame for stats.
CF_API void CF_CALL cf_draw_tiled_stats(int* tiled_batches, int* instanced_batches, uint64_t* upload_bytes);

struct CF_DrawUniform
{
	const char* name = NULL;
	void* data = NULL;
	int size = 0;
	CF_UniformType type = CF_UNIFORM_TYPE_UNKNOWN;
	int array_length = 0;
	bool is_texture = false;
	CF_Texture texture = { 0 };
};

struct CF_Command
{
	bool processed = false;
	int id = 0; // Simply increments for each command, used for sort ordering within a layer.
	int layer = 0;
	CF_Rect scissor = { 0, 0, -1, -1 };
	CF_Rect viewport = { 0, 0, -1, -1 };
	float alpha_discard = 1.0f;
	CF_DrawFilterMode filter_mode = CF_DRAW_FILTER_SMOOTH;
	CF_RenderState render_state;
	CF_Shader shader;
	Cute::Array<atlas_cache_entry_t> items; // Sprite/text atlas entries; udata indexes `geoms`.
	CF_DrawUniform u;
	bool is_canvas = false;
	CF_Canvas canvas = { 0 };
	CF_V2 canvas_verts[4];
	CF_V2 canvas_verts_posH[4];
	CF_Color canvas_attributes = cf_color_clear();
	// Every drawable's geometry in record (paint) order -- shapes, sprites, text.
	// Sprites/text additionally have an `items` atlas entry whose seq indexes here.
	Cute::Array<BatchGeometry> geoms;
	// Draw list replay (cf_draw_list): geometry borrowed from the list, flattened into
	// the pending stream at collate time with replay_mvp composed on and the AA band
	// rescaled -- replays never deep-copy geometry. NULL for ordinary commands.
	const Cute::Array<BatchGeometry>* geoms_ref = NULL;
	CF_M3x2 replay_mvp;
	float replay_aa_scale = 1.0f;
};

// Pushes a sprite/text atlas entry whose geometry was just appended via s_push_geom().
#define DRAW_PUSH_ITEM(s) \
	do { \
		CF_Command& cmd__ = s_draw->cmds.last(); \
		(s).udata = (ATLAS_CACHE_U64)(cmd__.geoms.count() - 1); \
		cmd__.items.add(s); \
	} while (0)

#define PUSH_DRAW_VAR(var) \
	s_draw->var##s.add(var)

#define POP_DRAW_VAR(var) \
	if (s_draw->var##s.count() > 1) { \
		auto var = s_draw->var##s.pop(); \
		return var; \
	} else { \
		return s_draw->var##s.last(); \
	}

#define PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(var) \
	if (s_draw->var##s.last() != var) { \
		CF_Command& cmd = s_draw->add_cmd(); \
		cmd.var = var; \
	} \
	PUSH_DRAW_VAR(var)

#define POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(var) \
	if (s_draw->var##s.count() > 1) { \
		auto result = s_draw->var##s.pop(); \
		if (s_draw->var##s.last() != result) { \
			CF_Command& cmd = s_draw->add_cmd(); \
			cmd.var = s_draw->var##s.last(); \
		} \
		return result; \
	} else { \
		return s_draw->var##s.last(); \
	}

#define ADD_UNIFORM(u) \
	s_draw->add_cmd(); \
	s_draw->cmds.last().u = u

struct CF_Draw
{
	CF_INLINE CF_Command& add_cmd() {
		CF_Command& cmd = cmds.add();
		cmd.id = draw_item_order++;
		cmd.layer = layers.last();
		cmd.scissor = scissors.last();
		cmd.viewport = viewports.last();
		cmd.alpha_discard = alpha_discards.last();
		cmd.filter_mode = filter_modes.last();
		cmd.render_state = render_states.last();
		cmd.shader = shaders.last();
		return cmd;
	}
	int cmd_index = 0;
	int draw_item_order = 0;
	Cute::Array<CF_Command> cmds;
	CF_V2 atlas_dims = cf_v2(2048, 2048);
	CF_V2 texel_dims = cf_v2(1.0f/2048.0f, 1.0f/2048.0f);
	bool delay_defrag = false;
	atlas_cache_t atlas_cache;
	CF_Material material;
	CF_Arena uniform_arena;
	Cute::Array<float> alpha_discards = { 1.0f };
	Cute::Array<CF_DrawFilterMode> filter_modes = { CF_DRAW_FILTER_SMOOTH };
	Cute::Array<int> blends = { 0 }; // CF_DrawBlend stack (cf_draw_push_blend).
	Cute::Array<CF_Color> colors = { cf_color_white() };
	Cute::Array<float> antialias = { 1.5f };
	Cute::Array<CF_RenderState> render_states;
	Cute::Array<CF_Rect> scissors = { { 0, 0, -1, -1 } };
	Cute::Array<CF_Rect> viewports = { { 0, 0, -1, -1 } };
	Cute::Array<int> layers = { 0 };
	Cute::Array<CF_M3x2> cam_stack = { cf_make_identity() };
	Cute::Array<CF_M3x2> projection_stack;
	float aaf = 0;
	CF_M3x2 projection;
	CF_M3x2 mvp;
	void reset_cam();
	void set_aaf();
	Cute::Array<CF_Color> user_params = { cf_color_clear() };
	Cute::Array<CF_Color> tri_colors0 = { cf_color_white() };
	Cute::Array<CF_Color> tri_colors1 = { cf_color_white() };
	Cute::Array<CF_Color> tri_colors2 = { cf_color_white() };
	Cute::Array<CF_Color> tri_attributes0 = { cf_make_color_hex(0) };
	Cute::Array<CF_Color> tri_attributes1 = { cf_make_color_hex(0) };
	Cute::Array<CF_Color> tri_attributes2 = { cf_make_color_hex(0) };
	Cute::Array<CF_Shader> shaders;
	Cute::Array<CF_V2> temp;
	Cute::Array<float> font_sizes = { 18 };
	Cute::Array<const char*> fonts = { sintern("Calibri") };
	Cute::Array<int> blurs = { 0 };
	Cute::Array<float> text_wrap_widths = { FLT_MAX };
	Cute::Array<bool> vertical = { false };
	Cute::Array<uint64_t> text_ids = { 0 };
	Cute::Array<CF_Strike> strikes;
	Cute::Array<CF_Strike> underlines;
	Cute::Array<bool> text_effects = { true };
	Cute::Array<bool> text_curves = { true };
	Cute::Array<float> text_strokes = { 0 };
	Cute::Map<CF_AtlasSubImage> premade_sub_image_id_to_sub_image;
	// Baked vector paths (cf_draw_path_end), keyed by image id in the path id range.
	Cute::Map<CF_DrawPathData> draw_paths;
	uint64_t path_image_id_gen = 0; // Seeded to CF_PATH_ID_RANGE_LO in cf_make_draw.
	// Retained draw lists (cf_make_draw_list). While recording, commands append to
	// `cmds` past recording_mark and move into the list at cf_draw_list_end.
	Cute::Map<struct CF_DrawListData*> draw_lists;
	uint64_t draw_list_id_gen = 1;
	struct CF_DrawListData* recording_list = NULL;
	int recording_mark = 0;
	// User SDF snippets registered via cf_make_custom_shape, in dispatch-index order.
	// Stitched into custom_shapes.shd and compiled into every SDF command pipeline.
	Cute::Array<Cute::String> custom_shape_srcs;
	// CSG shape group recording state (cf_draw_shape_group_begin/op/end). While active,
	// SDF shape recorders stage into group_geoms instead of emitting commands.
	bool shape_group_active = false;
	int shape_group_op = 0;
	float shape_group_k = 0;
	Cute::Array<BatchGeometry> group_geoms;
	Cute::Map<uint64_t> draw_shd_to_blit_shd;
	Cute::Map<const char*> shader_paths; // shader.id -> interned file path for reload
	bool blit_init = false;
	CF_Mesh blit_mesh = { 0 };
	bool need_flush = false;
	bool has_drawn_something = false;

	// All geometry for the current flush run in paint order, with a parallel per-entry
	// uv record for sprites/text (filled by the atlas callbacks; texture_id 0 for
	// shapes). After the flush the stream renders in paint order, splitting into a new
	// draw wherever the bound atlas texture changes -- paint order holds across atlas
	// textures.
	Cute::Array<BatchGeometry> pending_geoms;
	Cute::Array<CF_PendingUV> pending_uvs;
	CF_Texture white_texture = { 0 }; // Bound as u_image for shape-only draws.

	// Tiled renderer state. Modes: 0 = auto (per-batch profitability heuristics),
	// 1 = force off/CPU, 2 = force on/GPU.
	bool tiled_available = false;
	int tiled_mode = 0;     // Auto routes a batch tiled only when opaque-cover culling looks profitable.
	int tiled_threshold = 1; // Minimum batch item count to take the tiled path.
	// Max sum of per-command tile footprints for a tiled batch (bin list entries, 4B
	// each; 8M = 32MB). Batches over budget route instanced -- correctness never
	// depends on the tiled path, so the cap only trades its opaque-cover cull away.
	uint64_t tiled_list_budget = 8 * 1024 * 1024;
	CF_RenderState default_render_state;
	CF_Mesh tile_mesh = { 0 }; // Fullscreen triangle.
	CF_Mesh corner_mesh = { 0 }; // Six corner indices; instanced command-fed mesh path.
	bool instanced_available = false;
	CF_Material tile_material_cs = { 0 }; // Uniforms for the binning compute dispatches.
	CF_StorageBuffer tile_cmds_buf = { 0 };
	CF_StorageBuffer tile_payload_buf = { 0 };
	CF_StorageBuffer tile_headers_buf = { 0 };
	CF_StorageBuffer tile_list_buf = { 0 };
	int tile_headers_cap = 0; // Byte capacities for the GPU-written buffers.
	int tile_list_cap = 0;
	Cute::Array<CF_TileCmd> tile_cmds;
	Cute::Array<CF_TileV4> tile_payload;
	// Per-frame stats for perf inspection (reset in cf_app_draw_onto_screen path).
	int tiled_batch_count = 0;
	int instanced_batch_count = 0;
	uint64_t tiled_upload_bytes = 0;

	// Samplers for filter mode (backend-specific, stored as void* for cross-platform compatibility)
	void* sampler_nearest = NULL;
	void* sampler_linear = NULL;
};

// Retained draw list contents: deep copies of recorded commands (list-local
// transforms; replay composes the current camera on top) plus owned copies of any
// recorded uniform data (the live path arena resets every frame).
struct CF_DrawListData
{
	Cute::Array<CF_Command> cmds;
	Cute::Array<void*> uniform_blocks;
};

void cf_make_draw();
void cf_destroy_draw();

// We slice up a 64-bit int into lo + hi ranges to map where we can fetch pixels
// from. This slices up the 64-bit range into 16 unique range. The ranges are inclusive.
#define CF_IMAGE_ID_RANGE_SIZE   ((1ULL << 60) - 1)
#define CF_ASEPRITE_ID_RANGE_LO  (0ULL)
#define CF_ASEPRITE_ID_RANGE_HI  (CF_ASEPRITE_ID_RANGE_LO + CF_IMAGE_ID_RANGE_SIZE)
#define CF_CUSTOM_SPRITE_ID_RANGE_LO (CF_ASEPRITE_ID_RANGE_HI + 1)
#define CF_CUSTOM_SPRITE_ID_RANGE_HI (CF_CUSTOM_SPRITE_ID_RANGE_LO + CF_IMAGE_ID_RANGE_SIZE)
#define CF_FONT_ID_RANGE_LO      (CF_CUSTOM_SPRITE_ID_RANGE_HI + 1)
#define CF_FONT_ID_RANGE_HI      (CF_FONT_ID_RANGE_LO     + CF_IMAGE_ID_RANGE_SIZE)
#define CF_EASY_ID_RANGE_LO      (CF_FONT_ID_RANGE_HI     + 1)
#define CF_EASY_ID_RANGE_HI      (CF_EASY_ID_RANGE_LO     + CF_IMAGE_ID_RANGE_SIZE)
#define CF_PREMADE_ID_RANGE_LO   (CF_EASY_ID_RANGE_HI     + 1)
#define CF_PREMADE_ID_RANGE_HI   (CF_PREMADE_ID_RANGE_LO  + CF_IMAGE_ID_RANGE_SIZE)
#define CF_PATH_ID_RANGE_LO      (CF_PREMADE_ID_RANGE_HI  + 1)
#define CF_PATH_ID_RANGE_HI      (CF_PATH_ID_RANGE_LO     + CF_IMAGE_ID_RANGE_SIZE)

ATLAS_CACHE_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata);
void cf_destroy_texture_handle(ATLAS_CACHE_U64 texture_id, void* udata);
atlas_cache_t* cf_get_draw_atlas_cache();

#endif // CF_DRAW_INTERNAL_H
