[](../header.md ':include')

# cf_on_interval

Category: [time](/api_reference?id=time)  
GitHub: [cute_time.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_time.h)  
---

Returns true for one frame update each time an interval of seconds elapses.

```cpp
CF_API bool CF_CALL cf_on_interval(float interval, float offset);
```

Parameters | Description
--- | ---
interval | Number of seconds between each interval.
offset | An offset of seconds used to offset within the interval of [0,interval]. This gets mathematically mod'd,
           so it can be any number of seconds.

## Remarks

This function is super useful for general purpose gameplay implementation where you want an event to fire every N seconds.
Simply place this within an if-statement!

## Related Pages

[cf_on_timestamp](/time/cf_on_timestamp.md)  
[cf_between_interval](/time/cf_between_interval.md)  
