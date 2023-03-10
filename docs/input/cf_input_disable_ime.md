[](../header.md ':include')

# cf_input_disable_ime

Category: [input](/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Disables the IME (Input Method Editor) for the operating system.

```cpp
CF_API void CF_CALL cf_input_disable_ime();
```

## Remarks

This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
of different language inputs. This is usually a feature of the underlying operating system.

## Related Pages

[cf_input_enable_ime](/input/cf_input_enable_ime.md)  
[cf_input_set_ime_rect](/input/cf_input_set_ime_rect.md)  
[cf_input_is_ime_enabled](/input/cf_input_is_ime_enabled.md)  
[cf_input_has_ime_keyboard_support](/input/cf_input_has_ime_keyboard_support.md)  
[cf_input_is_ime_keyboard_shown](/input/cf_input_is_ime_keyboard_shown.md)  
