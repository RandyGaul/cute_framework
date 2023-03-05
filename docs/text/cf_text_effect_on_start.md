# cf_text_effect_on_start | [text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text_readme.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Helper function to see if the current glyph is the beginning of the text, from within a custom text effect.

```cpp
bool cf_text_effect_on_start(CF_TextEffect* fx);
```

Parameters | Description
--- | ---
fx | The text effect state.

## Return Value

Return true to continue to the next glyph, false otherwise.

## Related Pages

[CF_TextEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffect.md)  
[CF_TextEffectFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffectfn.md)  
[cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md)  
[cf_text_effect_get_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_string.md)  
[cf_text_effect_on_finish](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_finish.md)  
[cf_text_effect_get_number](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_number.md)  
[cf_text_effect_get_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_color.md)  
