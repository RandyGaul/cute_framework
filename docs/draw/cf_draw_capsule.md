# cf_draw_capsule | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Draws a capsule wireframe.

```cpp
void cf_draw_capsule(CF_V2 p0, CF_V2 p1, float r, int iters, float thickness);
```

Parameters | Description
--- | ---
p0 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
p1 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
r | Radius of the capsule.
iters | Number of edges used for the circle-caps. More looks smoother, but renders slower.
thickness | The thickness of each line to draw.

## Related Pages

[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
[cf_draw_capsule_fill](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_draw_capsule_fill.md)  
cf_draw_to  
