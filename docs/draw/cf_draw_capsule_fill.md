[](../header.md ':include')

# cf_draw_capsule_fill

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a capsule.

```cpp
CF_API void CF_CALL cf_draw_capsule_fill(CF_V2 p0, CF_V2 p1, float r, int iters);
```

Parameters | Description
--- | ---
p0 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
p1 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
r | Radius of the capsule.
iters | Number of edges used for the circle-caps. More looks smoother, but renders slower.

## Related Pages

[cf_draw_capsule](/draw/cf_draw_capsule.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
cf_draw_to  
