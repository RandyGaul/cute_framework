# cf_haptic_run_effect | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic_readme.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Starts playing the specified effect a number of times.

```cpp
void cf_haptic_run_effect(CF_Haptic* haptic, CF_HapticEffect effect, int iterations);
```

Parameters | Description
--- | ---
haptic | The haptic.
effect | The haptic effect created by [cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md).
iterations | A number of times to play the effect.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[cf_haptic_stop_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_stop_effect.md)  
