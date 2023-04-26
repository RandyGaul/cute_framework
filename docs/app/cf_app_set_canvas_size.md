[](../header.md ':include')

# cf_app_set_canvas_size

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Resizes the app's internal canvas to a new w/h, in pixels.

```cpp
void cf_app_set_canvas_size(int w, int h);
```

Parameters | Description
--- | ---
w | The width in pixels to resize the canvas to.
h | The height in pixels to resize the canvas to.

## Remarks

Be careful about calling this function, as it will invalidate any old references from [cf_app_get_canvas](/app/cf_app_get_canvas.md).

## Related Pages

[cf_app_get_canvas](/app/cf_app_get_canvas.md)  
[cf_app_get_canvas_width](/app/cf_app_get_canvas_width.md)  
[cf_app_get_canvas_height](/app/cf_app_get_canvas_height.md)  
[cf_app_set_vsync](/app/cf_app_set_vsync.md)  
[cf_app_get_vsync](/app/cf_app_get_vsync.md)  
