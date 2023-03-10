[](../header.md ':include')

# cf_joypad_axis

Category: [input](/api_reference?id=input)  
GitHub: [cute_joypad.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_joypad.h)  
---

Returns a signed 16-bit integer representing how much a joypad axis is activated by.

```cpp
CF_API int16_t CF_CALL cf_joypad_axis(CF_Joypad* joypad, CF_JoypadAxis axis);
```

Parameters | Description
--- | ---
joypad | The joypad.
axis | The axis.

## Related Pages

[CF_Joypad](/input/cf_joypad.md)  
[CF_JoypadButton](/input/cf_joypadbutton.md)  
[cf_joypad_button_down](/input/cf_joypad_button_down.md)  
[cf_joypad_button_just_pressed](/input/cf_joypad_button_just_pressed.md)  
[cf_joypad_button_just_released](/input/cf_joypad_button_just_released.md)  
