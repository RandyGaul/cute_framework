# app_init_upscaling

Sets up the upscaling system. This is used for pixel art games that want to render to a small resolution and then upscale onto the screen at a higher resolution. For non-pixel art games `UPSCALE_STRETCH` can be used.

## Syntax

```cpp
error_t app_init_upscaling(app_t* app, upscale_t upscaling, int offscreen_w, int offscreen_h);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
upscaling | The upscale settings, remarks below for more details.

## Return Value

Returns an `error_t` containing any related error details.

## Remarks

`upscale_t` Enumeration | Description
--- | ---
UPSCALE_PIXEL_PERFECT_AT_LEAST_1X | Upscales to the maximum factor while still fitting within the window. Prevents the window from resizing smaller than a factor of 1x.
UPSCALE_PIXEL_PERFECT_AT_LEAST_2X | Upscales to the maximum factor while still fitting within the window, and prevents the window from resizing below a factor of 2x.
UPSCALE_PIXEL_PERFECT_AT_LEAST_3X | Upscales to the maximum factor while still fitting within the window, and prevents the window from resizing below a factor of 3x.
UPSCALE_PIXEL_PERFECT_AT_LEAST_4X | Upscales to the maximum factor while still fitting within the window, and prevents the window from resizing below a factor of 4x.
UPSCALE_STRETCH | Simply stretches the the offscreen buffer to the window. Not useful for pixel art games.

## Related Functions

[app_offscreen_size](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_offscreen_size.md)  
