[](../header.md ':include')

# cf_input_text_pop_utf32

Category: [input](/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Pops a utf8 codepoint off of the input buffer of the application.

```cpp
CF_API int CF_CALL cf_input_text_pop_utf32();
```

## Remarks

The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
multiple keystrokes, especially when dealing with non-Latin based inputs.

## Related Pages

[cf_input_text_add_utf8](/input/cf_input_text_add_utf8.md)  
[cf_input_text_clear](/input/cf_input_text_clear.md)  
[cf_input_text_has_data](/input/cf_input_text_has_data.md)  
