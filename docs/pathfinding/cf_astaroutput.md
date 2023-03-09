# CF_AStarOutput

Category: [pathfinding](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=pathfinding)  
GitHub: [cute_a_star.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_a_star.h)  
---

Represents the shortest path between two points as an array of 2d vectors.

Struct Members | Description
--- | ---
`int x_count` | The number of elements in the `x` array.
`dyna int* x` | An array of x-coordinates, one for each (x, y) coordinate in the calculated path.
`int y_count` | The number of elements in the `y` array.
`dyna int* y` | An array of y-coordinates, one for each (x, y) coordinate in the calculated path.

## Remarks

This output is returned by [cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md) as an output parameter. Free it up with [cf_free_a_star_output](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_free_a_star_output.md) when done.

## Related Pages

[cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md)  
[cf_free_a_star_output](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_free_a_star_output.md)  
