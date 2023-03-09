# CF_HapticRamp

Category: [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=haptic)  
GitHub: [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)  
---

Adjusts the strength from `start` to `end` over `duration_milliseconds`.

Struct Members | Description
--- | ---
`int duration_milliseconds` | The delay between `attack` and `fade` in the envelope (see [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for more details).
`float start` | Strength value. Must be from 0.0f to 1.0f.
`float end` | Strength value. Must be from 0.0f to 1.0f.
`CF_HapticEnvelope envelope` | The envelope for the haptic. See [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for details.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[CF_HapticType](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptictype.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[cf_haptic_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_close.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
