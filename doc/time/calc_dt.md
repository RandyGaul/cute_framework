
# calc_dt

Returns whether or not the window is maximized.

## Syntax

```cpp
float calc_dt();
```

## Return Value

Calculates the time, in seconds, since the last time this function was called. No special care is taken to handle multi-threading (this function uses static memory). Returns 0 on the first call.

For more fine-grained measuring of time, try using `timer_t`.

## Related Functions

[timer_init](https://github.com/RandyGaul/cute_framework/blob/master/doc/time/timer_init.md)  
[timer_dt](https://github.com/RandyGaul/cute_framework/blob/master/doc/time/timer_dt.md)  
[timer_elapsed](https://github.com/RandyGaul/cute_framework/blob/master/doc/time/timer_elapsed.md)  
