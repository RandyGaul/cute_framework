[](../header.md ':include')

# cf_a_star

Category: [pathfinding](/api_reference?id=pathfinding)  
GitHub: [cute_a_star.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_a_star.h)  
---

Calculates the shortest path along a grid.

```cpp
bool cf_a_star(CF_AStarGrid grid, int start_x, int start_y, int end_x, int end_y, bool allow_diagonal_movement, CF_AStarOutput* out);
```

Parameters | Description
--- | ---
grid | The [CF_AStarGrid](/pathfinding/cf_astargrid.md) for calculating the shortest path along.
start_x | The starting x-position of the path.
start_y | The starting y-position of the path.
end_x | The ending x-position of the path.
end_y | The ending y-position of the path.
allow_diagonal_movement | True to allow diagonal movements on the grid. False for only up/down/left/right movements.
out | [CF_AStarOutput](/pathfinding/cf_astaroutput.md) containing the calculated shortest path. Free it up with [cf_free_a_star_output](/pathfinding/cf_free_a_star_output.md) when done.

## Return Value

Returns true if a path was calculated, false if no valid path is possible.

## Remarks

Call [cf_make_a_star_grid](/pathfinding/cf_make_a_star_grid.md) to make a [CF_AStarGrid](/pathfinding/cf_astargrid.md) before calling this function. The grid is treated as read-only, so you can freely
make multithreaded calls to [cf_a_star](/pathfinding/cf_a_star.md) with the same grid, as long as you don't modify the grid cell costs in the meantime.

## Related Pages

[CF_AStarGrid](/pathfinding/cf_astargrid.md)  
[cf_free_a_star_output](/pathfinding/cf_free_a_star_output.md)  
