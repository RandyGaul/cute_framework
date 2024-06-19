[](../header.md ':include')

# cf_canvas_blit

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Blits one canvas onto another.

```cpp
void cf_canvas_blit(CF_Canvas src, CF_V2 u0, CF_V2 v0, CF_Canvas dst, CF_V2 u1, CF_V2 v1);
```

Parameters | Description
--- | ---
src | The source texture to copy pixels from.
u0 | The normalized coordinate of the top-left of the source rect.
v0 | The normalized coordinate of the bottom-right of the source rect.
src | The canvas where pixels are copied from.
u1 | The normalized coordinate of the top-left of the destination rect.
v1 | The normalized coordinate of the bottom-right of the destination rect.
dst | The destination canvas where pixels are copied to.

## Remarks

The texture formats of the underlying canvas's must be PIXELFORMAT_DEFAULT. Each u/v coordinate
is normalized, meaning a number from 0 to 1. This lets the function operate on canvas's of any
size. To convert a coordinate to a normalized coordinate, simply divide the x/y of your coordinate
by the width/height of the canvas.

## Related Pages

[CF_Canvas](/graphics/cf_canvas.md)  
