# CF_ImeComposition | [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/input_readme.md) | [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)

Represents the IME (Input Method Editor) composition from the operating system, for gathering complex text inputs.

Struct Members | Description
--- | ---
`const char* composition` | The composition text, as it's currently being constructed.
`int cursor` | Where in the text the user is currently editing/typing/selecting.
`int selection_len` | If the user is currently selecting text, describes the length of the selection.

## Related Pages

[cf_input_enable_ime](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_enable_ime.md)  
[cf_input_get_ime_composition](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_input_get_ime_composition.md)  
