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

#ifndef CF_AABB_GRID_H
#define CF_AABB_GRID_H

#include <cute_math.h>
#include <cute_array.h>

//--------------------------------------------------------------------------------------------------
// C++ API
#ifdef CF_CPP

namespace Cute
{

struct AabbGridNode
{
	int id;
	Aabb aabb;
	void *udata;
};

struct AabbGrid
{
	v2 extents;
	v2 center;

	int tile_size;

	Array<Array<AabbGridNode>> map;

	int count;
};

AabbGrid make_aabb_grid(Aabb bounds, int grid_size);

void aabb_grid_insert(AabbGrid *grid, Cute::Aabb aabb, void *udata);

void aabb_grid_clear(AabbGrid *grid);

void aabb_grid_query(
		AabbGrid *grid,
		Cute::Aabb aabb,
		bool (*on_hit)(int id, Cute::Aabb aabb, void *leaf_udata, void *fn_udata),
		void *udata
);

}

#endif // CF_CPP

#endif // CF_AABB_GRID_H
