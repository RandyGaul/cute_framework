# cf_draw_quad_fill3 | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Draws a quad wireframe.

```cpp
void cf_draw_quad_fill3(CF_V2 p0, CF_V2 p1, CF_V2 p2, CF_V2 p3, CF_Color c0, CF_Color c1, CF_Color c2, CF_Color c3);
```

Parameters | Description
--- | ---
p0 | A corner of the quad.
p1 | A corner of the quad.
p2 | A corner of the quad.
p3 | A corner of the quad.
c0 | The color of a corner of the quad.
c1 | The color of a corner of the quad.
c2 | The color of a corner of the quad.
c3 | The color of a corner of the quad.

## Remarks

All points `p0` through `p3` are encouraged to be in counter-clockwise order.

## Related Pages

[cf_draw_quad](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad.md)  
[cf_draw_quad2](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad2.md)  
[cf_draw_quad3](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad3.md)  
[cf_draw_quad_fill](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad_fill.md)  
[cf_draw_quad_fill2](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_quad_fill2.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
cf_draw_to  
