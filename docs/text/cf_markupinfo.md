[](../header.md ':include')

# CF_MarkupInfo

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Info describing a markup inside of a string rendered with text effects.

Struct Members | Description
--- | ---
`const char* effect_name` | The name of the text effect. These would be effect names like `fade` or anything you have registered via [cf_text_effect_register](/text/cf_text_effect_register.md).
`int start_glyph_index` | The index of the first glyph this markup applies to. Use this index on the `text` string provided in the [cf_text_markup_info_fn](/text/cf_text_markup_info_fn.md) callback.
`int glyph_count` | The number of glyphs this markup applies to.
`int bounds_count` | The number of [CF_Aabb](/math/cf_aabb.md)'s in `bounds`.
`CF_Aabb* bounds` | An arry of [CF_Aabb](/math/cf_aabb.md)'s, one per line the `text` string provided in the [cf_text_markup_info_fn](/text/cf_text_markup_info_fn.md) callback.

## Remarks

This struct describes the markup information for each text effect within a renderable string.

## Related Pages

[CF_TextEffect](/text/cf_texteffect.md)  
[cf_text_get_markup_info](/text/cf_text_get_markup_info.md)  
[cf_text_markup_info_fn](/text/cf_text_markup_info_fn.md)  
