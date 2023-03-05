# CF_DELTA_TIME_INTERPOLANT | [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/README.md) | [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)

The remaining interpolant between two fixed timestep updates, used for rendering.

## Remarks

When fixed-timestepping is used (turned on by [cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md)) usually rendering happens at a different frequency
than the fixed updates. To produce smooth visuals, the current time is mod'd producing an interpolant value from [0,1].
Typically games will store two transforms, the previous and current transform, each representing a position at a fixed timestep
interval. This interpolant value is to interpolate between these two states to produce a smooth rendering transition.

## Related Pages

[CF_DELTA_TIME](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time.md)  
[cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)  
[CF_OnUpdateFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_onupdatefn.md)  
[cf_set_fixed_timestep_max_updates](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep_max_updates.md)  
[cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md)  
