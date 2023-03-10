[](../header.md ':include')

# cf_aabb_to_poly

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true an Aabb is intersecting a polygon.

```cpp
CF_API bool CF_CALL cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_aabb_to_poly_manifold](/collision/cf_aabb_to_poly_manifold.md).

## Related Pages

[CF_Aabb](/math/cf_aabb.md)  
[CF_Poly](/collision/cf_poly.md)  
[cf_aabb_to_poly_manifold](/collision/cf_aabb_to_poly_manifold.md)  
