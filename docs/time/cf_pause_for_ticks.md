[](../header.md ':include')

# cf_pause_for_ticks

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Pauses the application.

```cpp
void cf_pause_for_ticks(uint64_t pause_ticks);
```

Parameters | Description
--- | ---
ticks | A number of ticks to pause the application for. See [cf_get_tick_frequency](/time/cf_get_tick_frequency.md).

## Remarks

The entire application can be paused with [cf_pause_for](/time/cf_pause_for.md) or [cf_pause_for_ticks](/time/cf_pause_for_ticks.md), which pause updates that will
happen when [cf_update_time](/time/cf_update_time.md) is called.

## Related Pages

[CF_PAUSE_TIME_LEFT](/time/cf_pause_time_left.md)  
[cf_update_time](/time/cf_update_time.md)  
[cf_pause_for](/time/cf_pause_for.md)  
[cf_is_paused](/time/cf_is_paused.md)  
