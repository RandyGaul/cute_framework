[](../header.md ':include')

# cf_draw_circle_fill

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a circle.

```cpp
CF_API void CF_CALL cf_draw_circle_fill(CF_V2 p, float r, int iters);
```

Parameters | Description
--- | ---
p | Center of the circle.
r | Radius of the circle.
iters | Number of edges used for the circle. More looks smoother, but renders slower.

## Related Pages

[cf_draw_circle](/draw/cf_draw_circle.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_draw_circle_arc](/draw/cf_draw_circle_arc.md)  
[cf_draw_circle_arc_fill](/draw/cf_draw_circle_arc_fill.md)  
cf_draw_to  
