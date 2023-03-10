[](../header.md ':include')

# cf_aabb_to_aabb

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true two Aabb's are intersecting.

```cpp
CF_API bool CF_CALL cf_aabb_to_aabb(CF_Aabb A, CF_Aabb B);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_aabb_to_aabb_manifold](/collision/cf_aabb_to_aabb_manifold.md).

## Related Pages

[CF_Aabb](/math/cf_aabb.md)  
[cf_aabb_to_aabb_manifold](/collision/cf_aabb_to_aabb_manifold.md)  
