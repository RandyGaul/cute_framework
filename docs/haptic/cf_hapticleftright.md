# CF_HapticLeftRight | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/README.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

The leftright haptic allows direct control of one larger and one smaller freqeuncy motors, as commonly found in game controllers.

Struct Members | Description
--- | ---
`int duration_milliseconds` | The delay between `attack` and `fade` in the envelope (see [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for more details).
`float lo_motor_strength` | From 0.0f to 1.0f.
`float hi_motor_strength` | From 0.0f to 1.0f.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[CF_HapticType](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptictype.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[cf_haptic_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_close.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
