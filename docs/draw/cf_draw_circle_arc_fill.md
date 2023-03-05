# cf_draw_circle_arc_fill | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Draws an arc of a circle, like a pie-slice.

```cpp
void cf_draw_circle_arc_fill(CF_V2 p, CF_V2 center_of_arc, float range, int iters);
```

Parameters | Description
--- | ---
p | Center of the arc.
center_of_arc | Radius of the circle.
range | Angle the arc covers.
iters | Number of edges used for the circle. More looks smoother, but renders slower.

## Related Pages

[cf_draw_circle](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_circle.md)  
[cf_draw_circle_fill](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_circle_fill.md)  
[cf_draw_circle_arc](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_circle_arc.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
cf_draw_to  
