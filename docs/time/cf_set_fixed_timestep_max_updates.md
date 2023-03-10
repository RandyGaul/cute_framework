[](../header.md ':include')

# cf_set_fixed_timestep_max_updates

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Sets the frequency for fixed timestep updates to occur.

```cpp
CF_API void CF_CALL cf_set_fixed_timestep_max_updates(int max_updates);
```

Parameters | Description
--- | ---
max_updates | The max number of updates. A good default number is 5.

## Remarks

Often times a fixed-timestep can occur multiple times in one frame. In this case, [CF_OnUpdateFn](/time/cf_onupdatefn.md) will be called once
per update to simulate a fixed-timestep (see [CF_OnUpdateFn](/time/cf_onupdatefn.md) and [cf_update_time](/time/cf_update_time.md)). The max number of updates possible 
is clamped below `max_updates`.

## Related Pages

[cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md)  
[CF_DELTA_TIME_INTERPOLANT](/time/cf_delta_time_interpolant.md)  
[cf_update_time](/time/cf_update_time.md)  
[CF_DELTA_TIME_FIXED](/time/cf_delta_time_fixed.md)  
