[](../header.md ':include')

# cf_sleep

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Waits an estimated number of milliseconds before returning.

```cpp
CF_API void CF_CALL cf_sleep(int milliseconds);
```

## Remarks

This function actually sleeps the application. If you want to instead pause updates without locking the entire
applcation (so you can e.g. continue rendering and capturing user inputs) use [cf_pause_for](/time/cf_pause_for.md) instead.

## Related Pages

[cf_get_ticks](/time/cf_get_ticks.md)  
[cf_get_tick_frequency](/time/cf_get_tick_frequency.md)  
[cf_pause_for](/time/cf_pause_for.md)  
