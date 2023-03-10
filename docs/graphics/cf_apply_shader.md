[](../header.md ':include')

# cf_apply_shader

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Uses a specific shader + material combo for rendering.

```cpp
CF_API void CF_CALL cf_apply_shader(CF_Shader shader, CF_Material material);
```

## Remarks

The [CF_Shader](/graphics/cf_shader.md) defines how to render a mesh's geometry, set by [cf_apply_mesh](/graphics/cf_apply_mesh.md). The [CF_Mesh](/graphics/cf_mesh.md) holds input geometry to the
vertex shader. A [CF_Material](/graphics/cf_material.md) defines uniform and texture inputs to the shader.

## Related Pages

[CF_Mesh](/graphics/cf_mesh.md)  
cf_create_mesh  
[cf_draw_elements](/graphics/cf_draw_elements.md)  
