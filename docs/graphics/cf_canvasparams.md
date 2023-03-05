# CF_CanvasParams | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

A texture the GPU can draw upon (with an optional depth/stencil texture).

Struct Members | Description
--- | ---
`const char* name` | The name of the canvas, for debug purposes.
`CF_Texture target` | The texture used to store pixel information when rendering to the canvas. See [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md).
`CF_Texture depth_stencil_target` | The texture used to store depth and stencil information when rendering to the canvas. See [CF_Texture](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_texture.md).

## Remarks

The clear color settings are used when [cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md) is called. You can change the clear color
by calling [cf_clear_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_clear_color.md). Usually you will not need to create a canvas at all, as it's an advanced feature for
users who want to draw to an off-screen buffer. Use cases can include rendering reflections, advanced lighting
techniques, or other kinds of multi-pass effects.

## Related Pages

[cf_clear_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_clear_color.md)  
[cf_canvas_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_canvas_defaults.md)  
[cf_make_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_make_canvas.md)  
[cf_destroy_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_destroy_canvas.md)  
[cf_apply_canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_canvas.md)  
