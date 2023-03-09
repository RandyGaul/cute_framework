# CF_AabbTreeQueryFn

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_aabb_tree.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_aabb_tree.h)  
---

A function pointer (callback) that reports hits during a tree collision query.

```cpp
typedef bool (CF_AabbTreeQueryFn)(CF_Leaf leaf, CF_Aabb aabb, void* leaf_udata, void* fn_udata);
```

Parameters | Description
--- | ---
leaf | The leaf node currently hit by the query.
aabb | The AABB of the leaf currently hit by the query.
leaf_udata | The user data pointer provided when you called [cf_aabb_tree_insert](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_insert.md) as the `udata` param.
fn_udata | The user data pointer provided when you called a query function, such as [cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md) or [cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md).

## Return Value

You should return true if you want to continue the query, or false if you want to early-exit for any reason.

## Code Example

> Calculating a path along a small grid, and printing the result.

```cpp
TODO
```

## Remarks

This function pointer is a function you must implement yourself. It will be called by [cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md) or [cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)
during a query, and reports hits to you. Usually you will return true, but if you want to early out for any reason you can return false. You may
return false as an optimization, for example, if you want to know if you hit _anything_, since processing later hits won't matter after the
  first hit is found.

## Related Pages

[CF_AabbTree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabbtree.md)  
[cf_make_aabb_tree](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_make_aabb_tree.md)  
[cf_aabb_tree_query_aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_aabb.md)  
[cf_aabb_tree_query_ray](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_tree_query_ray.md)  
