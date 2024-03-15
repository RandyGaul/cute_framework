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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_AabbGrid { uint64_t  id; } CF_AabbGrid;
typedef struct CF_AabbGridNode { int id; } CF_AabbGridNode;
typedef bool (CF_AabbGridQueryFn)(CF_AabbGridNode node, CF_Aabb aabb, void* leaf_udata, void* fn_udata);

CF_API CF_AabbGrid CF_CALL cf_make_aabb_grid(CF_V2 pos, int width, int height, int grid_size);

CF_API void CF_CALL cf_destroy_aabb_grid(CF_AabbGrid grid_handle);

CF_API void CF_CALL cf_aabb_grid_clear(CF_AabbGrid grid);

CF_API void CF_CALL cf_aabb_grid_set_pos(CF_AabbGrid grid_handle, CF_V2 pos);

CF_API void CF_CALL cf_aabb_grid_insert(CF_AabbGrid grid, CF_Aabb aabb, void *udata);

CF_API void CF_CALL cf_aabb_grid_query(CF_AabbGrid grid, CF_Aabb aabb, CF_AabbGridQueryFn *fn, void *udata);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using AabbGrid = CF_AabbGrid;
using AabbGridNode = CF_AabbGridNode;
using AabbGridQueryFn = CF_AabbGridQueryFn;
using Aabb = CF_Aabb;
using v2 = CF_V2;

CF_INLINE AabbGrid make_aabb_grid(v2 pos, int width, int height, int grid_size) { return cf_make_aabb_grid(pos, width, height, grid_size); }
CF_INLINE void destroy_aabb_grid(AabbGrid grid) { cf_destroy_aabb_grid(grid); }
CF_INLINE void aabb_grid_clear(AabbGrid grid) { cf_aabb_grid_clear(grid); }
CF_INLINE void aabb_grid_set_pos(AabbGrid grid, v2 pos) { cf_aabb_grid_set_pos(grid, pos); }
CF_INLINE void aabb_grid_insert(AabbGrid grid, Aabb aabb, void *udata) { cf_aabb_grid_insert(grid, aabb, udata); }
CF_INLINE void aabb_grid_query(AabbGrid grid, Aabb aabb, AabbGridQueryFn *fn, void *udata) { cf_aabb_grid_query(grid, aabb, fn, udata); }

}

#endif // CF_CPP

#endif // CF_AABB_GRID_H
