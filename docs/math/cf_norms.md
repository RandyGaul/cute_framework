# cf_norms | [math](https://github.com/RandyGaul/cute_framework/blob/master/docs/math_readme.md) | [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)

Computes the normals for a polygon.

```cpp
void cf_norms(CF_V2* verts, CF_V2* norms, int count);
```

Parameters | Description
--- | ---
verts | The vertices of the polygon.
norms | The normals of the polygon (these are written to as output).
count | The number of vertices in `verts`.

## Return Value

Writes the calculated normals to `norms`.

## Related Pages

[CF_Poly](https://github.com/RandyGaul/cute_framework/blob/master/docs/math/cf_poly.md)  
