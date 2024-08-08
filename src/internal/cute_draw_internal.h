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
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Pixel color;
	CF_Aabb clip;
	CF_V2 box[4];
	CF_V2 boxH[4];
	CF_V2 a, b, c, d;
	float alpha;
	float radius;
	float stroke;
	float aa;
	bool do_clipping;
	bool is_text;
	bool is_sprite;
	bool fill;
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

struct CF_Draw
{
	CF_V2 atlas_dims = cf_v2(2048, 2048);
	CF_V2 texel_dims = cf_v2(1.0f/2048.0f, 1.0f/2048.0f);
	bool delay_defrag = false;
	spritebatch_t sb;
	CF_Mesh mesh;
	CF_Material material;
	CF_Filter filter = CF_FILTER_NEAREST;
	Cute::Array<CF_Color> colors = { cf_color_white() };
	Cute::Array<CF_Color> tints = { cf_color_grey() };
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
	Cute::Array<CF_Vertex> verts;
	Cute::Array<float> font_sizes = { 18 };
	Cute::Array<const char*> fonts = { sintern("Calibri") };
	Cute::Array<int> blurs = { 0 };
	Cute::Array<float> text_wrap_widths = { FLT_MAX };
	Cute::Array<CF_Aabb> text_clip_boxes = { cf_make_aabb(cf_v2(-FLT_MAX, -FLT_MAX), cf_v2(FLT_MAX, FLT_MAX)) };
	Cute::Array<bool> vertical = { false };
	Cute::Array<CF_Strike> strikes;
	Cute::Array<bool> text_effects = { true };
	Cute::Map<uint64_t, uint64_t> premade_sub_image_id_to_png_atlas_map;
	Cute::Map<uint64_t, CF_AtlasSubImage> premade_sub_image_id_to_sub_image;
	CF_VertexFn* vertex_fn = NULL;
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
