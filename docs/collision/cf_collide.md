[](../header.md ':include')

# cf_collide

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Computes a [CF_Manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_manifold.md) between two shapes.

```cpp
void cf_collide(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB, CF_Manifold* m);
```

Parameters | Description
--- | ---
A | The first shape.
ax | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `A`.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_shapetype.md) of the first shape `A`.
B | The second shape.
bx | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `B`.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_shapetype.md) of the second shape `B`.
m | Contains information about the intersection. `m->count` is set to zero for no-intersection. See [CF_Manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_manifold.md) for details.

## Related Pages

[cf_collided](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_collided.md)  
[CF_Manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_manifold.md)  
[CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md)  
[CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_shapetype.md)  
