/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_DRAW_INTERNAL_H
#define CF_DRAW_INTERNAL_H

#include <cute_array.h>
#include <cute_math.h>
#include <cute_draw.h>
#include <cute_graphics.h>

#include <float.h>

extern struct CF_Draw* draw;

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
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Pixel color;
	CF_V2 box[4];
	CF_V2 boxH[4];
	int n; // Only needed/used for polygon.
	CF_V2 shape[8];
	float alpha;
	float radius;
	float stroke;
	float aa;
	bool is_text;
	bool is_sprite;
	bool fill;
	bool unused;
	CF_Color user_params;
};

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry

#define SPRITEBATCH_ASSERT CF_ASSERT
#include <cute/cute_spritebatch.h>

struct CF_Strike
{
	CF_V2 p0, p1;
	float thickness;
};

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
	int id = 0; // Simply increments for each command, used for sort ordering within a layer.
	int layer = 0;
	CF_Rect scissor = { 0, 0, -1, -1 };
	CF_Rect viewport = { 0, 0, -1, -1 };
	float alpha_discard = 1.0f;
	CF_RenderState render_state;
	CF_Shader shader;
	Cute::Array<spritebatch_sprite_t> items;
	CF_DrawUniform u;
	bool is_canvas = false;
	CF_Canvas canvas = { 0 };
	CF_V2 canvas_verts[4];
	CF_V2 canvas_verts_posH[4];
	CF_Color canvas_attributes = cf_color_clear();
};

#define DRAW_PUSH_ITEM(s) \
	draw->cmds.last().items.add(s)

#define PUSH_DRAW_VAR(var) \
	draw->var##s.add(var)

#define POP_DRAW_VAR(var) \
	if (draw->var##s.count() > 1) { \
		auto var = draw->var##s.pop(); \
		return var; \
	} else { \
		return draw->var##s.last(); \
	}

#define PUSH_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(var) \
	if (draw->var##s.last() != var) { \
		CF_Command& cmd = draw->add_cmd(); \
		cmd.var = var; \
	} \
	PUSH_DRAW_VAR(var)

#define POP_DRAW_VAR_AND_ADD_CMD_IF_NEEDED(var) \
	if (draw->var##s.count() > 1) { \
		auto result = draw->var##s.pop(); \
		if (draw->var##s.last() != result) { \
			CF_Command& cmd = draw->add_cmd(); \
			cmd.var = draw->var##s.last(); \
		} \
		return result; \
	} else { \
		return draw->var##s.last(); \
	}

#define ADD_UNIFORM(u) \
	draw->add_cmd(); \
	draw->cmds.last().u = u

struct CF_Draw
{
	CF_INLINE CF_Command& add_cmd() {
		CF_Command& cmd = cmds.add();
		cmd.id = draw_item_order++;
		cmd.layer = layers.last();
		cmd.scissor = scissors.last();
		cmd.viewport = viewports.last();
		cmd.alpha_discard = alpha_discards.last();
		cmd.render_state = render_states.last();
		cmd.shader = shaders.last();
		return cmd;
	}
	int cmd_index = 0;
	int draw_item_order = 0;
	Cute::Array<CF_Command> cmds;
	Cute::Array<CF_Vertex> verts;
	CF_V2 atlas_dims = cf_v2(2048, 2048);
	CF_V2 texel_dims = cf_v2(1.0f/2048.0f, 1.0f/2048.0f);
	bool delay_defrag = false;
	spritebatch_t sb;
	CF_Mesh mesh;
	CF_Material material;
	CF_Arena uniform_arena;
	Cute::Array<float> alpha_discards = { true };
	Cute::Array<CF_Color> colors = { cf_color_white() };
	Cute::Array<bool> antialias = { true };
	Cute::Array<float> antialias_scale = { 1.5f };
	Cute::Array<CF_RenderState> render_states;
	Cute::Array<CF_Rect> scissors = { { 0, 0, -1, -1 } };
	Cute::Array<CF_Rect> viewports = { { 0, 0, -1, -1 } };
	Cute::Array<int> layers = { 0 };
	Cute::Array<CF_M3x2> cam_stack = { cf_make_identity() };
	float aaf = 0;
	CF_M3x2 projection;
	CF_M3x2 mvp;
	void reset_cam();
	void set_aaf();
	Cute::Array<CF_Color> user_params = { cf_make_color_hex(0) };
	Cute::Array<CF_Shader> shaders;
	Cute::Array<CF_V2> temp;
	Cute::Array<float> font_sizes = { 18 };
	Cute::Array<const char*> fonts = { sintern("Calibri") };
	Cute::Array<int> blurs = { 0 };
	Cute::Array<float> text_wrap_widths = { FLT_MAX };
	Cute::Array<bool> vertical = { false };
	Cute::Array<CF_Strike> strikes;
	Cute::Array<bool> text_effects = { true };
	Cute::Map<uint64_t, CF_AtlasSubImage> premade_sub_image_id_to_sub_image;
	Cute::Map<uint64_t, uint64_t> draw_shd_to_blit_shd;
	bool blit_init = false;
	CF_Mesh blit_mesh = { 0 };
	CF_VertexFn* vertex_fn = NULL;
	bool has_drawn_something = false;
};

void cf_make_draw();
void cf_destroy_draw();

// We slice up a 64-bit int into lo + hi ranges to map where we can fetch pixels
// from. This slices up the 64-bit range into 16 unique range. The ranges are inclusive.
#define CF_IMAGE_ID_RANGE_SIZE   ((1ULL << 60) - 1)
#define CF_ASEPRITE_ID_RANGE_LO  (0ULL)
#define CF_ASEPRITE_ID_RANGE_HI  (CF_ASEPRITE_ID_RANGE_LO + CF_IMAGE_ID_RANGE_SIZE)
#define CF_PNG_ID_RANGE_LO       (CF_ASEPRITE_ID_RANGE_HI + 1)
#define CF_PNG_ID_RANGE_HI       (CF_PNG_ID_RANGE_LO      + CF_IMAGE_ID_RANGE_SIZE)
#define CF_FONT_ID_RANGE_LO      (CF_PNG_ID_RANGE_HI      + 1)
#define CF_FONT_ID_RANGE_HI      (CF_FONT_ID_RANGE_LO     + CF_IMAGE_ID_RANGE_SIZE)
#define CF_EASY_ID_RANGE_LO      (CF_FONT_ID_RANGE_HI     + 1)
#define CF_EASY_ID_RANGE_HI      (CF_EASY_ID_RANGE_LO     + CF_IMAGE_ID_RANGE_SIZE)
#define CF_PREMADE_ID_RANGE_LO   (CF_EASY_ID_RANGE_HI     + 1)
#define CF_PREMADE_ID_RANGE_HI   (CF_PREMADE_ID_RANGE_LO  + CF_IMAGE_ID_RANGE_SIZE)

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata);
void cf_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata);
spritebatch_t* cf_get_draw_sb();

#endif // CF_DRAW_INTERNAL_H
