# cf_fetch_image | [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Returns a [CF_TemporaryImage](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_temporaryimage.md) for a given sprite.

```cpp
CF_TemporaryImage cf_fetch_image(const CF_Sprite* sprite);
```

Parameters | Description
--- | ---
sprite | The sprite.

## Remarks

Useful to render a sprite in an external system, e.g. Dear ImGui. This struct is only valid until the next time [cf_render_to](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_render_to.md) or
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md) is called.

## Related Pages

[CF_TemporaryImage](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_temporaryimage.md)  
