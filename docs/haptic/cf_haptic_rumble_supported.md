[](../header.md ':include')

# cf_haptic_rumble_supported

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Checks to see if a simple sine/leftright haptic can be supported on the device.

```cpp
CF_API bool CF_CALL cf_haptic_rumble_supported(CF_Haptic* haptic);
```

Parameters | Description
--- | ---
haptic | The haptic.

## Remarks

Creates a set of decent parameters to play a rumbling effect in an easy to use way.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[cf_haptic_rumble_stop](/haptic/cf_haptic_rumble_stop.md)  
[cf_haptic_rumble_play](/haptic/cf_haptic_rumble_play.md)  
