# Sokol GFX

Cute uses a high-quality library called [sokol_gfx.h](https://github.com/floooh/sokol) as an abstraction over modern 3D rendering APIs. Since each platform typically requires, or prefers, different rendering APIs a wrapper around all APIs is really valuable. Especially for writing portable code. Rather than reinventing this solution, Cute directly incorporates sokol_gfx as a first-class citizen, and encourages users to directly call into sokol_gfx on an as-needed basis for all custom rendering needs.

For basic sprite drawing Cute's sprite batching system internally uses sokol_gfx. Cute's pixel art upscale feature also makes use of sokol_gfx for the render to texture and scaling effect.

Learning how to use Sokol GFX isn't necessary unless you want to start implementing your own custom shader effects. In this case there are [some tutorials online](https://floooh.github.io/2017/07/29/sokol-gfx-tour.html) available! However, the best place to learn about sokol_gfx.h is to look at the [source code directly](https://github.com/RandyGaul/cute_framework/blob/master/include/sokol/sokol_gfx.h), as the code is *heavily documented* with top-notch documentation.

# Sokol Shader Compiler

Cute comes packaged with the Sokol shader compiler under /tools/sokol-shdc. This tool can run on Windows/Mac/Linux, and cross compiles GLSL code into various backend formats, such as D3D11, OpenGL ES, and more. Running the compiler is quite simple and is [documented here](https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md).

If you're on Windows you can try using this `.cmd` script. Simply copy + paste this into a `.cmd` or `.bat` file, place that file into a folder with your `.glsl` files, and run the script. You might need to alter the path to `sokol-shdc.exe`.

```bat
@echo off
for %%f in ("*.glsl") do call :compile %%~nf
exit /B

:compile
@echo Compiling %~1.glsl into %~1_shader.h ...
call "../../tools/sokol-shdc/win32/sokol-shdc.exe" --input %~1.glsl --output %~1_shader.h --slang glsl330:hlsl5:metal_macos:glsl300es:glsl100
```

This will all glsl files in the current folder and compile them into cross-platform headers, ready to drop into your code base.
