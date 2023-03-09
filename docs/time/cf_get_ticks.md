[](../header.md ':include')

# cf_get_ticks

Category: [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Returns the number of ticks elapsed _right now_ since program start.

```cpp
uint64_t cf_get_ticks();
```

## Remarks

`CF_TICK` and [CF_SECONDS](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_seconds.md) are only recorded once at the beginning of an update (see [cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)). This function instead
queries the application for the number of ticks _right now_. Mostly useful for performance measuring.

## Related Pages

[cf_get_tick_frequency](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_get_tick_frequency.md)  
