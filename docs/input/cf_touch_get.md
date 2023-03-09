[](../header.md ':include')

# cf_touch_get

Category: [input](/api_reference?id=input)  
GitHub: [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)  
---

Fetches a specific touch event.

```cpp
bool cf_touch_get(uint64_t id, CF_Touch* touch);
```

Parameters | Description
--- | ---
id | The unique identifier of a specific touch event.
touch | Pointer to a [CF_Touch](/input/cf_touch.md) to fill in.

## Return Value

Returns true if the touch event was found and currently still active.

## Remarks

You should use [cf_touch_get_all](/input/cf_touch_get_all.md) to peek at all current touch events. Make note of any touch events that are
new. Then, you can loop over all touch events you've noted with this function, and remove them when they
become unavailable.

## Related Pages

[CF_Touch](/input/cf_touch.md)  
[cf_touch_get_all](/input/cf_touch_get_all.md)  
