# cf_set_fixed_timestep

Category: [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Sets the frequency for fixed timestep updates to occur.

```cpp
void cf_set_fixed_timestep(int frames_per_second);
```

Parameters | Description
--- | ---
frames_per_second | The frequency for fixed-timestep updates to occur, e.g. 30 is a good default number.

## Remarks

Often times a fixed-timestep can occur multiple times in one frame. In this case, [CF_OnUpdateFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_onupdatefn.md) will be called once
per update to simulate a fixed-timestep (see [CF_OnUpdateFn](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_onupdatefn.md) and [cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)). The max number of updates possible 
is clamped below [cf_set_fixed_timestep_max_updates](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep_max_updates.md).

## Related Pages

[CF_DELTA_TIME_INTERPOLANT](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time_interpolant.md)  
[cf_set_fixed_timestep_max_updates](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_set_fixed_timestep_max_updates.md)  
[cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)  
[CF_DELTA_TIME_FIXED](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_delta_time_fixed.md)  
