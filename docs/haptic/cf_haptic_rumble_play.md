# cf_haptic_rumble_play | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic_readme.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Starts playing a simple rumble effect

```cpp
void cf_haptic_rumble_play(CF_Haptic* haptic, float strength, int duration_milliseconds);
```

Parameters | Description
--- | ---
haptic | The haptic.
strength | Must be from 0.0f to 1.0f. How strong the rumble is.
duration_milliseconds | The duration of the rumble in milliseconds.

## Remarks

Successive calls on this function will update the underlying rumble effect, instead of creating and playing multiple different effect instances.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[cf_haptic_rumble_supported](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_rumble_supported.md)  
[cf_haptic_rumble_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_rumble_stop.md)  
