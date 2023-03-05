# CF_HapticPeriodic | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/README.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

A basic haptic for sine-based waveforms (https://en.wikipedia.org/wiki/Sine_wave).

Struct Members | Description
--- | ---
`CF_HapticWaveType wave_type` | The delay between `attack` and `fade` in the envelope (see [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for more details).
`int duration_milliseconds` | Time between each wave in milliseconds, or 1.0f/frequency. See [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for details.
`int period_milliseconds` | The period of the sin wave. Must be from 0.0f to 1.0f.
`float magnitude` | The strength/amplitude of the sin wave.
`CF_HapticEnvelope envelope` | The envelope for the haptic. See [CF_HapticEnvelope](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticenvelope.md) for details.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[CF_HapticType](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptictype.md)  
[cf_haptic_open](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_open.md)  
[cf_haptic_close](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_close.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
