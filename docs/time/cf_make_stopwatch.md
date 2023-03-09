# cf_make_stopwatch

Category: [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Records a new `start_time` for a [CF_Stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch.md).

```cpp
CF_Stopwatch cf_make_stopwatch();
```

## Return Value

Returns an initialize [CF_Stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch.md) to the time of _right now_ (e.g. with [cf_get_ticks](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_get_ticks.md) internally).

## Remarks

Once created with [cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md) the time elapsed can be fetched. To reset the stopwatch, simply call
[cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md) again and overwrite your old stopwatch. To fetch the time elapsed, call [cf_stopwatch_seconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch_seconds.md),
[cf_stopwatch_milliseconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch_milliseconds.md), or `cf_microseconds`.

## Related Pages

[CF_Stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch.md)  
[cf_stopwatch_microseconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch_microseconds.md)  
[cf_stopwatch_seconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch_seconds.md)  
[cf_stopwatch_milliseconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_stopwatch_milliseconds.md)  
