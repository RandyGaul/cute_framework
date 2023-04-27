[](../header.md ':include')

# CF_Haptic

Category: [haptic](/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

An opaque pointer representing a haptic.

## Remarks

Haptics is for rumbling or vibrating devices or controllers.

A haptic pointer can be opened by calling `haptic_open` on a joypad. See cute_joypad.h for info on joypads.

Haptics can be a little complicated -- if you just want a simple rumble effect then the rumble functions might be a good option for you.
- [cf_haptic_rumble_supported](/haptic/cf_haptic_rumble_supported.md)
- [cf_haptic_rumble_play](/haptic/cf_haptic_rumble_play.md)
- [cf_haptic_rumble_stop](/haptic/cf_haptic_rumble_stop.md)

TODO - Open haptic on the device itself (e.g. for phones).

## Related Pages

[CF_HapticEffect](/haptic/cf_hapticeffect.md)  
[CF_HapticType](/haptic/cf_haptictype.md)  
[cf_haptic_open](/haptic/cf_haptic_open.md)  
[cf_haptic_close](/haptic/cf_haptic_close.md)  
