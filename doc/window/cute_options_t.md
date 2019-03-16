# cute_options_t

An emuration for all the options to initialize the Cute framework.

## Values

Enumeration Entry | Description
--- | ---
CUTE_OPTIONS_NO_GFX | Skips all graphics initialization. 
CUTE_OPTIONS_NO_AUDIO | Skips all audio initialization.
CUTE_OPTIONS_FULLSCREEN | Starts up with a fullscreen window.
CUTE_OPTIONS_GFX_GL | Initializes an OpenGL 3.2 context (not mobile-friendly).
CUTE_OPTIONS_GFX_GLES | Initialize an OpenGL ES 2.0 context (mobile devices only).
CUTE_OPTIONS_GFX_D3D9 | Initializes a DirecX 9 context (Windows only).

## Code Example

```cpp
// Initialize Cute with an OpenGL 3.2 context, but no audio.
uint32_t options = CUTE_OPTIONS_GFX_GL | CUTE_OPTIONS_NO_AUDIO;
cute_t* cute = cute_make(title, x, y, w, h, options);
```

## Remarks

The `options` parameter of [cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_make.md) defaults to `0`. The default behavior is to initialize a DirectX 9 context on Windows, OpenGL 3.2 context on Mac/Linux, and OpenGL ES 2.0 context on mobile devices (like iOS and Android). The default behavior also initializes audio. To select a specific graphics context and override the default behavior, OR one of the GFX enumerations onto your options.

Cute can be initialized without any graphics or audio (for example, to implement a game server) by passing in `CUTE_OPTIONS_NO_GFX` or `CUTE_OPTIONS_NO_AUDIO` in the `options` parameter of [cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_make.md).

## Related Functions

[cute_destroy](https://github.com/RandyGaul/cute_framework/tree/master/doc/window/cute_destroy.md)
