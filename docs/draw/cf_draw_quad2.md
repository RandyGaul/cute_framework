[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_draw_quad2

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a quad wireframe.

```cpp
void cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness, float chubbiness);
```

Parameters | Description
--- | ---
p0 | A corner of the quad.
p1 | A corner of the quad.
p2 | A corner of the quad.
p3 | A corner of the quad.
thickness | The thickness of each line to draw.
chubbiness | Inflates the shape, similar to corner-rounding. Makes the shape chubbier.

## Remarks

All points `p0` through `p3` are encouraged to be in counter-clockwise order.

## Related Pages

[cf_draw_quad](/draw/cf_draw_quad.md)  
[cf_draw_quad_fill2](/draw/cf_draw_quad_fill2.md)  
[cf_draw_quad_fill](/draw/cf_draw_quad_fill.md)  
