[](../header.md ':include')

# cf_make_shader

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Creates a shader from a shader compiled by sokol-shdc.

```cpp
CF_Shader cf_make_shader(CF_SokolShader sokol_shader);
```

Parameters | Description
--- | ---
sokol_shader | A compiled shader.

## Remarks

You should instead call [CF_MAKE_SOKOL_SHADER](/graphics/cf_make_sokol_shader.md) unless you really know what you're doing.

## Related Pages

[CF_MAKE_SOKOL_SHADER](/graphics/cf_make_sokol_shader.md)  
[CF_SokolShader](/graphics/cf_sokolshader.md)  
[CF_Shader](/graphics/cf_shader.md)  
[CF_Material](/graphics/cf_material.md)  
[cf_destroy_shader](/graphics/cf_destroy_shader.md)  
[cf_apply_shader](/graphics/cf_apply_shader.md)  
