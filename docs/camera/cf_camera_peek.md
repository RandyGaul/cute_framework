[](../header.md ':include')

# cf_camera_peek

Category: [camera](/api_reference?id=camera)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns the current camera as a [CF_M3x2](/math/cf_m3x2.md).

```cpp
CF_M3x2 cf_camera_peek();
```

## Remarks

Multiplying this matrix against a vector will transform the vector to "cam space" or "eye space".

## Related Pages

[cf_camera_dimensions](/camera/cf_camera_dimensions.md)  
[cf_camera_look_at](/camera/cf_camera_look_at.md)  
[cf_camera_rotate](/camera/cf_camera_rotate.md)  
[cf_camera_push](/camera/cf_camera_push.md)  
[cf_camera_pop](/camera/cf_camera_pop.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_render_to](/draw/cf_render_to.md)  
[cf_camera_peek_position](/camera/cf_camera_peek_position.md)  
[cf_camera_peek_dimensions](/camera/cf_camera_peek_dimensions.md)  
[cf_camera_peek_rotation](/camera/cf_camera_peek_rotation.md)  
