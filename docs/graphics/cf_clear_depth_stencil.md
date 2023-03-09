[](../header.md ':include')

# cf_clear_depth_stencil

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets the depth/stencil values used when clearing a canvas, if depth/stencil are enabled (see [CF_RenderState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_renderstate.md)).

```cpp
void cf_clear_depth_stencil(float depth, float stencil);
```

## Remarks

This will get used when [cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md) or when [cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md) is called.

## Related Pages

[cf_clear_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_clear_color.md)  
[cf_clear_color2](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_clear_color2.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
[cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md)  
