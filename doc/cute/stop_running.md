# stop_running

Signals to Cute to stop running, and will cause [is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/is_running.md) to return `0`.

## Syntax

```cpp
void stop_running(cute_t* cute);
```

## Function Parameters

Parameter Name | Description
--- | ---
cute | The instance of the Cute framework to signal to stop running.

## Related Functions

[cute_make](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/cute_make.md)
[cute_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/cute_destroy.md)
[is_running](https://github.com/RandyGaul/cute_framework/blob/master/doc/cute/is_running.md)
