# cf_render_to | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Renders to a [CF_Canvas](https://github.com/RandyGaul/cute_framework/blob/master/docs/graphics/cf_canvas.md).

```cpp
void cf_render_to(CF_Canvas canvas, bool clear);
```

Parameters | Description
--- | ---
canvas | The canvas to render to.
clear | True to clear the canvas's previous contents, false otherwise.

## Remarks

This is advanced function. It's useful for off-screen rendering for certain rendering effects, such as multi-pass
effects like reflections, or advanced lighting techniques. By default, everything will get renderered to the app's
canvas, so this function is not necessary to call at all. Instead, calling [cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md) should be the go-to.

## Related Pages

[cf_camera_dimensions](https://github.com/RandyGaul/cute_framework/blob/master/docs/camera/cf_camera_dimensions.md)  
[cf_camera_look_at](https://github.com/RandyGaul/cute_framework/blob/master/docs/camera/cf_camera_look_at.md)  
[cf_camera_rotate](https://github.com/RandyGaul/cute_framework/blob/master/docs/camera/cf_camera_rotate.md)  
[cf_camera_push](https://github.com/RandyGaul/cute_framework/blob/master/docs/camera/cf_camera_push.md)  
[cf_camera_pop](https://github.com/RandyGaul/cute_framework/blob/master/docs/camera/cf_camera_pop.md)  
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md)  
