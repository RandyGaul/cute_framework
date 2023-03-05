# CF_HapticEnvelope | [haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic_readme.md) | [cute_haptics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_haptics.h)

Defines a fade in and fade out of strength for a [CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md).

Struct Members | Description
--- | ---
`float attack` | A value from 0.0f to 1.0f. Represents the initial rise in the envelope curve.
`int attack_milliseconds` | The duration of `attack` in milliseconds.
`float fade` | A value from 0.0f to 1.0f. Represents the fall in the envelope curve.
`int fade_milliseconds` | The duration for `fade` to back down to zero at the end of the envelope, in milliseconds.

## Remarks

The envelope defines a fade in and fade out of strength for a haptic effect. The strength
starts at zero, and linearly fades up to `attack` over `attack_milliseconds`. The strength
then linearly interpolates down to `fade` over the duration of the haptic effect. Finally, the
strength diminishes from `fade` to zero over `fade milliseconds`.

`attack` and `fade` should be values from 0 to 1, where 0 means do nothing (no strength) and
1 means run the haptic motors as hard as possible (max strength).

Here's an example.

```
         ^
         1      (2)--.
         |      /     `--.
strength |     /          `--(3)
         |    /                \
         0  (1)                (4)
         +-------------------------->
                    time
```

From (1) to (2) is the `attack_milliseconds`.
From (1) to (4) is the effect duration (search for `duration_milliseconds` in this file).
From (3) to (4) is the `fade_milliseconds`.

## Related Pages

[CF_Haptic](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic.md)  
[cf_haptic_create_effect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_haptic_create_effect.md)  
[CF_HapticEffect](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticeffect.md)  
[CF_HapticData](https://github.com/RandyGaul/cute_framework/blob/master/docs/haptic/cf_hapticdata.md)  
