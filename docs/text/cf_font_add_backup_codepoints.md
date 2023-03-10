[](../header.md ':include')

# cf_font_add_backup_codepoints

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

When drawing text, and missing glyphs from the font will be replaced by any backup codepoints.

```cpp
CF_API void CF_CALL cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count);
```

Parameters | Description
--- | ---
font_name | The unique name for this font.
codepoints | An array of backup codepoints. Highest priority comes first in the array.
count | The number of elements in `codepoints`.

## Related Pages

[cf_draw_text](/text/cf_draw_text.md)  
