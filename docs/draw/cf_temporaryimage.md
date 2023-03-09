[](../header.md ':include')

# CF_TemporaryImage

Category: [draw](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns temporal information about a sprite's rendering internals.

Struct Members | Description
--- | ---
`CF_Texture tex` | A handle representing the texture for this image.
`int w` | Width in pixels of the image.
`int h` | Height in pixels of the image.
`CF_V2 u` | u coordinate of the image in the texture.
`CF_V2 v` | v coordinate of the image in the texture.

## Remarks

Useful to render a sprite in an external system, e.g. Dear ImGui. This struct is only valid until the next time [cf_render_to](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_render_to.md) or
[cf_app_draw_onto_screen](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_draw_onto_screen.md) is called.

## Related Pages

[cf_fetch_image](https://github.com/RandyGaul/cute_framework/blob/master/docs/draw/cf_fetch_image.md)  
