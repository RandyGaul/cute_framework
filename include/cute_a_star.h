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

#include <cute_defines.h>
#include <cute_array.h>

namespace cute
{

struct a_star_grid_t;

/**
 * Creates a grid based on an array of cells, usable in calls to `a_star`. The `cells` array
 * is an array of types of cells. For example, if your grid should have land and water, you
 * could use 0 for land, and 1 for water for each element in `cells`.
 * 
 * To specify a cost for each kind of cell, please see the `cell_to_cost` member of the
 * `a_star_input_t` struct.
 */
CUTE_API const a_star_grid_t* CUTE_CALL a_star_make_grid(int w, int h, const int* cells);
CUTE_API void CUTE_CALL a_star_destroy_grid(a_star_grid_t* grid);

struct a_star_input_t
{
	bool allow_diagonal_movement = true;
	int start_x = 0;
	int start_y = 0;
	int end_x = 0;
	int end_y = 0;

	/**
	 * Each element of `cells_to_cost` is used to map types of cells in the `cells` parameter
	 * of `a_star_make_grid` to a cost. The default cost is 1.0f, while <= 0.0f costs are
	 * treated as *not* traversable.
	 * 
	 * For example, to lookup the cost of a cell at { x, y }, we could do something like this.
	 * You don't need to do any lookups like this -- it all happens internally within the
	 * `a_star` function -- this is just here to explain how to setup `cell_to_cost` properly.
	 * 
	 *     float cost = cell_to_cost[cell[y * w + x]];
	 */
	const float* cell_to_cost = NULL;
};

/**
 * Represents the shortest path between two points as an array of 2d vectors.
 * The vectors x and y can be sized before calling `a_star` to avoid dynamic allocations.
 */
struct a_star_output_t
{
	array<int> x;
	array<int> y;
};

/**
 * Calculates the shortest path from start to end from `input` upon `grid`.
 * Only works with 2d grids. Arbitrary graphs are not supported.
 * The `output` struct is optional and can be NULL.
 * Returns true if a valid path was found, false otherwise.
 * 
 * Note: `grid` cannot be used by two different `a_star` calls simultaneously. If you want to
 * perform many different A* computations in a concurrent way, you need a different `grid`
 * pointer for each multithreaded call.
 */
CUTE_API bool CUTE_CALL a_star(const a_star_grid_t* grid, const a_star_input_t* input, a_star_output_t* output = NULL);

}

#endif // CUTE_A_STAR_H
