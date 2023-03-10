[](../header.md ':include')

# cf_joypad_open

Category: [input](/api_reference?id=input)  
GitHub: [cute_joypad.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_joypad.h)  
---

Opens a joypad on the system.

```cpp
CF_API CF_Joypad* CF_CALL cf_joypad_open(int index);
```

Parameters | Description
--- | ---
index | Which joypad to open.

## Remarks

The first joypad connected to the system is 0, the second is 1, and so on.

## Related Pages

[CF_Joypad](/input/cf_joypad.md)  
[cf_joypad_count](/input/cf_joypad_count.md)  
[cf_joypad_close](/input/cf_joypad_close.md)  
