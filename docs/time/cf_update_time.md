# cf_update_time | [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/README.md) | [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)

Updates the application.

```cpp
void cf_update_time(CF_OnUpdateFn* on_update);
```

Parameters | Description
--- | ---
on_update | Can be `NULL`. Called once per update. Mostly just useful for the fixed-timestep case (see [cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md)).

## Related Pages

[cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md)  
[cf_set_fixed_timestep_max_updates](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep_max_updates.md)  
[CF_DELTA_TIME_INTERPOLANT](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time_interpolant.md)  
[CF_DELTA_TIME_FIXED](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time_fixed.md)  
