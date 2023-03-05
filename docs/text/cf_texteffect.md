# CF_TextEffect | [text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

A user-defined text effect that can be triggered with text codes.

Struct Members | Description
--- | ---
`const char* effect_name` | Name of this effect, as registered by [cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md).
`int character` | UTF8 codepoint of the current character.
`int index_into_string` | The index into the string in [cf_draw_text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_draw_text.md) currently affected.
`int index_into_effect` | Starts at 0 and increments for each character affected.
`int glyph_count` | The number of glyphs spanning the entire effect.
`float elapsed` | How long this effect has persisted for.
`CF_V2 center` | Center of this glyp's space -- not the same as the center of the glyph quad.
`CF_V2 q0, q1` | User-modifiable. This glyph's renderable quad. q0 is the min vertex, while q1 is the max vertex.
`int w, h` | Width and height of the glyph.
`CF_Color color` | User-modifiable. The color to render this glyph with.
`float opacity` | User-modifiable. The opacity to render this glyph with.
`float xadvance` | User-modifiable. How far the text will advance along the x-axis (only applicable for non-vertical layout mode).
`bool visible` | User-modifiable. Whether or not this glyph is visibly rendered (e.g. not visible for spaces ' ').
`float font_size` | The last size passed to [cf_push_font_size](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_push_font_size.md).

## Code Example

> Quick example listing some valid strings using text effects. These are all built-in text effects, and not user-defined custom ones.

```cpp
"This text is white. And this is <color=0x55b6f2ff>blue text</color>!"
"<fade>This text shows a fade example~</fade>"
```

## Remarks

A text code is an XML-style markup for strings. See the above code example for what this looks like. See [CF_TextEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffect.md) and
[cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md) on registering a custom-made text effect. See [cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md) for a big list of built-in text effects
that work out-of-the-box. Members of this struct that can be mutated freely within a custom text effect are noted with "User-modifiable"
in their description.

## Related Pages

[cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md)  
[CF_TextEffectFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffectfn.md)  
