# cf_apply_mesh | [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/README.md) | [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)

Uses a specific mesh for rendering.

```cpp
void cf_apply_mesh(CF_Mesh mesh);
```

## Remarks

The mesh contains vertex data, defining the geometry to be rendered. The mesh vertices are sent to the GPU as inputs to
the vertex shader. See [CF_Mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh.md) for an overview.

## Related Pages

[CF_Mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh.md)  
cf_create_mesh  
[cf_apply_shader](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_shader.md)  
[cf_draw_elements](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_draw_elements.md)  
