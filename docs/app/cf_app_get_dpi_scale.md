[](../header.md ':include')

# cf_app_get_dpi_scale

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Returns the scaling factor for the device's intended DPI setting.

```cpp
float cf_app_get_dpi_scale();
```

## Remarks

On some devices (e.g. Apple Retina or iOS) pixels are clustered in 4x4 packs and abstracted as a single pixel
called a "point". The intent is for applications to work in points, and scale their UI elements by a factor of 2x
to aid in readability. These devices have very small pixels. Most of the time you should ignore dpi and let the OS
handle this. CF enables DPI settings by default, but, you can see if this function returns 2.0f to let you know if
pixels are clustered for you under the hood.

## Related Pages

[cf_app_set_size](/app/cf_app_set_size.md)  
[cf_app_get_position](/app/cf_app_get_position.md)  
[cf_app_set_position](/app/cf_app_set_position.md)  
[cf_app_get_width](/app/cf_app_get_width.md)  
[cf_app_get_height](/app/cf_app_get_height.md)  
[cf_app_dpi_scale_was_changed](/app/cf_app_dpi_scale_was_changed.md)  
