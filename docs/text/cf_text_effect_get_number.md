[](../header.md ':include')

# cf_text_effect_get_number

Category: [text](/api_reference?id=text)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Returns the text parameter as a number.

```cpp
double cf_text_effect_get_number(const CF_TextEffect* fx, const char* key, double default_val);
```

Parameters | Description
--- | ---
fx | The text effect state.
key | The name of the text code parameter
default_val | A default value for the text code parameter if doesn't exist in the text.

## Return Value

Returns the value of the text code parameter.

## Related Pages

[CF_TextEffect](/text/cf_texteffect.md)  
[CF_TextEffectFn](/text/cf_texteffectfn.md)  
[cf_text_effect_register](/text/cf_text_effect_register.md)  
cf_text_effect_on_start  
cf_text_effect_on_finish  
[cf_text_effect_get_string](/text/cf_text_effect_get_string.md)  
[cf_text_effect_get_color](/text/cf_text_effect_get_color.md)  
