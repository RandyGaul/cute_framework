# CF_StencilFunction | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics_readme.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Defines functions for stencil rendering.

Struct Members | Description
--- | ---
`CF_CompareFunction compare` | Comparison type for the stencil test. See [CF_CompareFunction](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_comparefunction.md).
`CF_StencilOp fail_op` | An operation to perform upon failing a stencil test. See [CF_StencilOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilop.md).
`CF_StencilOp depth_fail_op` | An operation to perform upon failing a depth test. See [CF_StencilOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilop.md).
`CF_StencilOp pass_op` | An operation to perform upon passing a stencil test. See [CF_StencilOp](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilop.md).

## Remarks

The stencil buffer stores references values used for rendering with comparisons. Only comparisons that end up
logically true pass the stencil test and end up getting drawn. For an overview of stencil testing [learnopengl.com
has an excellent article](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) on the topic.

## Related Pages

[cf_material_set_render_state](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material_set_render_state.md)  
[CF_StencilParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_stencilparams.md)  
[CF_RenderState](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_renderstate.md)  
