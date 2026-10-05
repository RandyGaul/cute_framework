# WebGPU

Cute Framework has an optional WebGPU backend. Its main job is web builds: with it, a game in the browser renders through WebGPU instead of WebGL 2, and gets real compute shaders, real storage buffers, and indirect draws. The same backend also runs on desktop through [wgpu-native](https://github.com/gfx-rs/wgpu-native), so you can test and debug it without a browser.

The backend is off by default. A build without it is exactly the same as before.

## How It Works

- **Web**: CF talks to the browser's WebGPU through Emscripten's `emdawnwebgpu` port. The build still contains the regular [GLES3 backend](emscripten.md#gles3-backend-capabilities) as well, and CF picks between the two at startup.
- **Desktop**: CF talks to wgpu-native, which runs on top of Vulkan, D3D12, or Metal. This is meant for testing the WebGPU backend. To ship a desktop game, use the default SDL_GPU backends.

Your code doesn't change between backends. Draw calls, canvases, shaders, and the [low level graphics API](low_level_graphics.md) all work the same way.

## Enabling WebGPU

Turn on the `CF_WEBGPU` CMake option when configuring CF.

For the web, add it to your usual Emscripten configure step (see [Web Builds with Emscripten](emscripten.md) for setting up Emscripten itself):

```
emcmake cmake -S . -B build_web -G Ninja -DCMAKE_BUILD_TYPE=Release -DCF_WEBGPU=ON
cmake --build build_web
```

There is nothing else to install. The `emdawnwebgpu` port ships with Emscripten, and CF adds `--use-port=emdawnwebgpu` to your game's compile and link flags for you.

For desktop:

```
cmake -B build -DCF_WEBGPU=ON
cmake --build build
```

CMake downloads a pinned wgpu-native release while configuring, the same way CF fetches its other dependencies. If you'd rather use your own copy, set `CF_WGPU_NATIVE_DIR` to an unpacked wgpu-native release (the folder that contains `include/webgpu/webgpu.h` and `lib/`):

```
cmake -B build -DCF_WEBGPU=ON -DCF_WGPU_NATIVE_DIR=C:/libs/wgpu-native
```

wgpu-native is a shared library (`wgpu_native.dll`, `libwgpu_native.so`, or `libwgpu_native.dylib`) that your executable loads at startup. CF's own samples and tests get a copy automatically. On Windows the DLL has to sit next to your game's executable, and one way to do that is to copy every DLL your game links against after each build:

```cmake
if(WIN32)
	add_custom_command(TARGET your_game POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:your_game> $<TARGET_FILE_DIR:your_game>
		COMMAND_EXPAND_LISTS
	)
endif()
```

`CF_WEBGPU` is also a public compile definition, so your own code can check for it with `#ifdef CF_WEBGPU`.

## Picking the Backend at Runtime

**On the web** you don't have to do anything. When CF is built with `CF_WEBGPU`, it asks the browser for a WebGPU adapter at startup. If it gets one, the game runs on WebGPU. If it doesn't (the browser has no WebGPU, or it is turned off or blocked), or if WebGPU fails to start, the game runs on WebGL 2 instead. Passing `CF_APP_OPTIONS_GFX_OPENGL_BIT` to [`cf_make_app`](../app/function/cf_make_app.md) forces WebGL 2.

Since a player may land on either backend, keep your shaders within what the GLES3 backend supports if you want the game to run everywhere. The [GLES3 capability table](emscripten.md#gles3-backend-capabilities) lists those limits.

**On desktop** WebGPU is never picked on its own. Pass `CF_APP_OPTIONS_GFX_WEBGPU_BIT` to [`cf_make_app`](../app/function/cf_make_app.md):

```c
int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
#ifdef CF_WEBGPU
	options |= CF_APP_OPTIONS_GFX_WEBGPU_BIT;
#endif
CF_Result result = cf_make_app("My Game", 0, 0, 0, 640, 480, options, argv[0]);
```

There is no fallback on desktop: if wgpu-native can't find an adapter, `cf_make_app` returns an error. If CF was built without `CF_WEBGPU`, the bit is ignored with a warning and the default backend is used.

Either way, [`cf_query_backend`](../graphics/function/cf_query_backend.md) tells you which backend is running. It returns `CF_BACKEND_TYPE_WEBGPU` for WebGPU.

> [!NOTE]
> If the GPU device is lost while the game is running (for example the browser resets the GPU), CF logs it once and stops drawing. It does not switch to WebGL 2 at that point.

## Capabilities

| Feature | SDL_GPU (Vulkan/D3D/Metal) | WebGPU | GLES3 / WebGL2 |
| --- | --- | --- | --- |
| Draw API (2D + 3D), meshes, canvases, MRT | ✔ | ✔ | ✔ |
| Depth/comparison samplers, cube/3D/array textures | ✔ | ✔ | ✔ |
| Render into faces/layers/mips (`attach_target`) | ✔ | ✔ | ✔ |
| Per-target blend states | ✔ | ✔ | ✘ (all targets share `blends[0]`) |
| Standalone samplers (`CF_Sampler`) | ✔ | ✔ | ✔ |
| Range draws / geometry arenas | ✔ | ✔ | ✔ |
| Storage buffers (read-only, vertex/fragment) | ✔ | ✔ | ✔ (emulated) |
| Compute shaders | ✔ | ✔ | ✔ (emulated, with limits) |
| GPU-writable storage (`compute_writable`) | ✔ | ✔ | ✔ (emulated, with limits) |
| Indirect draws | ✔ | ✔ | ✘ |
| Dear ImGui | ✔ | ✔ | ✔ |
| `cf_push_gpu_label` capture regions | ✔ | no-op | no-op |

The GLES3 column's emulations and their limits are described on the [Emscripten page](emscripten.md#gles3-backend-capabilities).

## What's Not Supported

WebGPU itself is missing a few things the other backends have. Here is what behaves differently:

- **Pixel formats**: `CF_PIXEL_FORMAT_A8_UNORM` and the 16-bit packed formats (`B5G6R5`, `B5G5R5A1`, `B4G4R4A4`) have no WebGPU equivalent, so creating a texture with them fails. BC compressed formats, the 16-bit normalized formats, and `D32_FLOAT_S8_UINT` depend on the device. Check with [`cf_query_pixel_format`](../graphics/function/cf_query_pixel_format.md).
- **LOD bias**: a sampler's LOD bias is ignored. CF logs this once.
- **3D texture mipmaps**: [`cf_generate_mipmaps`](../graphics/function/cf_generate_mipmaps.md) skips 3D textures. CF logs this once.
- **Debug labels**: [`cf_push_gpu_label`](../graphics/function/cf_push_gpu_label.md) and `cf_pop_gpu_label` do nothing.
- **Depth clip**: turning off depth clipping (`enable_depth_clip = false`) needs a device feature. Without it the setting is ignored, and CF logs this once.
- **MSAA**: 1x and 4x always work. 2x and 8x only work on desktop when the device supports them, and never on the web. [`cf_app_set_msaa`](../app/function/cf_app_set_msaa.md) returns false for a sample count the backend can't do.

## Shaders

You write the same GLSL 450 shaders as on every other backend. CF's shader compiler, [cute_spirv](glsl_support.md), turns them into WGSL (WebGPU's shading language). Runtime compilation only produces WGSL when the app is running on WebGPU. Headers made by [`cute-shaderc`](shader_compilation.md#precompiling-shaders) include WGSL unless you pass `-nowgsl`.

You don't need to know how resources map to WGSL to use the backend, but it helps when reading generated WGSL or error messages:

- Each CF resource set `N` becomes one WGSL bind group, `@group(N)`.
- WGSL has no combined texture-and-sampler type, so each `sampler2D` (and friends) becomes a texture plus a sampler. Sampled texture `i` lands at `@binding(2i)` and its sampler at `@binding(2i+1)`. Storage textures come next, then storage buffers.
- Uniform block `u` lands at `@binding(u)` of its group.
- WebGPU only allows read+write storage textures for the `r32f`, `r32ui`, and `r32i` formats. A compute shader that both reads and writes an image of any other format gets the image split in two: a write-only storage texture, and a regular texture for reading. CF copies the image right before the dispatch and binds the copy as the read side, so reads see the image as it was before the dispatch, not values written during it.

## Testing on Desktop

The desktop backend runs the same code as the web one, which makes it the easy way to check WebGPU without a browser:

- Run the test suite on WebGPU by setting the environment variable `CF_TEST_WEBGPU=1` before running `tests`.
- Run the `hrc_gi` sample on WebGPU with `HRC_WEBGPU=1`.
- Pass `CF_APP_OPTIONS_GFX_DEBUG_BIT` along with `CF_APP_OPTIONS_GFX_WEBGPU_BIT` to print wgpu-native's warnings to the console.

CF's CI runs the whole test suite this way on Linux (wgpu-native over a software Vulkan driver), and builds the web version with `CF_WEBGPU=ON`.
