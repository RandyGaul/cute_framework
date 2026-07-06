# High-DPI Rendering

Modern displays (Apple Retina, many 4K laptops, phones) pack several physical pixels into a single logical "point". Cute Framework handles this for you: your game works in **logical points**, while CF renders internally at the display's **physical pixel** resolution. You get crisp output on high-density displays without changing any of your drawing code.

## Points vs. Pixels

CF's public API is expressed in logical points. Window size ([`cf_app_get_size`](../app/cf_app_get_size.md)), draw coordinates, and the [camera](../topics/camera.md) all work in points. A 640x480 window is 640x480 points no matter which display it lives on.

Under the hood CF sizes its default canvas in *physical* pixels: `logical_size * pixel_scale`. On a 2x Retina display a 640x480 window renders into a 1280x960 canvas. The projection still spans 640x480 logical units, so nothing you draw moves — there are simply more physical texels to rasterize text glyphs and SDF shape edges into, which is what makes them look sharp.

Because of this you generally don't need to think about DPI at all. Draw in points; CF renders at native resolution.

## `pixel_scale` vs. `dpi_scale`

Two functions report display density, and they're easy to confuse:

- [`cf_app_get_pixel_scale`](../app/cf_app_get_pixel_scale.md) — the number of physical pixels per logical point, e.g. `2.0f` on a 2x Retina display. This is the ratio CF actually renders at, and the one to multiply a logical size by to get physical pixels.
- [`cf_app_get_dpi_scale`](../app/cf_app_get_dpi_scale.md) — the OS's *suggested* UI content scale. It is informational only and does **not** describe the rendering ratio. Most games can ignore it.

When you need to convert between points and physical pixels — for example, sizing an offscreen canvas to match the swapchain — use `cf_app_get_pixel_scale`.

## Retro / pixel-art canvas

Sometimes a low resolution is the whole point. For an intentional retro or pixel-art look, call [`cf_app_set_canvas_size`](../app/cf_app_set_canvas_size.md) to pin the app's default canvas to an exact device-pixel size:

```cpp
// Render at a fixed 320x180, then scale up to fill the window.
cf_app_set_canvas_size(320, 180);
```

Pinning opts that canvas out of CF's automatic pixel-scale sizing, so it won't auto-resize when the window moves to a display with a different density. The upscale to fill the window becomes a deliberate stylistic choice rather than accidental blur.

## Opting out entirely

To render at 1:1 logical-to-physical (skipping the high-density backbuffer altogether — e.g. to save fill-rate on a low-end device), pass [`CF_APP_OPTIONS_NO_HIGH_DPI_BIT`](../app/cf_appoptionflagbits.md) when creating the app. [`cf_app_get_pixel_scale`](../app/cf_app_get_pixel_scale.md) then always returns `1.0f`.

```cpp
cf_make_app("My Game", 0, 0, 0, 640, 480, CF_APP_OPTIONS_NO_HIGH_DPI_BIT, argv[0]);
```

## Sample

See [samples/hidpi.c](https://github.com/RandyGaul/cute_framework/blob/master/samples/hidpi.c) for a runnable demonstration: text at several sizes, a row of SDF shapes, and a live [`cf_app_get_pixel_scale`](../app/cf_app_get_pixel_scale.md) readout to eyeball crispness on your own display.
