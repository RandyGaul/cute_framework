# cf_text_effect_get_string | [text](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/README.md) | [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)

Returns the text parameter as a string.

```cpp
const char* cf_text_effect_get_string(CF_TextEffect* fx, const char* key, const char* default_val);
```

Parameters | Description
--- | ---
fx | The text effect state.
key | The name of the text code parameter
default_val | A default value for the text code parameter if doesn't exist in the text.

## Return Value

Returns the value of the text code parameter.

## Related Pages

[CF_TextEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffect.md)  
[CF_TextEffectFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_texteffectfn.md)  
[cf_text_effect_register](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_register.md)  
[cf_text_effect_on_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_start.md)  
[cf_text_effect_on_finish](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_on_finish.md)  
[cf_text_effect_get_number](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_number.md)  
[cf_text_effect_get_color](https://github.com/RandyGaul/cute_framework/blob/master/docs/text/cf_text_effect_get_color.md)  
