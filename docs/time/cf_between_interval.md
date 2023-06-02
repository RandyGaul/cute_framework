[](../header.md ':include')

# cf_between_interval

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Returns true for one interval of seconds, every other interval.

```cpp
bool cf_between_interval(float interval, float offset);
```

Parameters | Description
--- | ---
interval | Number of seconds between each interval.
offset | A starting offset in seconds for the interval. This gets mathematically modulo'd. Used to sync times for rythms or repeats.

## Remarks

This function is super useful for general purpose gameplay implementation where you want an event to fire for N seconds,
and then _not_ fire for N seconds, flipping back and forth periodically. Simply place this within an if-statement.

## Related Pages

[cf_on_interval](/time/cf_on_interval.md)  
[cf_on_timestamp](/time/cf_on_timestamp.md)  
