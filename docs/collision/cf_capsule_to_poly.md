# cf_capsule_to_poly

Category: [collision](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true an capsule is intersecting a polygon.

```cpp
bool cf_capsule_to_poly(CF_Capsule A, const CF_Poly* B, const CF_Transform* bx);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_capsule_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly_manifold.md).

## Related Pages

[CF_Capsule](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule.md)  
[CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_poly.md)  
[cf_capsule_to_poly_manifold](https://github.com/RandyGaul/cute_framework/blob/master/docs/collision/cf_capsule_to_poly_manifold.md)  
