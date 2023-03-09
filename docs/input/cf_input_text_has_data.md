# cf_input_text_has_data

Category: [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Returns true if the input buffer of the application has any text within.

```cpp
bool cf_input_text_has_data();
```

## Remarks

The input text functions are for dealing with text input. Not all text inputs come from a single key-stroke, as some are comprised of
multiple keystrokes, especially when dealing with non-Latin based inputs.

## Related Pages

[cf_input_text_add_utf8](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_text_add_utf8.md)  
[cf_input_text_pop_utf32](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_text_pop_utf32.md)  
[cf_input_text_clear](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_text_clear.md)  
