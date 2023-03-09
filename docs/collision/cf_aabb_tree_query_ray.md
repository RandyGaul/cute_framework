# cf_aabb_tree_query_ray

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

Finds all [CF_Leaf](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_leaf.md)'s who's AABB overlaps the param `ray`.

```cpp
void cf_aabb_tree_query_ray(const CF_AabbTree tree, CF_AabbTreeQueryFn* fn, CF_Ray ray, void* fn_udata);
```

Parameters | Description
--- | ---
tree | The tree to query.
fn | A callback [CF_AabbTreeQueryFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtreequeryfn.md) you have implemented. This is called once per hit. See [CF_AabbTreeQueryFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtreequeryfn.md) for more details.
ray | The [CF_Ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_ray.md) to query the tree against. Internally tracked AABBs hit will be reported via `fn`, a callback [CF_AabbTreeQueryFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtreequeryfn.md) you must implement.
fn_udata | Can be `NULL`. An optional user data pointer, handed back to you when `fn` is called to report hits.

## Related Pages

[cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
