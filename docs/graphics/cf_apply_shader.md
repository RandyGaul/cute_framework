# cf_apply_shader

Category: [graphics](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Uses a specific shader + material combo for rendering.

```cpp
void cf_apply_shader(CF_Shader shader, CF_Material material);
```

## Remarks

The [CF_Shader](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_shader.md) defines how to render a mesh's geometry, set by [cf_apply_mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_apply_mesh.md). The [CF_Mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh.md) holds input geometry to the
vertex shader. A [CF_Material](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_material.md) defines uniform and texture inputs to the shader.

## Related Pages

[CF_Mesh](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_mesh.md)  
cf_create_mesh  
[cf_draw_elements](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_draw_elements.md)  
