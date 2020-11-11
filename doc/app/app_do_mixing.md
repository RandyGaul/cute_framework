# app_do_mixing

Performs mixing operations for all currently playing sounds and mixing in a single-threaded way.

## Syntax

```cpp
void app_do_mixing(app_t* app);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.

## Remarks

This function may only be called if the application has audio initialized without a mixing thread. See [app_init_audio](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_init_audio.md) for more details.

## Related Functions

[app_init_audio](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_init_audio.md)  
