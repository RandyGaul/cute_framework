# cf_draw_bezier_line

Category: [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws line segments over a quadratic bezier line.

```cpp
void cf_draw_bezier_line(CF_V2 a, CF_V2 c0, CF_V2 b, int iters, float thickness);
```

Parameters | Description
--- | ---
a | The starting point.
c0 | A bezier control point.
b | The end point.
thickness | The thickness of the line to draw.
iters | The number of lines used to draw the bezier spline.

## Related Pages

[cf_draw_line](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_line.md)  
[cf_draw_line2](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_line2.md)  
[cf_draw_polyline](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_polyline.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
[cf_draw_bezier_line2](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_bezier_line2.md)  
cf_draw_to  
