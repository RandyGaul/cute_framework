# cf_pause_for | [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time_readme.md) | [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)

Pauses the application.

```cpp
void cf_pause_for(float seconds);
```

Parameters | Description
--- | ---
seconds | A number of seconds to pause the application for.

## Remarks

The entire application can be paused with [cf_pause_for](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_for.md) or [cf_pause_for_ticks](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_for_ticks.md), which pause updates that will
happen when [cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md) is called.

## Related Pages

[CF_PAUSE_TIME_LEFT](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_time_left.md)  
[cf_update_time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_update_time.md)  
[cf_is_paused](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_is_paused.md)  
[cf_pause_for_ticks](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_for_ticks.md)  
