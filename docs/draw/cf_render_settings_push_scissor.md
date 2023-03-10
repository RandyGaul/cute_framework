[](../header.md ':include')

# cf_render_settings_push_scissor

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Pushes a [CF_Rect](/math/cf_rect.md) for the scissor to render within.

```cpp
CF_API void CF_CALL cf_render_settings_push_scissor(CF_Rect scissor);
```

Parameters | Description
--- | ---
scissor | The scissor box.

## Related Pages

[cf_render_settings_filter](/draw/cf_render_settings_filter.md)  
[cf_render_settings_push_viewport](/draw/cf_render_settings_push_viewport.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_render_settings_pop_scissor](/draw/cf_render_settings_pop_scissor.md)  
[cf_render_settings_peek_scissor](/draw/cf_render_settings_peek_scissor.md)  
[cf_render_settings_push_render_state](/draw/cf_render_settings_push_render_state.md)  
[cf_render_to](/draw/cf_render_to.md)  
