/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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

#ifndef CUTE_DRAW_INTERNAL_H
#define CUTE_DRAW_INTERNAL_H

#include <cute_array.h>
#include <cute_math.h>

#include <float.h>

extern struct CF_Draw* draw;

enum BatchGeometryType : int
{
	BATCH_GEOMETRY_TYPE_TRI,
	BATCH_GEOMETRY_TYPE_QUAD,
	BATCH_GEOMETRY_TYPE_SPRITE,
	BATCH_GEOMETRY_TYPE_CIRCLE,
	BATCH_GEOMETRY_TYPE_SEGMENT,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_BEGIN,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_MIDDLE,
	BATCH_GEOMETRY_TYPE_SEGMENT_CHAIN_END,
};

struct BatchTri
{
	CF_V2 p0, p1, p2;
};

struct BatchQuad
{
	CF_V2 p0, p1, p2, p3;
};

struct BatchSprite
{
	CF_V2 p0, p1, p2, p3;
	CF_Aabb clip;
	bool do_clipping;
	bool is_text;
	bool is_sprite;
};

struct BatchCircle
{
	CF_V2 p0;
};

struct BatchSegment
{
	CF_V2 p0, p1, p2, p3;
};

struct BatchGeometry
{
	BatchGeometryType type;
	CF_Pixel color;
	float alpha;
	float radius;
	float thickness; // -1 means fill.
	union
	{
		BatchTri tri;
		BatchQuad quad;
		BatchSprite sprite;
		BatchCircle circle;
		BatchSegment segment;
	} u;
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
	uint8_t type;   // r
	uint8_t alpha;  // g
	uint8_t unused; // b
	uint8_t unused; // a
};

struct CF_Strike
{
	CF_V2 p0, p1;
	float thickness;
};

struct CF_Draw
{
	v2 atlas_dims = V2(1024, 1024);
	v2 texel_dims = V2(1.0f/1024.0f, 1.0f/1024.0f);
	spritebatch_t sb;
	CF_Shader shader;
	CF_Mesh mesh;
	CF_Material material;
	CF_Filter filter = CF_FILTER_NEAREST;
	Cute::Array<CF_Color> colors = { cf_color_white() };
	Cute::Array<CF_Color> tints = { cf_color_grey() };
	Cute::Array<bool> antialias = { false };
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

// We slice up a 64-bit int into lo + hi ranges to map where we can fetch pixels
// from. This slices up the 64-bit range into 16 unique ranges, though we're only
// using three of those ranges for now. The ranges are inclusive.
#define CUTE_IMAGE_ID_RANGE_SIZE  ((1ULL << 60) - 1)
#define CUTE_ASEPRITE_ID_RANGE_LO (0ULL)
#define CUTE_ASEPRITE_ID_RANGE_HI (CUTE_ASEPRITE_ID_RANGE_LO + CUTE_IMAGE_ID_RANGE_SIZE)
#define CUTE_PNG_ID_RANGE_LO      (CUTE_ASEPRITE_ID_RANGE_HI + 1)
#define CUTE_PNG_ID_RANGE_HI      (CUTE_PNG_ID_RANGE_LO      + CUTE_IMAGE_ID_RANGE_SIZE)
#define CUTE_FONT_ID_RANGE_LO     (CUTE_PNG_ID_RANGE_HI      + 1)
#define CUTE_FONT_ID_RANGE_HI     (CUTE_FONT_ID_RANGE_LO     + CUTE_IMAGE_ID_RANGE_SIZE)

SPRITEBATCH_U64 cf_generate_texture_handle(void* pixels, int w, int h, void* udata);
void cf_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata);
spritebatch_t* cf_get_draw_sb();

#endif // CUTE_DRAW_INTERNAL_H
