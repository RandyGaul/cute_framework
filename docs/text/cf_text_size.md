[](../header.md ':include')

# cf_text_size

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns the width/height of a text given the currently pushed font.

```cpp
CF_V2 cf_text_size(const char* text, int num_chars_to_draw);
```

Parameters | Description
--- | ---
text | The text considered for rendering.
num_chars_to_draw | The number of characters to draw `text`. Use -1 to draw the whole string.

## Remarks

This function is slightly superior to [cf_text_width](/text/cf_text_width.md) or [cf_text_height](/text/cf_text_height.md) if you need both width/height, as
it will run the layout code only a single time.

## Related Pages

[cf_make_font](/text/cf_make_font.md)  
[cf_text_width](/text/cf_text_width.md)  
[cf_text_height](/text/cf_text_height.md)  
[cf_draw_text](/text/cf_draw_text.md)  
