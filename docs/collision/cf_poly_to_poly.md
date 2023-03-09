[](../header.md ':include')

# cf_poly_to_poly

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Returns true if two polygons are intersecting.

```cpp
bool cf_poly_to_poly(const CF_Poly* A, const CF_Transform* ax, const CF_Poly* B, const CF_Transform* bx);
```

## Remarks

For information about _how_ two shapes are intersecting (and not just boolean result), see [cf_poly_to_poly_manifold](/collision/cf_poly_to_poly_manifold.md).

## Related Pages

[CF_Poly](/collision/cf_poly.md)  
[cf_poly_to_poly_manifold](/collision/cf_poly_to_poly_manifold.md)  
