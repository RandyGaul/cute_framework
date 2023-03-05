# cf_input_get_ime_composition | [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/input_readme.md) | [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)

Returns the current IME (Input Method Editor) composition. See [CF_ImeComposition](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_imecomposition.md).

```cpp
bool cf_input_get_ime_composition(CF_ImeComposition* composition);
```

Parameters | Description
--- | ---
composition | The text composition.

## Return Value

Returns true if the IME (Input Method Editor) is currently composing text.

## Remarks

This is an advanced function. It's useful for gathering input from a variety of languages, where keystrokes are translated into a variety
of different language inputs. This is usually a feature of the underlying operating system.

## Related Pages

[cf_input_enable_ime](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_enable_ime.md)  
[CF_ImeComposition](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_imecomposition.md)  
