[](../header.md ':include')

# cf_capsule_to_capsule_manifold

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Computes information about how two shapes intersect.

```cpp
CF_API void CF_CALL cf_capsule_to_capsule_manifold(CF_Capsule A, CF_Capsule B, CF_Manifold* m);
```

Parameters | Description
--- | ---
A | The first shape.
B | The second shape.
m | Contains information about the intersection. `m->count` is set to zero for no-intersection. See [CF_Manifold](/collision/cf_manifold.md) for details.

## Remarks

This function is slightly slower than the boolean version [cf_capsule_to_capsule](/collision/cf_capsule_to_capsule.md).

## Related Pages

[CF_Manifold](/collision/cf_manifold.md)  
[CF_Capsule](/collision/cf_capsule.md)  
[cf_circle_to_circle_manifold](/collision/cf_circle_to_circle_manifold.md)  
[cf_circle_to_aabb_manifold](/collision/cf_circle_to_aabb_manifold.md)  
[cf_circle_to_capsule_manifold](/collision/cf_circle_to_capsule_manifold.md)  
[cf_aabb_to_aabb_manifold](/collision/cf_aabb_to_aabb_manifold.md)  
[cf_aabb_to_capsule_manifold](/collision/cf_aabb_to_capsule_manifold.md)  
[cf_circle_to_poly_manifold](/collision/cf_circle_to_poly_manifold.md)  
[cf_aabb_to_poly_manifold](/collision/cf_aabb_to_poly_manifold.md)  
[cf_capsule_to_poly_manifold](/collision/cf_capsule_to_poly_manifold.md)  
[cf_poly_to_poly_manifold](/collision/cf_poly_to_poly_manifold.md)  
