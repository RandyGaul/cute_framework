[](../header.md ':include')

# cf_render_settings_push_render_state

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Pushes a [CF_RenderState](/graphics/cf_renderstate.md) for controlling various rendering settings.

```cpp
CF_API void CF_CALL cf_render_settings_push_render_state(CF_RenderState render_state);
```

Parameters | Description
--- | ---
render_state | Various types of rendering states.

## Related Pages

[CF_RenderState](/graphics/cf_renderstate.md)  
[cf_render_settings_filter](/draw/cf_render_settings_filter.md)  
[cf_render_settings_push_viewport](/draw/cf_render_settings_push_viewport.md)  
[cf_render_settings_push_scissor](/draw/cf_render_settings_push_scissor.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_render_settings_pop_render_state](/draw/cf_render_settings_pop_render_state.md)  
[cf_render_settings_peek_render_state](/draw/cf_render_settings_peek_render_state.md)  
[cf_render_to](/draw/cf_render_to.md)  
