[](../header.md ':include')

# cf_haptic_supports

Category: [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Checks to see if a certain type of haptic is supported on this device.

```cpp
bool cf_haptic_supports(CF_Haptic* haptic, CF_HapticType type);
```

Parameters | Description
--- | ---
haptic | The haptic.
type | The [CF_HapticType](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptictype.md) to check support for.

## Return Value

Returns true if supported, false otherwise.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[cf_haptic_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_close.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_run_effect.md)  
