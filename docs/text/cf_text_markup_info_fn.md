[](../header.md ':include')

# cf_text_markup_info_fn

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Reports markup information for a text effect.

```cpp
typedef void (cf_text_markup_info_fn)(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx);
```

Parameters | Description
--- | ---
text | The renderable text.
info | Description of the markup for this text effect.
fx | The [CF_TextEffect](/text/cf_texteffect.md) instance used for rendering, containing markup metadata. See remarks for details.

## Remarks

This callback is invoked once per markup within the renderable `text`. If you wish to fetch any of the markup metadata
you may use [cf_text_effect_get_number](/text/cf_text_effect_get_number.md), [cf_text_effect_get_color](/text/cf_text_effect_get_color.md), or [cf_text_effect_get_string](/text/cf_text_effect_get_string.md) by passing in the `fx` pointer to each.

## Related Pages

[CF_TextEffect](/text/cf_texteffect.md)  
[CF_MarkupInfo](/text/cf_markupinfo.md)  
[cf_text_get_markup_info](/text/cf_text_get_markup_info.md)  
