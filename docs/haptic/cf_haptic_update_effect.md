[](../header.md ':include')

# cf_haptic_update_effect

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Dynamically updates an effect on the device. This _can not_ change the effect type.

```cpp
void cf_haptic_update_effect(CF_Haptic* haptic, CF_HapticEffect effect, CF_HapticData data);
```

Parameters | Description
--- | ---
haptic | The haptic.
effect | The haptic effect created by [cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md).
data | The updated haptic specification.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[CF_HapticData](/haptic/cf_hapticdata.md)  
[CF_HapticEffect](/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](/haptic/cf_haptic_run_effect.md)  
[cf_haptic_stop_effect](/haptic/cf_haptic_stop_effect.md)  
