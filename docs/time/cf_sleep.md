# cf_sleep | [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/README.md) | [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)

Waits an estimated number of milliseconds before returning.

```cpp
void cf_sleep(int milliseconds);
```

## Remarks

This function actually sleeps the application. If you want to instead pause updates without locking the entire
applcation (so you can e.g. continue rendering and capturing user inputs) use [cf_pause_for](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_for.md) instead.

## Related Pages

[cf_get_ticks](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_get_ticks.md)  
[cf_get_tick_frequency](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_get_tick_frequency.md)  
[cf_pause_for](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_pause_for.md)  
