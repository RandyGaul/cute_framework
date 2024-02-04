[](../header.md ':include')

# cf_register_key_callback

Category: [input](/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Registers a callback invoked whenever a key is pressed.

```cpp
void cf_register_key_callback(void (*key_callback)(CF_KeyButton key, bool true_down_false_up));
```

## Related Pages

[CF_KeyButton](/input/cf_keybutton.md)  
[cf_key_down](/input/cf_key_down.md)  
cf_key_up  
[cf_key_just_pressed](/input/cf_key_just_pressed.md)  
[cf_key_just_released](/input/cf_key_just_released.md)  
[cf_key_ctrl](/input/cf_key_ctrl.md)  
[cf_key_shift](/input/cf_key_shift.md)  
[cf_key_alt](/input/cf_key_alt.md)  
[cf_key_gui](/input/cf_key_gui.md)  
[cf_clear_key_states](/input/cf_clear_key_states.md)  
