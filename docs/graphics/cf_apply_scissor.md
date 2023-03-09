# cf_apply_scissor

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets up a scissor box to clip rendering within.

```cpp
void cf_apply_scissor(int x, int y, int width, int height);
```

Parameters | Description
--- | ---
x | Center of the scissor box on the x-axis.
y | Center of the scissor box on the y-axis.
width | Width of the scissor box in pixels.
height | Height of the scissor box in pixels.

## Remarks

The scissor box is a window on the screen that rendering will be clipped within. Any rendering that occurs outside the
scissor box will simply be ignored, rendering nothing and leaving the previous pixel contents untouched.

## Related Pages

[cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md)  
[cf_apply_viewport](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_viewport.md)  
