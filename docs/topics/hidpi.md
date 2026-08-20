# High-DPI Rendering

Modern displays (Apple Retina, many 4K laptops, phones) pack several physical pixels into a single logical "point". Cute Framework handles this for you: your game works in **logical points**, while CF renders internally at the display's **physical pixel** resolution. You get crisp output on high-density displays without changing any of your drawing code.

## Points vs. Pixels

CF's public API is expressed in logical points. Window size ([`cf_app_get_size`](../app/function/cf_app_get_size.md)), draw coordinates, and the [camera](../topics/camera.md) all work in points. A 640x480 window is 640x480 points no matter which display it lives on.

Under the hood CF sizes its default canvas in *physical* pixels: `logical_size * pixel_scale`. On a 2x Retina display a 640x480 window renders into a 1280x960 canvas. The projection still spans 640x480 logical units, so nothing you draw moves — there are simply more physical texels to rasterize text glyphs and SDF shape edges into, which is what makes them look sharp.

Because of this you generally don't need to think about DPI at all. Draw in points; CF renders at native resolution.

## `pixel_scale` vs. `display_scale`

Two functions report display density, and they're easy to confuse:

- [`cf_app_get_pixel_scale`](../app/function/cf_app_get_pixel_scale.md) — the number of physical pixels per logical point, e.g. `2.0f` on a 2x Retina display. This is the ratio CF actually renders at, and the one to multiply a logical size by to get physical pixels.
- [`cf_app_get_display_scale`](../app/function/cf_app_get_display_scale.md) — the OS's *suggested* UI content scale. It is informational only and does **not** describe the rendering ratio. Most games can ignore it.

When you need to convert between points and physical pixels — for example, sizing an offscreen canvas to match the swapchain — use `cf_app_get_pixel_scale`.

## How the default canvas resizes

CF recreates the app's default canvas at `window_size * pixel_scale` physical pixels on every *canvas recreation event*: a window resize, moving to a display with a different pixel density, [`cf_app_set_size`](../app/function/cf_app_set_size.md), or [`cf_app_set_msaa`](../app/function/cf_app_set_msaa.md).

[`cf_app_set_canvas_size`](../app/function/cf_app_set_canvas_size.md) resizes the canvas to an exact pixel size as a **one-shot override** — the custom size lasts only until the next recreation event snaps the canvas back to `window_size * pixel_scale`. To hold a custom size (say, rendering at half resolution to save fill-rate), just re-apply it whenever the canvas no longer matches your intended size, as demonstrated in the [Canvas Modes](../samples/canvas_modes.md) sample.

## Retro / pixel-art rendering

Sometimes a low resolution is the whole point. For a persistent fixed-resolution target, make your own canvas and scale it up onto the screen each frame:

```cpp
// Startup: a 320x180 canvas, nearest-filtered so the upscale stays blocky.
CF_CanvasParams params = cf_canvas_defaults(320, 180);
params.target.filter = CF_FILTER_NEAREST;
CF_Canvas canvas = cf_make_canvas(params);

// Each frame: draw the scene at 320x180, then letterbox it up to the window.
cf_draw_projection(cf_ortho_2d(0, 0, 320, 180));
draw_game();
cf_render_to(canvas, true);

int window_w, window_h;
cf_app_get_size(&window_w, &window_h);
cf_draw_projection(cf_ortho_2d(0, 0, (float)window_w, (float)window_h));
float scale = fminf(window_w / 320.0f, window_h / 180.0f);
cf_draw_canvas(canvas, cf_v2(0, 0), cf_v2(320 * scale, 180 * scale));
```

Unlike the app's canvas, a canvas you make yourself never resizes behind your back, and the upscale is a deliberate stylistic choice rather than accidental blur. See [Canvas Modes](../samples/canvas_modes.md) for a live comparison of all of these setups, and [Window Resizing](../samples/windowresizing.md) for letterbox/crop/stretch scaling variants.

## Opting out entirely

To render at 1:1 logical-to-physical (skipping the high-density backbuffer altogether — e.g. to save fill-rate on a low-end device), pass [`CF_APP_OPTIONS_NO_HIGH_DPI_BIT`](../app/enum/cf_appoptionflagbits.md) when creating the app. [`cf_app_get_pixel_scale`](../app/function/cf_app_get_pixel_scale.md) then always returns `1.0f`.

```cpp
cf_make_app("My Game", 0, 0, 0, 640, 480, CF_APP_OPTIONS_NO_HIGH_DPI_BIT, argv[0]);
```

## Sample

See [samples/hidpi.c](https://github.com/RandyGaul/cute_framework/blob/master/samples/hidpi.c) for a runnable demonstration: text at several sizes, a row of SDF shapes, and a live [`cf_app_get_pixel_scale`](../app/function/cf_app_get_pixel_scale.md) readout to eyeball crispness on your own display. For an interactive comparison of canvas sizing setups (default, custom scale, forced 1x, fixed-resolution retro canvas) see [samples/canvas_modes.c](https://github.com/RandyGaul/cute_framework/blob/master/samples/canvas_modes.c).
