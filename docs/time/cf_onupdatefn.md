[](../header.md ':include')

# CF_OnUpdateFn

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

An optional function pointer (callback), called whenever an update should occur, after calling [cf_update_time](/time/cf_update_time.md).

```cpp
typedef void (CF_OnUpdateFn)(void* udata);
```

## Remarks

This callback can be used to implement your main-loop, and is generally used to update your gameplay/logic/physics.
Usually rendering will occur outside of this callback one time per frame. This callback is only really useful when
using a fixed timestep (see [cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md)), as multiple calls can occur given a single frame's update.

## Related Pages

[cf_set_fixed_timestep](/time/cf_set_fixed_timestep.md)  
[cf_set_fixed_timestep_max_updates](/time/cf_set_fixed_timestep_max_updates.md)  
[cf_update_time](/time/cf_update_time.md)  
[CF_DELTA_TIME_FIXED](/time/cf_delta_time_fixed.md)  
[CF_DELTA_TIME_INTERPOLANT](/time/cf_delta_time_interpolant.md)  
[cf_set_target_framerate](/time/cf_set_target_framerate.md)  
