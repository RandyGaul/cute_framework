[](../header.md ':include')

# CF_TICKS

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

The number of machine ticks since program start.

## Remarks

The frequency of these ticks is platform dependent. If you want to convert these ticks to relative to one second of actual time
call [cf_get_tick_frequency](/time/cf_get_tick_frequency.md) and use it as a divisor.

## Related Pages

[CF_SECONDS](/time/cf_seconds.md)  
[CF_PREV_TICKS](/time/cf_prev_ticks.md)  
[cf_get_ticks](/time/cf_get_ticks.md)  
[cf_get_tick_frequency](/time/cf_get_tick_frequency.md)  
[cf_make_stopwatch](/time/cf_make_stopwatch.md)  
