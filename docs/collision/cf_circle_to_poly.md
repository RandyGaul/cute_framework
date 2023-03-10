[](../header.md ':include')

# cf_circle_to_poly

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if a circle is intersecting a polygon.

```cpp
CF_API bool CF_CALL cf_circle_to_poly(CF_Circle A, const CF_Poly* B, const CF_Transform* bx);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_circle_to_poly_manifold](/collision/cf_circle_to_poly_manifold.md).

## Related Pages

[CF_Circle](/math/cf_circle.md)  
[CF_Poly](/collision/cf_poly.md)  
[cf_circle_to_poly_manifold](/collision/cf_circle_to_poly_manifold.md)  
