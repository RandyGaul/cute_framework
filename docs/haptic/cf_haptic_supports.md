[](../header.md ':include')

# cf_haptic_supports

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Checks to see if a certain type of haptic is supported on this device.

```cpp
CF_API bool CF_CALL cf_haptic_supports(CF_Haptic* haptic, CF_HapticType type);
```

Parameters | Description
--- | ---
haptic | The haptic.
type | The [CF_HapticType](/haptic/cf_haptictype.md) to check support for.

## Return Value

Returns true if supported, false otherwise.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[cf_haptic_close](/haptic/cf_haptic_close.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](/haptic/cf_haptic_run_effect.md)  
