# cf_haptic_open | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/README.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Attempts to open a joypad for haptics use.

```cpp
CF_Haptic* cf_haptic_open(CF_Joypad* joypad);
```

Parameters | Description
--- | ---
joypad | A joypad (see [CF_Joypad](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_joypad.md)).

## Return Value

Returns a new [CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md).

## Remarks

Returns `NULL` upon any errors, including missing support from the underlying device.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[CF_Joypad](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_joypad.md)  
[cf_haptic_rumble_play](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_rumble_play.md)  
[cf_haptic_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_close.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_run_effect.md)  
