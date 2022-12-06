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

extern struct CF_Draw* draw;

enum BatchGeometryType : int
{
	BATCH_GEOMETRY_TYPE_TRI,
	BATCH_GEOMETRY_TYPE_QUAD,
	BATCH_GEOMETRY_TYPE_SPRITE,
};

struct BatchTri
{
	CF_V2 p0, p1, p2;
	CF_Pixel c0, c1, c2;
};

struct BatchQuad
{
	CF_V2 p0, p1, p2, p3;
	CF_Pixel c0, c1, c2, c3;
};

struct BatchSprite
{
	CF_V2 position;
	CF_SinCos rotation;
	CF_V2 scale;
};

struct BatchGeometry
{
	BatchGeometryType type;
	float alpha;
	union
	{
		BatchTri tri;
		BatchQuad quad;
		BatchSprite sprite;
	} u;
};

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry

#include <cute/cute_spritebatch.h>

struct DrawVertex
{
	CF_V2 position;
	CF_V2 uv;
	CF_Pixel color;
	uint8_t solid;
	uint8_t outline;
	uint8_t alpha;
	uint8_t unused1;
};

#define DEFAULT_TINT cf_make_color_rgba_f(0.5f, 0.5f, 0.5f, 1.0f)

struct CF_Draw
{
	spritebatch_t sb;
	float atlas_width = 1024;
	float atlas_height = 1024;
	CF_Shader shader;
	CF_Mesh mesh;
	CF_Material material;
	float outline_use_border;
	float outline_use_corners;
	CF_Color outline_color = cf_color_white();
	CF_Filter filter = CF_FILTER_NEAREST;
	Cute::Array<CF_RenderState> render_states;
	Cute::Array<CF_Rect> scissors;
	Cute::Array<CF_Color> tints = { DEFAULT_TINT };
	Cute::Array<int> layers = { 0 };
	Cute::Array<CF_M3x2> cam_stack;
	CF_M3x2 cam = cf_make_identity();
	CF_V2 cam_dimensions = { };
	CF_V2 cam_dimensions_inverse = { };
	CF_V2 cam_position = { };
	float cam_rotation = 0;
	Cute::Array<DrawVertex> verts;
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

#endif // CUTE_DRAW_INTERNAL_H
