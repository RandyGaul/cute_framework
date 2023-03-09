# cf_free_a_star_output

Category: [pathfinding](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=pathfinding)  
GitHub: [cute_a_star.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_a_star.h)  
---

Frees up all resources used by [CF_AStarOutput](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_astaroutput.md) from calling [cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md).

```cpp
void cf_free_a_star_output(CF_AStarOutput* out);
```

Parameters | Description
--- | ---
out | The output from a call to [cf_a_star](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_a_star.md).

## Related Pages

[CF_AStarGrid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_astargrid.md)  
[cf_destroy_a_star_grid](https://github.com/RandyGaul/cute_framework/blob/master/docs/pathfinding/cf_destroy_a_star_grid.md)  
