[](../header.md ':include')

# cf_draw_tri

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a triangle wireframe.

```cpp
void cf_draw_tri(CF_V2 p0, CF_V2 p1, CF_V2 p2, float thickness, float chubbiness);
```

Parameters | Description
--- | ---
p0 | A corner of the triangle.
p1 | A corner of the triangle.
p2 | A corner of the triangle.
thickness | The thickness of each line to draw.
chubbiness | Inflates the shape, similar to corner-rounding. Makes the shape chubbier.

## Related Pages

[cf_draw_tri_fill](/draw/cf_draw_tri_fill.md)  
