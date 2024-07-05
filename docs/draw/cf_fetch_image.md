[](../header.md ':include')

# cf_fetch_image

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns a [CF_TemporaryImage](/draw/cf_temporaryimage.md) for a given sprite.

```cpp
CF_TemporaryImage cf_fetch_image(const CF_Sprite* sprite);
```

Parameters | Description
--- | ---
sprite | The sprite.

## Remarks

Useful to render a sprite in an external system, e.g. Dear ImGui. This struct is only valid until the next time
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md) is called.

## Related Pages

[CF_TemporaryImage](/draw/cf_temporaryimage.md)  
