[](../header.md ':include')

# cf_draw_text

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws text.

```cpp
void cf_draw_text(const char* text, CF_V2 position, int text_length /*= -1*/);
```

Parameters | Description
--- | ---
text | The text to draw.
position | The top-left corner of the text.
text_length | The length of the text to draw. Use -1 to draw until a null terminator.

## Remarks

`text_length` is a great way to control how many characters to draw for implementing a typewriter style effect.

## Related Pages

[cf_make_font](/text/cf_make_font.md)  
[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_text_effect_register](/text/cf_text_effect_register.md)  
cf_draw_to  
