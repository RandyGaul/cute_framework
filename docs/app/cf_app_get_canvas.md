[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_app_get_canvas

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Fetches the app's internal canvas for displaying content on the screen.

```cpp
CF_Canvas cf_app_get_canvas();
```

## Remarks

This is an advanced function. If you just want to draw things on screen, try checking out [CF_Sprite](/sprite/cf_sprite.md).
The app's canvas can be used to implement low-level graphics features, such as multi-pass algorithms. Be careful about
calling [cf_app_set_canvas_size](/app/cf_app_set_canvas_size.md), as it will invalidate any references to the app's canvas.

If you fetch this canvas and have MSAA on (see [cf_app_set_msaa](/app/cf_app_set_msaa.md)) you may not sample from the canvas.

## Related Pages

[cf_app_set_canvas_size](/app/cf_app_set_canvas_size.md)  
[cf_app_get_canvas_width](/app/cf_app_get_canvas_width.md)  
[cf_app_get_canvas_height](/app/cf_app_get_canvas_height.md)  
[cf_app_set_vsync](/app/cf_app_set_vsync.md)  
[cf_app_get_vsync](/app/cf_app_get_vsync.md)  
