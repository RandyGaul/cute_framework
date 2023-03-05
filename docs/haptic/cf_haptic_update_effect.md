# cf_haptic_update_effect | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/README.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Dynamically updates an effect on the device. This _can not_ change the effect type.

```cpp
void cf_haptic_update_effect(CF_Haptic* haptic, CF_HapticEffect effect, CF_HapticData data);
```

Parameters | Description
--- | ---
haptic | The haptic.
effect | The haptic effect created by [cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md).
data | The updated haptic specification.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[CF_HapticData](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticdata.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_run_effect.md)  
[cf_haptic_stop_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_stop_effect.md)  
