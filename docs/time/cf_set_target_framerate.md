[](../header.md ':include')

# cf_set_target_framerate

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Attempts to render onto the screen at a certain frequency.

```cpp
void cf_set_target_framerate(int frames_per_second);
```

Parameters | Description
--- | ---
frames_per_second | Target frequency to run the app.

## Remarks

This effect will try to render the game at a target framerate, similar to vsync. Set to -1 to disable this effect (disabled by default).
This only affects rendering (not gameplay/update), see [cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md) to control your gameplay/update framerate.

## Related Pages

[cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md)  
[cf_set_fixed_timestep_max_updates](/time/cf_set_fixed_timestep_max_updates.md)  
[cf_update_time](/time/cf_update_time.md)  
[CF_DELTA_TIME_FIXED](/time/cf_delta_time_fixed.md)  
[CF_DELTA_TIME_INTERPOLANT](/time/cf_delta_time_interpolant.md)  
