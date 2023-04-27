[](../header.md ':include')

# cf_app_init_imgui

Category: [app](/api_reference?id=app)  
GitHub: [cute_app.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_app.h)  
---

Initializes Dear ImGui.

```cpp
ImGuiContext* cf_app_init_imgui(bool no_default_font /*= false*/);
```

Parameters | Description
--- | ---
no_default_font | Prevents Dear ImGui from loading up it's own default font to save a small bit of memory.
                 You must then supply your own font.

## Remarks

[Dear ImGui](https://github.com/ocornut/imgui) is an excellent UI library for debugging, great for making tools and editors.
After calling this init function you can call into Dear ImGui's functions.

## Related Pages

[cf_app_get_sokol_imgui](/app/cf_app_get_sokol_imgui.md)  
