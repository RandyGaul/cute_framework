[](../header.md ':include')

# cf_draw_circle_arc

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws an arc of a circle wireframe.

```cpp
void cf_draw_circle_arc(CF_V2 p, CF_V2 center_of_arc, float range, int iters, float thickness);
```

Parameters | Description
--- | ---
p | Center of the arc.
center_of_arc | Radius of the circle.
range | Angle the arc covers.
iters | Number of edges used for the circle. More looks smoother, but renders slower.
thickness | The thickness of each line to draw.

## Related Pages

[cf_draw_circle](/draw/cf_draw_circle.md)  
[cf_draw_circle_fill](/draw/cf_draw_circle_fill.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_draw_circle_arc_fill](/draw/cf_draw_circle_arc_fill.md)  
cf_draw_to  
