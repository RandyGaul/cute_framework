[](../header.md ':include')

# cf_apply_viewport

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets up a viewport to render within.

```cpp
void cf_apply_viewport(int x, int y, int width, int height);
```

Parameters | Description
--- | ---
x | Center of the viewport on the x-axis.
y | Center of the viewport on the y-axis.
width | Width of the viewport in pixels.
height | Height of the viewport in pixels.

## Remarks

The viewport is a window on the screen to render within. The canvas will be stretched to fit onto the viewport.

## Related Pages

[cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md)  
[cf_apply_scissor](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_scissor.md)  
