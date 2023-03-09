[](../header.md ':include')

# CF_DELTA_TIME_FIXED

Category: [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

The time elapsed between two frames in fixed-timestep mode.

## Remarks

To turn on fixed-timestep use [cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md). Fixed timestep is useful when games need determinism. Determinism
means given the same inputs, the exact same outputs are produced. This is especially important for networked games, or
games that require extreme precision and fine-tuning (such as certain fighting games or platformers).

## Related Pages

[CF_DELTA_TIME](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time.md)  
[cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)  
[CF_OnUpdateFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_onupdatefn.md)  
[CF_DELTA_TIME_INTERPOLANT](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time_interpolant.md)  
[cf_set_fixed_timestep](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep.md)  
[cf_set_fixed_timestep_max_updates](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep_max_updates.md)  
