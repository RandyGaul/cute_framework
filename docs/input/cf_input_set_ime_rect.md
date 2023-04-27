[](../header.md ':include')

# cf_input_set_ime_rect

Category: [input](/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Tells the operating system where the current IME (Input Method Editor) rect should be.

```cpp
void cf_input_set_ime_rect(int x, int y, int w, int h);
```

## Remarks

This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
of different language inputs. This is usually a feature of the underlying operating system.

## Related Pages

[cf_input_enable_ime](/input/cf_input_enable_ime.md)  
[cf_input_disable_ime](/input/cf_input_disable_ime.md)  
[cf_input_is_ime_enabled](/input/cf_input_is_ime_enabled.md)  
[cf_input_has_ime_keyboard_support](/input/cf_input_has_ime_keyboard_support.md)  
[cf_input_is_ime_keyboard_shown](/input/cf_input_is_ime_keyboard_shown.md)  
