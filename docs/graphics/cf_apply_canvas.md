[](../header.md ':include')

# cf_apply_canvas

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets up which canvas to draw to.

```cpp
CF_API void CF_CALL cf_apply_canvas(CF_Canvas canvas, bool clear);
```

Parameters | Description
--- | ---
canvas | The canvas to draw to.
clear | Clears the screen to [cf_clear_color](/graphics/cf_clear_color.md) if true.

## Related Pages

[CF_Canvas](/graphics/cf_canvas.md)  
[cf_clear_color](/graphics/cf_clear_color.md)  
[cf_apply_viewport](/graphics/cf_apply_viewport.md)  
[cf_apply_scissor](/graphics/cf_apply_scissor.md)  
