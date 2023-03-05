# cf_touch_get_all | [input](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/README.md) | [cute_input.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_input.h)

Returns an array of all touch events.

```cpp
int cf_touch_get_all(CF_Touch** touch_all);
```

Parameters | Description
--- | ---
touch_all | An array of all [CF_Touch](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_touch.md) touch events. See example section.

## Return Value

Returns the number of [CF_Touch](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_touch.md) events in `touch_all`.

## Code Example

> Looping over all touch events.

```cpp
CF_Touch touches = NULL;
int touch_count = cf_touch_get_all(&touches);
for (int i = 0; i < touch_count; ++i) {
    do_something(touches[i]);
}
```

## Related Pages

[CF_Touch](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_touch.md)  
[cf_touch_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/input/cf_touch_get.md)  
