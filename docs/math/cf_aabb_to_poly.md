# cf_aabb_to_poly | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/README.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Returns true an Aabb is intersecting a polygon.

```cpp
bool cf_aabb_to_poly(CF_Aabb A, const CF_Poly* B, const CF_Transform* bx);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb_to_poly_manifold.md).

## Related Pages

[CF_Aabb](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb.md)  
[CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_poly.md)  
[cf_aabb_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_aabb_to_poly_manifold.md)  
