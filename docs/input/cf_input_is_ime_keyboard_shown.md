[](../header.md ':include')

# cf_input_is_ime_keyboard_shown

Category: [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Returns true if the IME (Input Method Editor) for the operating system is currently showing the keyboard.

```cpp
bool cf_input_is_ime_keyboard_shown();
```

## Remarks

This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
of different language inputs. This is usually a feature of the underlying operating system.

## Related Pages

[cf_input_enable_ime](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_enable_ime.md)  
[cf_input_disable_ime](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_disable_ime.md)  
[cf_input_is_ime_enabled](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_is_ime_enabled.md)  
[cf_input_has_ime_keyboard_support](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_has_ime_keyboard_support.md)  
[cf_input_set_ime_rect](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_set_ime_rect.md)  
