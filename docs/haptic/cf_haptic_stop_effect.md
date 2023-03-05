# cf_haptic_stop_effect | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic_readme.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Stops playing the specified effect.

```cpp
void cf_haptic_stop_effect(CF_Haptic* haptic, CF_HapticEffect effect);
```

Parameters | Description
--- | ---
haptic | The haptic.
effect | The haptic effect created by [cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md).

## Remarks

The effect is not destroyed.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[cf_haptic_stop_all](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_stop_all.md)  
[cf_haptic_destroy_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_destroy_effect.md)  
[cf_haptic_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_pause.md)  
[cf_haptic_unpause](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_unpause.md)  
