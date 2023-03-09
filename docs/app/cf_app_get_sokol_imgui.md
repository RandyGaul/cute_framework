[](../header.md ':include')

# cf_app_get_sokol_imgui

Category: [app](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Fetches a sokol Dear ImGui debug context.

```cpp
sg_imgui_t* cf_app_get_sokol_imgui();
```

## Remarks

Internally Cute Framework uses [sokol_gfx.h](https://github.com/floooh/sokol) for wrapping low-level graphics APIs.
As an optional feature you can access `sokol_imgui_t` to use Dear ImGui to debug inspect all of sokol_gfx's primitives.
You must call [cf_app_init_imgui](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_init_imgui.md) to use this function.

## Related Pages

[cf_app_init_imgui](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/cf_app_init_imgui.md)  
