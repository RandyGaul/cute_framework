# cf_joypad_open | [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/input_readme.md) | [cute_joypad.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_joypad.h)

Opens a joypad on the system.

```cpp
CF_Joypad* cf_joypad_open(int index);
```

Parameters | Description
--- | ---
index | Which joypad to open.

## Remarks

The first joypad connected to the system is 0, the second is 1, and so on.

## Related Pages

[CF_Joypad](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_joypad.md)  
[cf_joypad_count](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_joypad_count.md)  
[cf_joypad_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_joypad_close.md)  
