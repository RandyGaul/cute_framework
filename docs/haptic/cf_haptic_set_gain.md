[](../header.md ':include')

# cf_haptic_set_gain

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Sets the global gain for all haptics on this device.

```cpp
void cf_haptic_set_gain(CF_Haptic* haptic, float gain);
```

Parameters | Description
--- | ---
haptic | The haptic.
gain | Must be from 0 to 1. This is like a global "volume" for the strength of all haptics that run on this device.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[cf_haptic_close](/haptic/cf_haptic_close.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](/haptic/cf_haptic_run_effect.md)  
