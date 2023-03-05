# CF_Stopwatch | [time](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/README.md) | [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)

A stopwatch for general purpose precise timings.

Struct Members | Description
--- | ---
`uint64_t start_time` | The starting time (in ticks) of the stopwatch. See [cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md).

## Remarks

Once created with [cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md) the time elapsed can be fetched. To reset the stopwatch, simply call
[cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md) again and overwrite your old stopwatch.

## Related Pages

cf_microseconds  
[cf_make_stopwatch](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_make_stopwatch.md)  
[cf_seconds](https://github.com/RandyGaul/cute_framework/blob/master/docs/time/cf_seconds.md)  
cf_milliseconds  
