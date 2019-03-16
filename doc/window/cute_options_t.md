# cute_options_t

An emuration for all the options to initialize the Cute framework.

## Values

Enumeration Entry | Description
CUTE_OPTIONS_NO_GFX | Skips all graphics initialization. 
CUTE_OPTIONS_NO_AUDIO | Skips all audio initialization.
CUTE_OPTIONS_FULLSCREEN | Starts up with a fullscreen window.
CUTE_OPTIONS_GFX_GL | Initializes an OpenGL 3.2 context.
CUTE_OPTIONS_GFX_GLES | Initialize an OpenGL ES 2.0 context.
CUTE_OPTIONS_GFX_D3D9 | Initializes a DirecX 9 context.

## Code Example

```
uint32_t options = CUTE_OPTIONS_NO_GFX
cute_t* cute = cute_make(title, x, y, w, h, options);
```

## Remarks

The `options` parameter of [cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/window/cute_make.md) defaults to `0`. The default behavior is to initialize a DirectX 9 context on Windows, OpenGL 3.2 context on Mac/Linux, and OpenGL ES 2.0 context on mobile devices (like iOS and Android).

## Related Functions

---

enumerations, window
