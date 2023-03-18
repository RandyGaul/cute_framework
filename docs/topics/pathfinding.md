[](../header.md ':include')

<br>

CF uses the [A\* algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) on 2D grids for path-finding. A\* will find the shortest path in a grid from point A to point B. It also allows each grid cell to have a cost value, making it more difficult to pass over certain kinds of terrain. Be sure to check out the [Pathfinding API](https://randygaul.github.io/cute_framework/#/api_reference?id=pathfinding) list to see all the available functionality.

## Creating a Grid

Before performing A\* a grid must be constructed with [`cf_make_a_star_grid`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_make_a_star_grid). The grid is a 2x2 array of floats, each initialized to a default _cost value_ of 1.0f.

```cpp
CF_AStarGrid grid = cf_make_a_star_grid(200, 200, NULL);
```

The last parameter `NULL` is optional -- you can provide your own array of floats for cost values. By passing in `NULL` an array of floats is initialized to 1.0f cost. You can adjust the cost of any grid cell with [`cf_a_star_grid_set_cost`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_a_star_grid_set_cost), or lookup a particular cost with [`cf_a_star_grid_get_cost`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_a_star_grid_get_cost).

```cpp
// Make sure the mountain coordinates are set to high cost of 10!
// Recall that 1.0f is the default cost.
for (int i = 0; i < mountains.count(); ++i) {
	int x = mountains.x[i];
	int y = mountains.y[i];
	cf_a_star_grid_set_cost(grid, x, y, 10.0f);
}
```

When you are done using a grid free it up with [`cf_destroy_a_star_grid`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_destroy_a_star_grid).

## Performing A*

To perform the A\* algorithm call [`cf_a_star`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_a_star). You must supply a start/end position. This function will fill in a struct called [`CF_AStarOutput`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_astaroutput) containing the shortest path from start/end. If no valid path could be found `false` is returned instead of `true`.

```cpp
CF_AStarOutput out;
bool allow_diagonal_movement = true;
bool valid = cf_a_star(grid, start_x, start_y, end_x, end_y, allow_diagonal_movement, out);
if (valid) {
	for (int i = 0; i < out.count; ++i) {
		int x = out.x[i];
		int y = out.y[i];
		// ...
	}
	cf_free_a_star_output(&out);
}
```

?> Be sure to free up your [`CF_AStarOutput`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_astaroutput) with [`cf_free_a_star_output`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_free_a_star_output) if [`cf_a_star`](https://randygaul.github.io/cute_framework/#/pathfinding/cf_a_star) returns `true`.
