# app_init_audio

Initializes the audio system for the application.

## Syntax

```cpp
void app_init_audio(app_t* app, bool spawn_mix_thread = true, int max_simultaneous_sounds = 5000);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
spawn_mix_thread | If true a dedicated thread for mixing will be spawned. If this is false you must call [app_do_mixing](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_do_mixing.md) yourself.
max_simultaneous_sounds | The number of simultaneously playing sound instances allowed at any given time.

## Related Functions

[app_do_mixing](https://github.com/RandyGaul/cute_framework/blob/master/doc/app/app_do_mixing.md)  
