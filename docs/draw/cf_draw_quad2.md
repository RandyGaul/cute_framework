# cf_draw_quad2 | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Draws a quad wireframe.

```cpp
void cf_draw_quad2(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, float thickness);
```

Parameters | Description
--- | ---
p0 | A corner of the quad.
p1 | A corner of the quad.
p2 | A corner of the quad.
p3 | A corner of the quad.
thickness | The thickness of each line to draw.

## Remarks

All points `p0` through `p3` are encouraged to be in counter-clockwise order.

## Related Pages

[cf_draw_quad](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
[cf_draw_quad3](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad3.md)  
[cf_draw_quad_fill](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad_fill.md)  
[cf_draw_quad_fill2](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad_fill2.md)  
[cf_draw_quad_fill3](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad_fill3.md)  
cf_draw_to  
