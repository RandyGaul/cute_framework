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

#ifndef CUTE_A_STAR_H
#define CUTE_A_STAR_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct CF_AStarGrid CF_AStarGrid;

/**
 * Creates a grid based on an array of cells, usable in calls to `a_star`. The `cells` array
 * is an array of types of cells. For example, if your grid should have land and water, you
 * could use 0 for land, and 1 for water for each element in `cells`.
 *
 * To specify a cost for each kind of cell, please see the `cell_to_cost` member of the
 * `CF_AStarInput` struct.
 */
CUTE_API const CF_AStarGrid* CUTE_CALL cf_make_a_star_grid(int w, int h, const int* cells);
CUTE_API void CUTE_CALL cf_destroy_a_star_grid(CF_AStarGrid* grid);

typedef struct CF_AStarInput
{
	bool allow_diagonal_movement;
	int start_x;
	int start_y;
	int end_x;
	int end_y;

	/**
	 * Each element of `cells_to_cost` is used to map types of cells in the `cells` parameter
	 * of `make_a_star_grid` to a cost. The default cost is 1.0f, while <= 0.0f costs are
	 * treated as *not* traversable.
	 *
	 * For example, to lookup the cost of a cell at { x, y }, we could do something like this.
	 * You don't need to do any lookups like this -- it all happens internally within the
	 * `a_star` function -- this is just here to explain how to setup `cell_to_cost` properly.
	 *
	 *     float cost = cell_to_cost[cell[y * w + x]];
	 */
	const float* cell_to_cost;
} CF_AStarInput;

CUTE_API CF_AStarInput CUTE_CALL cf_a_star_input_defaults();

/**
 * Represents the shortest path between two points as an array of 2d vectors.
 */
typedef struct CF_AStarOutput
{
	const int* x;
	const int* y;

	int x_count;
	int y_count;
} CF_AStarOutput;

/**
 * Calculates the shortest path from start to end from `input` upon `grid`.
 * Only works with 2d grids. Arbitrary graphs are not supported.
 * The `output` struct is optional and can be NULL.
 * Returns true if a valid path was found, false otherwise.
 *
 * Note: `grid` cannot be used by two different `cf_a_star` calls simultaneously. If you want to
 * perform many different A* computations in a concurrent way, you need a different `grid`
 * pointer for each multithreaded call.
 */
CUTE_API bool CUTE_CALL cf_a_star(const CF_AStarGrid* grid, const CF_AStarInput* input, CF_AStarOutput* output /* = NULL */);
CUTE_API void CUTE_CALL cf_free_a_star_output(CF_AStarOutput* output);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

#include "cute_array.h"

namespace Cute
{

using AStarGrid = CF_AStarGrid;

struct AStarInput : public CF_AStarInput
{
	AStarInput() { *(CF_AStarInput*)this = cf_a_star_input_defaults(); }
};

struct AStarOutput
{
	Array<int> x;
	Array<int> y;
};

CUTE_INLINE const AStarGrid* make_a_star_grid(int w, int h, const int* cells) { return cf_make_a_star_grid(w, h, cells); }
CUTE_INLINE void destroy_a_star_grid(AStarGrid* grid) { cf_destroy_a_star_grid(grid); }
CUTE_API bool CUTE_CALL a_star(const AStarGrid* grid, const AStarInput* input, AStarOutput* output = NULL);

}

#endif // CUTE_CPP

#endif // CUTE_A_STAR_H
