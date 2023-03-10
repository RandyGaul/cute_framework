[](../header.md ':include')

# cf_haptic_stop_all

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Stops all haptics on the device.

```cpp
CF_API void CF_CALL cf_haptic_stop_all(CF_Haptic* haptic);
```

Parameters | Description
--- | ---
haptic | The haptic.

## Remarks

The effects are not destroyed.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[CF_HapticEffect](/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_stop_effect](/haptic/cf_haptic_stop_effect.md)  
[cf_haptic_destroy_effect](/haptic/cf_haptic_destroy_effect.md)  
[cf_haptic_pause](/haptic/cf_haptic_pause.md)  
[cf_haptic_unpause](/haptic/cf_haptic_unpause.md)  
