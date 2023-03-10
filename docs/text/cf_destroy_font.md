[](../header.md ':include')

# cf_destroy_font

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Destroys a font previously made by [cf_make_font](/text/cf_make_font.md) or [cf_make_font_mem](/text/cf_make_font_mem.md).

```cpp
CF_API void CF_CALL cf_destroy_font(const char* font_name);
```

Parameters | Description
--- | ---
font_name | The unique name for this font.

## Related Pages

[cf_make_font](/text/cf_make_font.md)  
[cf_make_font_mem](/text/cf_make_font_mem.md)  
[cf_draw_text](/text/cf_draw_text.md)  
[cf_push_font](/text/cf_push_font.md)  
[cf_push_font_size](/text/cf_push_font_size.md)  
[cf_push_font_blur](/text/cf_push_font_blur.md)  
