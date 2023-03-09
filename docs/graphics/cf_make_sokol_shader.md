[](../header.md ':include')

# CF_MAKE_SOKOL_SHADER

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Creates a shader from a shader compiled by sokol-shdc.

```cpp
#define CF_MAKE_SOKOL_SHADER(prefix) \
```

Parameters | Description
--- | ---
prefix | The name of your sokol-shdc compiled shader. See remarks for details.

## Remarks

There's an industry-wide problem where cross-platform shaders are difficult to setup. We have many
different shading languages and many different devices to deal with, but very little work has gone
into making high-quality and easy to use shader solutions. Most shader cross-compilers are way too
complex and riddled with giant dependencies, making them a poor fit for CF's style.

The best option (besides writing our own cross-compiler) is to use sokol_gfx.h, a very well written
thin wrapper around low-level 3D APIs. It supports a variety of backends:

 - Metal
 - OpenGL Core 3.3
 - OpenGL ES2
 - OpenGL ES3
 - D3D11
 - WebGPU

This lets CF run basically anywhere, including phones and web browsers. In the future SDL (Simple
Direct Media Library) will implement a GPU API that exposes a shader compiler. But until then we're
stuck using an offline compiler solution. It's still a pretty good solution though! It just means
a little extra work to generate shaders.

Cute Framework comes with compatible binaries Windows, Linux and MacOS to compile shaders onto
all supported platforms using the tool [sokol-shdc](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md). They are found in the `tools` folder.
The basic idea is to write your shader _one time_ in GLSL, then sokol-shdc will cross-compiler the shader
into a header file that's compatible with all supported backends.

Just make sure to call the sokol-shdc compiler with the `--reflection` parameter. Once done, `my_shader.h`
is ready to go! Include `my_shader.h` and get a [CF_SokolShader](/graphics/cf_sokolshader.md) with a single call to [cf_make_shader](/graphics/cf_make_shader.md).

```cpp
#include "my_shader.h"
CF_Shader my_shd = CF_MAKE_SOKOL_SHADER(my_shader);
```

## Related Pages

[CF_Material](/graphics/cf_material.md)  
[CF_SokolShader](/graphics/cf_sokolshader.md)  
[CF_Shader](/graphics/cf_shader.md)  
[cf_make_shader](/graphics/cf_make_shader.md)  
[cf_destroy_shader](/graphics/cf_destroy_shader.md)  
[cf_apply_shader](/graphics/cf_apply_shader.md)  
