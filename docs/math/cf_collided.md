# cf_collided | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns a true if two shapes collided.

```cpp
int cf_collided(const void* A, const CF_Transform* ax, CF_ShapeType typeA, const void* B, const CF_Transform* bx, CF_ShapeType typeB);
```

Parameters | Description
--- | ---
A | The first shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the first shape `A`.
ax_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `A`.
vA | The velocity of `A`.
B | The second shape.
typeA | The [CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md) of the second shape `B`.
bx_ptr | Can be `NULL` to represent an identity transform. An optional pointer to a [CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md) to transform `B`.

## Related Pages

[CF_ShapeType](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_shapetype.md)  
[cf_collide](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_collide.md)  
[CF_Transform](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_transform.md)  
