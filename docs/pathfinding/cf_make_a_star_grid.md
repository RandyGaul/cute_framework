# cf_make_a_star_grid

Category: [pathfinding](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=pathfinding)  
GitHub: [cute_a_star.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_a_star.h)  
---

Creates a [CF_AStarGrid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_astargrid.md) for running a path-finding algorithm via [cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md).

```cpp
CF_AStarGrid cf_make_a_star_grid(int w, int h, float* cell_costs);
```

Parameters | Description
--- | ---
w | The width of the grid.
h | The height of the grid.
cell_costs | The cost of each cell in the grid. 1.0f is default. Anything < 1.0f is treated as _non-traversable_. Can be `NULL`, meaning each cell is 1.0f cost.

## Return Value

Returns a [CF_AStarGrid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_astargrid.md) for path-finding. Pass this to the function [cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md).

## Code Example

> Calculating a path along a small grid, and printing the result.

```cpp
TODO
```

## Remarks

[CF_AStarGrid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_astargrid.md)'s are designed to be created once and used many times. You can update the cost of any index with
[cf_a_star_grid_set_cost](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star_grid_set_cost.md) and [cf_a_star_grid_get_cost](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star_grid_get_cost.md). Free the grid when you're done using it with [cf_destroy_a_star_grid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_destroy_a_star_grid.md).

## Related Pages

[cf_destroy_a_star_grid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_destroy_a_star_grid.md)  
[cf_a_star_grid_set_cost](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star_grid_set_cost.md)  
[cf_a_star_grid_get_cost](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star_grid_get_cost.md)  
[cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md)  
