[](../header.md ':include')

# cf_haptic_open

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Attempts to open a joypad for haptics use.

```cpp
CF_API CF_Haptic* CF_CALL cf_haptic_open(CF_Joypad* joypad);
```

Parameters | Description
--- | ---
joypad | A joypad (see [CF_Joypad](/input/cf_joypad.md)).

## Return Value

Returns a new [CF_Haptic](/haptic/cf_haptic.md).

## Remarks

Returns `NULL` upon any errors, including missing support from the underlying device.

## Related Pages

[CF_Haptic](/haptic/cf_haptic.md)  
[CF_Joypad](/input/cf_joypad.md)  
[cf_haptic_rumble_play](/haptic/cf_haptic_rumble_play.md)  
[cf_haptic_close](/haptic/cf_haptic_close.md)  
[cf_haptic_create_effect](/haptic/cf_haptic_create_effect.md)  
[cf_haptic_run_effect](/haptic/cf_haptic_run_effect.md)  
