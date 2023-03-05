# cf_font_add_backup_codepoints | [text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

When drawing text, and missing glyphs from the font will be replaced by any backup codepoints.

```cpp
void cf_font_add_backup_codepoints(const char* font_name, int* codepoints, int count);
```

Parameters | Description
--- | ---
font_name | The unique name for this font.
codepoints | An array of backup codepoints. Highest priority comes first in the array.
count | The number of elements in `codepoints`.

## Related Pages

[cf_draw_text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_draw_text.md)  
