# CF_StencilParams | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Settings for the stencil buffer.

Struct Members | Description
--- | ---
`bool enabled` | The stencil buffer will not be used unless this is true.
`uint8_t read_mask` | Used to control which bits get read from the stencil buffer.
`uint8_t write_mask` | Used to control which bits get written to the stencil buffer.
`uint8_t reference` | After reading from the stencil buffer, the `reference` value is used in a comparison to perform a stencil operation. See [CF_StencilFunction](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilfunction.md) and [CF_StencilOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilop.md).
`CF_StencilFunction front` | The stencil function to use for front-facing triangles (counter-clockwise).
`CF_StencilFunction back` | The stencil function to use for back-facing triangles (clockwise).

## Remarks

For an overview of stencil testing [learnopengl.com has an excellent article](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) on the topic.

## Related Pages

[CF_StencilFunction](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilfunction.md)  
[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[CF_RenderState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_renderstate.md)  
