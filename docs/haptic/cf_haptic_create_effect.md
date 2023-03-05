# cf_haptic_create_effect | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/README.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Creates a single effect instance on the device.

```cpp
CF_HapticEffect cf_haptic_create_effect(CF_Haptic* haptic, CF_HapticData data);
```

Parameters | Description
--- | ---
haptic | The haptic.
data | The haptic specification.

## Remarks

Run the haptic with `haptic_run_effect`.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_run_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_run_effect.md)  
