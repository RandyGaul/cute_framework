[](../header.md ':include')

# cf_app_get_canvas

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Fetches the app's internal canvas for displaying content on the screen.

```cpp
CF_API CF_Canvas CF_CALL cf_app_get_canvas();
```

## Remarks

This is an advanced function. If you just want to draw things on screen, try checking out [CF_Sprite](/sprite/cf_sprite.md).
The app's canvas can be used to implement low-level graphics features, such as multi-pass algorithms. Be careful about
calling [cf_app_set_canvas_size](/app/cf_app_set_canvas_size.md), as it will invalidate any references to the app's canvas.

## Related Pages

[cf_app_set_canvas_size](/app/cf_app_set_canvas_size.md)  
[cf_app_get_canvas_width](/app/cf_app_get_canvas_width.md)  
[cf_app_get_canvas_height](/app/cf_app_get_canvas_height.md)  
