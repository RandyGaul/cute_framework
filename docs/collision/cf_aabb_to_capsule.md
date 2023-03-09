[](../header.md ':include')

# cf_aabb_to_capsule

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if an Aabb is intersecting a capsule.

```cpp
bool cf_aabb_to_capsule(CF_Aabb A, CF_Capsule B);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_aabb_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule_manifold.md).

## Related Pages

[CF_Aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb.md)  
[CF_Capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule.md)  
[cf_aabb_to_capsule_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_aabb_to_capsule_manifold.md)  
