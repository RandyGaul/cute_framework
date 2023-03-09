[](../header.md ':include')

# cf_hull

Category: [collision](/api_reference?id=collision)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Computes 2D convex hull.

```cpp
int cf_hull(CF_V2* verts, int count);
```

Parameters | Description
--- | ---
verts | The vertices of the shape.
count | The number of vertices in `verts`.
skin_factor | The amount to inflate the shape by.

## Return Value

Returns the number of vertices written to the `verts` array.

## Remarks

Will not do anything if less than two verts supplied. If more than CF_POLY_MAX_VERTS are supplied extras are ignored.

## Related Pages

[CF_Poly](/collision/cf_poly.md)  
