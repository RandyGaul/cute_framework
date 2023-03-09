[](../header.md ':include')

# cf_circle_to_circle_manifold

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Computes information about how two shapes intersect.

```cpp
void cf_circle_to_circle_manifold(CF_Circle A, CF_Circle B, CF_Manifold* m);
```

Parameters | Description
--- | ---
A | The first shape.
B | The second shape.
m | Contains information about the intersection. `m->count` is set to zero for no-intersection. See [CF_Manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_manifold.md) for details.

## Remarks

This function is slightly slower than the boolean version [cf_circle_to_circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_circle.md).

## Related Pages

[CF_Manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_manifold.md)  
[CF_Circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_circle.md)  
[cf_poly_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly_to_poly_manifold.md)  
[cf_circle_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_aabb_manifold.md)  
[cf_circle_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_capsule_manifold.md)  
[cf_aabb_to_aabb_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_aabb_manifold.md)  
[cf_aabb_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule_manifold.md)  
[cf_circle_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_circle_to_poly_manifold.md)  
[cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_poly_manifold.md)  
[cf_capsule_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly_manifold.md)  
