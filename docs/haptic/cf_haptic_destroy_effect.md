[](../header.md ':include')

# cf_haptic_destroy_effect

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Destroys an instance of an effect on the device.

```cpp
void cf_haptic_destroy_effect(CF_Haptic* haptic, CF_HapticEffect effect);
```

Parameters | Description
--- | ---
haptic | The haptic.
effect | The haptic effect created by [cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md).

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[CF_HapticEffect](/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_stop_effect](/haptic/cf_haptic_stop_effect.md)  
[cf_haptic_stop_all](/haptic/cf_haptic_stop_all.md)  
[cf_haptic_pause](/haptic/cf_haptic_pause.md)  
[cf_haptic_unpause](/haptic/cf_haptic_unpause.md)  
