[](../header.md ':include')

# cf_set_target_framerate

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Attempts to run the app at a certain frequency.

```cpp
void cf_set_target_framerate(int frames_per_second);
```

Parameters | Description
--- | ---
frames_per_second | Target frequency to run the app.

## Remarks

Sleeps the app if you're updating too fast. This is sort of like vsync. Off by default. Set to -1 to turn off.

## Related Pages

[cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md)  
[cf_set_fixed_timestep_max_updates](/time/cf_set_fixed_timestep_max_updates.md)  
[cf_update_time](/time/cf_update_time.md)  
[CF_DELTA_TIME_FIXED](/time/cf_delta_time_fixed.md)  
[CF_DELTA_TIME_INTERPOLANT](/time/cf_delta_time_interpolant.md)  
