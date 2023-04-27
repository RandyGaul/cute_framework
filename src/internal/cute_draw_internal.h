/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
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

#ifndef CF_DRAW_INTERNAL_H
#define CF_DRAW_INTERNAL_H

#include <cute_array.h>
#include <cute_math.h>

#include <float.h>

extern struct CF_Draw* draw;

enum BatchGeometryType : int
{
	BATCH_GEOMETRY_TYPE_TRI,
	BATCH_GEOMETRY_TYPE_TRI_SDF,
	BATCH_GEOMETRY_TYPE_QUAD,
	BATCH_GEOMETRY_TYPE_SPRITE,
	BATCH_GEOMETRY_TYPE_CIRCLE,
	BATCH_GEOMETRY_TYPE_SEGMENT,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_BEGIN,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_END,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_MIDDLE,
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Pixel color;
	CF_Aabb clip;
	CF_V2 box[4];
	CF_V2 a, b, c, d;
	float alpha;
	float radius;
	float stroke;
	bool do_clipping;
	bool is_text;
	bool is_sprite;
	bool fill;
	bool antialias;
};

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry

#include <cute/cute_spritebatch.h>

struct DrawVertex
{
	CF_V2 p;
	CF_V2 a, b, c, d;
	CF_V2 uv;
	CF_Pixel color;
	float radius;
	float stroke;
	uint8_t type;    // r
	uint8_t alpha;   // g
	uint8_t fill;    // b
	uint8_t aa;      // a
};

struct CF_Strike
{
	CF_V2 p0, p1;
	float thickness;
};

struct CF_Draw
{
	CF_V2 atlas_dims = cf_v2(1024, 1024);
	CF_V2 texel_dims = cf_v2(1.0f/1024.0f, 1.0f/1024.0f);
	spritebatch_t sb;
	CF_Shader shader;
	CF_Mesh mesh;
	CF_Material material;
	CF_Filter filter = CF_FILTER_NEAREST;
	Cute::Array<CF_Color> colors = { cf_color_white() };
	Cute::Array<CF_Color> tints = { cf_color_grey() };
	Cute::Array<bool> antialias = { true };
	Cute::Array<CF_RenderState> render_states;
	Cute::Array<CF_Rect> scissors = { { -1, -1, 0, 0 } };
	Cute::Array<CF_Rect> viewports = { { -1, -1, 0, 0 } };
	Cute::Array<int> layers = { 0 };
	Cute::Array<CF_M3x2> cam_stack;
	CF_M3x2 cam = cf_make_identity();
	CF_V2 cam_dimensions = { };
	CF_V2 cam_position = { };
	float cam_rotation = 0;
	Cute::Array<CF_V2> temp;
	Cute::Array<DrawVertex> verts;
	Cute::Array<float> font_sizes = { 18 };
	Cute::Array<const char*> fonts = { NULL };
	Cute::Array<int> blurs = { 0 };
	Cute::Array<float> text_wrap_widths = { FLT_MAX };
	Cute::Array<CF_Aabb> text_clip_boxes = { cf_make_aabb(cf_v2(-FLT_MAX, -FLT_MAX), cf_v2(FLT_MAX, FLT_MAX)) };
	Cute::Array<bool> vertical = { false };
	Cute::Array<CF_Strike> strikes;
};

void cf_make_draw();
void cf_destroy_draw();
void cf_load_default_font();

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

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata);
void cf_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata);
spritebatch_t* cf_get_draw_sb();

#endif // CF_DRAW_INTERNAL_H
