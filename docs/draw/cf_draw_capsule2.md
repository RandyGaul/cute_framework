[](../header.md ':include')

# cf_draw_capsule2

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws a capsule wireframe.

```cpp
void cf_draw_capsule2(CF_V2 p0, CF_V2 p1, float r, float thickness);
```

Parameters | Description
--- | ---
p0 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
p1 | An endpoint of the interior line-segment of the capsule (the center of one end-cap).
r | Radius of the capsule.
thickness | The thickness of each line to draw.

## Related Pages

[cf_draw_capsule](/draw/cf_draw_capsule.md)  
[cf_draw_capsule_fill2](/draw/cf_draw_capsule_fill2.md)  
[cf_draw_capsule_fill](/draw/cf_draw_capsule_fill.md)  
