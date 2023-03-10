[](../header.md ':include')

# cf_make_poly

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Fills out the polygon with values.

```cpp
CF_API void CF_CALL cf_make_poly(CF_Poly* p);
```

## Remarks

Runs [cf_hull](/collision/cf_hull.md) and [cf_norms](/collision/cf_norms.md), assumes p->verts and p->count are both set to valid values.

## Related Pages

[CF_Poly](/collision/cf_poly.md)  
[cf_hull](/collision/cf_hull.md)  
[cf_norms](/collision/cf_norms.md)  
