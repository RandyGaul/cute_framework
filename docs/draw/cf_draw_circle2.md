[](../header.md ':include')

# cf_draw_circle2

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a circle wireframe.

```cpp
void cf_draw_circle2(CF_Circle circle, int iters, float thickness);
```

Parameters | Description
--- | ---
circle | The circle.
iters | Number of edges used for the circle. More looks smoother, but renders slower.
thickness | The thickness of each line to draw.

## Related Pages

[cf_draw_circle](/draw/cf_draw_circle.md)  
[cf_draw_circle_fill](/draw/cf_draw_circle_fill.md)  
[cf_draw_circle_arc](/draw/cf_draw_circle_arc.md)  
[cf_draw_circle_arc_fill](/draw/cf_draw_circle_arc_fill.md)  
cf_draw_to  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
