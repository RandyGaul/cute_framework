/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/


#ifndef CF_A_STAR_H
#define CF_A_STAR_H

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_AStarGrid
 * @category pathfinding
 * @brief    An opaque handle representing a grid for calculating shortest paths. See `cf_make_a_star_grid` for more details.
 * @related  cf_make_a_star_grid cf_destroy_a_star_grid cf_a_star_grid_set_cost cf_a_star_grid_get_cost cf_a_star
 */
typedef struct CF_AStarGrid { uint64_t id; } CF_AStarGrid;
// @end

/**
 * @function cf_make_a_star_grid
 * @category pathfinding
 * @brief    Creates a `CF_AStarGrid` for running a path-finding algorithm via `cf_a_star`.
 * @param    w           The width of the grid.
 * @param    h           The height of the grid.
 * @param    cell_costs  The cost of each cell in the grid. 1.0f is default. Anything < 1.0f is treated as _non-traversable_. Can be `NULL`, meaning each cell is 1.0f cost.
 * @return   Returns a `CF_AStarGrid` for path-finding. Pass this to the function `cf_a_star`.
 * @example > Calculating a path along a small grid, and printing the result.
 *     TODO
 * @remarks  `CF_AStarGrid`'s are designed to be created once and used many times. You can update the cost of any index with
 *           `cf_a_star_grid_set_cost` and `cf_a_star_grid_get_cost`. Free the grid when you're done using it with `cf_destroy_a_star_grid`.
 * @related  cf_destroy_a_star_grid cf_a_star_grid_set_cost cf_a_star_grid_get_cost cf_a_star
 */
CF_API CF_AStarGrid CF_CALL cf_make_a_star_grid(int w, int h, float* cell_costs);

/**
 * @function cf_a_star_grid_get_cost
 * @category pathfinding
 * @brief    Get the cost of a grid cell.
 * @param    x          The x position of the grid cell.
 * @param    y          The y position of the grid cell.
 * @return   Returns the cost of the grid cell.
 * @related  cf_make_a_star_grid cf_destroy_a_star_grid cf_a_star_grid_set_cost cf_a_star
 */
CF_API float CF_CALL cf_a_star_grid_get_cost(CF_AStarGrid grid, int x, int y);

/**
 * @function cf_a_star_grid_set_cost
 * @category pathfinding
 * @brief    Set the cost of a grid cell.
 * @param    x          The x position of the grid cell.
 * @param    y          The y position of the grid cell.
 * @param    cost       The cost of the grid cell.
 * @related  cf_make_a_star_grid cf_destroy_a_star_grid cf_a_star_grid_get_cost cf_a_star
 */
CF_API void CF_CALL cf_a_star_grid_set_cost(CF_AStarGrid grid, int x, int y, float cost);

/**
 * @function cf_destroy_a_star_grid
 * @category pathfinding
 * @brief    Free up all resources used by a grid.
 * @param    grid       The grid.
 * @related  cf_make_a_star_grid cf_a_star_grid_set_cost cf_a_star_grid_get_cost cf_a_star
 */
CF_API void CF_CALL cf_destroy_a_star_grid(CF_AStarGrid grid);

/**
 * @struct   CF_AStarOutput
 * @category pathfinding
 * @brief    Represents the shortest path between two points as an array of 2d vectors.
 * @remarks  This output is returned by `cf_a_star` as an output parameter. Free it up with `cf_free_a_star_output` when done.
 * @related  cf_a_star cf_free_a_star_output
 */
typedef struct CF_AStarOutput
{
	/* @member The number of elements in the `x` and `y` arrays. */
	int count;

	/* @member An array of x-coordinates, one for each (x, y) coordinate in the calculated path. */
	dyna int* x;

	/* @member An array of y-coordinates, one for each (x, y) coordinate in the calculated path. */
	dyna int* y;
} CF_AStarOutput;
// @end

/**
 * @function cf_a_star
 * @category pathfinding
 * @brief    Calculates the shortest path along a grid.
 * @param    grid                     The `CF_AStarGrid` for calculating the shortest path along.
 * @param    start_x                  The starting x-position of the path.
 * @param    start_y                  The starting y-position of the path.
 * @param    end_x                    The ending x-position of the path.
 * @param    end_y                    The ending y-position of the path.
 * @param    allow_diagonal_movement  True to allow diagonal movements on the grid. False for only up/down/left/right movements.
 * @param    out                      `CF_AStarOutput` containing the calculated shortest path. Free it up with `cf_free_a_star_output` when done.
 * @return   Returns true if a path was calculated, false if no valid path is possible.
 * @remarks  Call `cf_make_a_star_grid` to make a `CF_AStarGrid` before calling this function. The grid is treated as read-only, so you can freely
 *           make multithreaded calls to `cf_a_star` with the same grid, as long as you don't modify the grid cell costs in the meantime.
 * @related  CF_AStarGrid cf_free_a_star_output
 */
CF_API bool CF_CALL cf_a_star(CF_AStarGrid grid, int start_x, int start_y, int end_x, int end_y, bool allow_diagonal_movement, CF_AStarOutput* out);

/**
 * @function cf_free_a_star_output
 * @category pathfinding
 * @brief    Frees up all resources used by `CF_AStarOutput` from calling `cf_a_star`.
 * @param    out           The output from a call to `cf_a_star`.
 * @related  CF_AStarGrid cf_destroy_a_star_grid
 */
CF_API void CF_CALL cf_free_a_star_output(CF_AStarOutput* out);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

#include "cute_array.h"

namespace Cute
{

using AStarGrid = CF_AStarGrid;
using AStarOutput = CF_AStarOutput;

CF_INLINE AStarGrid make_a_star_grid(int w, int h, float* cell_costs) { return cf_make_a_star_grid(w, h, cell_costs); }
CF_INLINE void destroy_a_star_grid(AStarGrid grid) { cf_destroy_a_star_grid(grid); }
CF_INLINE bool a_star(AStarGrid grid, int start_x, int start_y, int end_x, int end_y, bool allow_diagonal_movement, AStarOutput* out = NULL) { return cf_a_star(grid, start_x, start_y, end_x, end_y, allow_diagonal_movement, out); }
CF_INLINE void free_a_star_output(CF_AStarOutput* output) { cf_free_a_star_output(output); }

}

#endif // CF_CPP

#endif // CF_A_STAR_H
