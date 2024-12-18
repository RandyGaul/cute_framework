[](../header.md ':include')

<br>

CF uses SDL_Gpu under the hood for rendering.
SDL_Gpu will, at some point, offer their own [shader tools](https://github.com/libsdl-org/SDL_shader_tools) to provide an easy way to support cross-platform shaders. For now, CF has its own shader tooling based on [glslang](https://github.com/KhronosGroup/glslang) as a temporary solution, to be removed once SDL_Gpu shader tools gets going.

## Runtime Shader Compilation

CF compiles shaders of the format GLSL 450. Shaders can be compiled at runtime, or precompiled into bytecode blobs. By default shaders are compiled at runtime, though this is adjustable via by the CMake option `CF_RUNTIME_SHADER_COMPILATION` (`ON` by default).
You can compile a shader by calling [`cf_make_shader_from_source`](/graphics/cf_make_shader_from_source.md).

!> **Note** Unfortunately, when runtime shaders are enabled the build process will involve pulling in [glslang](https://github.com/KhronosGroup/glslang), which requires Python 3.x installation on your machine.

To remove the online compiler from CF, set `CF_RUNTIME_SHADER_COMPILATION` to `OFF`.
Take note that online compilation functions such as [`cf_make_shader_from_source`](/graphics/cf_make_shader_from_source.md) will always fail if this is the case. Turning off runtime shader compilation can dramatically reduce the size and complexity of building CF. This is because [glslang](https://github.com/KhronosGroup/glslang) and similar alternatives to SDL's (unfinished) [shader tools](https://github.com/libsdl-org/SDL_shader_tools) are heavily bloated libraries, not written for fast compilation or load times. These will be removed at a later date from CF once SDL's shader tools get going.

## Precompiling Shaders

CF provides its own offline compiler called `cute-shaderc`.

You may want to disable runtime shader compilation, and instead precompile your shaders. Here are some reasons why this may make sense for you:

- Reduce compilation size and time of CF itself
- Reduce the time it takes to load shaders when your game is running
- Reduce the number of dependencies your build process involves (reduced bug/breakage risk)

!> **Note** By default (when using CMake) `CF_CUTE_SHADERC` is set to `ON` which will output `cute-shaderc`, an executable for precompiling shaders, in the same directory as where the `cute` library is placed when building CF. You may freely take a copy of `cute-shaderc` and place/use it wherever you like to support precompiled shaders.

```
Usage: cute-shaderc [options] <input>
Compile GLSL into SPIRV bytecode and/or generate a C header for embedding.

--help             Print this message.
-I<dir>            Add directory to #include search path.
-type=<type>       The shader type. Valid values are:
                   * draw (default): Draw shader for `cf_make_draw_shader_from_bytecode`.
                   * vertex: Standalone vertex shader for `cf_make_shader_from_bytecode`.
                   * fragment: Standalone fragment shader for `cf_make_shader_from_bytecode`.
-oheader=<file>    Where to write the C header file.
                   Also requires -varname.
-varname=<file>    The variable name inside the C header.
-obytecode=<file>  (Optional) Where to write the raw SPIRV blob.

Example (compiles my_shader.glsl to a C header):
cute-shaderc -I./my_shaders -type=draw -oheader=my_shader.h -varname=my_shader my_shader.shd
```

`-oheader=` indicates where you want to output the header file.

`-varname=` indicates the name of the static variable of the type [`CF_ShaderBytecode`](/graphics/cf_shaderbytecode.md).
This variable will be defined in the generated header.
It can be passed to related shader functions (explained below).

The `-type=` flag indicates which type of shader you want to compile:

* `vertex` and `fragment` are for compiling [low level shaders](https://randygaul.github.io/cute_framework/#/topics/low_level_graphics?id=shaders).
  The result should be passed into [`cf_make_shader_from_bytecode`](/graphics/cf_make_shader_from_bytecode.md).
* `draw` is for compiling [custom draw shaders](https://randygaul.github.io/cute_framework/#/topics/drawing?id=shaders).
  The result should be passed into [`cf_make_draw_shader_from_bytecode`](/draw/cf_make_draw_shader_from_bytecode.md).

The `-I` flag will be explained in the "Shader inclusion" section below.

In case you need the raw SPIRV blob, `-obytecode=` can also be used.
Take note that this is only available for shaders of type `vertex` or `fragment`.

## Working with `cute-shaderc`

Typically you will choose to either use runtime, or precompiled, shaders in your project and never deviate from this choice. However, if for some reason you want your project to support both you can use the C preprocessor as shown below to support both online/offline shader compilation. You may want to support both styles during development, as runtime compilation may be preferred during development to faster iteration times when modifying shaders.

To detect compilation setting, CF defines the macro `CF_RUNTIME_SHADER_COMPILATION` when online compilation is enabled.
In order to make your code compatible with both options (`ON` and `OFF`), use the following idiom:

```c
#ifndef CF_RUNTIME_SHADER_COMPILATION // When runtime shader compilation is disabled
#include "custom_draw_shd.h" // Include the header generated by cute-shaderc
#endif

int main() {
#ifdef CF_RUNTIME_SHADER_COMPILATION // If runtime compilation is enabled
    // Load shader from disk
    cf_shader_directory("/shaders");
    CF_Shader custom_draw_shader = cf_make_draw_shader("custom_draw.shd");
#else // If runtime compilation is disabled
    // Use precompiled shader
    CF_Shader custom_draw_shader = cf_make_draw_shader_from_bytecode(s_custom_draw_shd);
#endif
}
```

During development you can edit the shader source file directly for faster iteration.
When you need to ship, your game can use the precompiled version.

In order to keep the generated header in sync, add the following into your `CMakeLists.txt`:

```cmake
if (NOT CF_RUNTIME_SHADER_COMPILATION)
    add_custom_command(
        # Replace with where you want to output the header
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/custom_draw_shd.h
        COMMAND cute-shaderc  # The path is set automatically by CMake
            # Replace with your shader directory
            -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            # Other types are possible
            -type=draw
            # What you want the variable name to be
            -varname=s_flash_shd_bytecode
            # Repeat the path above
            -oheader=${CMAKE_CURRENT_SOURCE_DIR}/src/custom_draw_shd.h
            # Replace with path to the shader file
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/custom_draw.shd
        DEPENDS shaders/custom_draw.shd  # Rebuild when the shader source change
        DEPENDS cute-shaderc  # Rebuild when the compiler is updated
    )
endif ()

# Make sure that your program depends on the generated header
set(SOURCES
    # Other source files
    src/main.c
    # Generated headers
    src/custom_draw_shd.h
)
add_executable(my_game SOURCES)
```

## Minimal Build (Turning Off Runtime Shader Compilation)

If you do not need custom shaders then set both `CF_RUNTIME_SHADER_COMPILATION` and `CF_CUTE_SHADERC` to `OFF`. This will reduce build and load times for CF and shaders. You can still use all other CF features.

You may want to use a minimal build if you plan to build a copy of `cute-shaderc` and call it on your own, managing shader compilation outside of building CF.

## Shader Inclusion `#include`

To make reusable utility functions, CF supports shaders including each other with the `#include` directive.

"Include guard", usually seen in C/C++, is not needed (e.g. `#pragma once`). Each file will only be included once and subsequent inclusions are ignored.

With online compilation, the include directory must be set with [`cf_shader_directory`](/graphics/cf_shader_directory.md). For example: `cf_shader_directory("/shaders")`. Take note that this is a path in the [VFS](/topics/virtual_file_system.md), hence, the leading slash ('/'). When shaders inclusions occur they always search relative to this shader directory, and *never* search outside of it. You may organize your shaders _within the shader directory_ however you like, but they cannot exist outside the shader directory.

With offline compilation, the include directory is set with the `-I` flag. You run the shader compiler on the command line, after building the shader compiler `scute-shaderc`. For example: `cute-shaderc -Ishaders -o src/my_shader_shd.h my_shader.shd`. Take note that this is a path in your actual filesystem, and not a path in the [VFS](/topics/virtual_file_system.md).
The include directory is relative to wherever you run the command.

When using CMake, prefix the path with `${CMAKE_CURRENT_SOURCE_DIR}` to make it independent of the build directory.

CF also provides several builtin utility modules: `gamma.shd`, `distance.shd`, `smooth_uv.shd`, `blend.shd`.
These can always be `#include`-d by your shader without setting the include path. You can view these files by looking at CF's source code to see what sort of extra helper functions are available for use in your shaders.

## Draw Shader Quirks

Due to the way [custom draw shaders](https://randygaul.github.io/cute_framework/#/topics/drawing?id=shaders) are compiled, any errors in your shader will be reported as being from `shader_stub.shd`.

Do not look for this file and just take it as the errors are coming from whatever shader you are trying to compile.

## Migrating to SDL_GPU official shader tool

The current shader tooling is temporary until SDL's own [shader tools](https://github.com/libsdl-org/SDL_shader_tools) are mature enough. It is still in early development so we do not know what will change. To ensure as little friction as possible during migration, the following practice is advised.

The `CF_ShaderBytecode` struct, whether coming from [`cf_compile_shader_to_bytecode`](/graphics/cf_compile_shader_to_bytecode.md) or the `cute-shaderc` compiler should be treated as opaque. It should not be modified in anyway and only passed verbatim to related functions: [`cf_make_shader_from_bytecode`](/graphics/cf_make_shader_from_bytecode.md) and [`cf_make_draw_shader_from_bytecode`](/draw/cf_make_draw_shader_from_bytecode.md).

There is no guarantee on how the inner structure may change but the signature of the above functions will remain the same regardless of compilation backend. In other words, the API should remain stable at source level but there is no guarantee on the ABI (binary data compatibility with future versions of SDL).

`cute-shaderc` will still be provided as an offline compilation tool. Its various flags and their behaviours will remain the same. The generated header will still declare a variable of the type [`CF_ShaderBytecode`](/graphics/cf_shaderbytecode.md). The actual content of the output, however, might change with compilation backend.
