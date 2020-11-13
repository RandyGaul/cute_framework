# music_stop

Stops the currently playing music track.

## Syntax

```cpp
error_t music_stop(app_t* app, float fade_out_time = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
fade_out_time | Number of milliseconds to slowly ramp down the volume set by [music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md) (defaults to 1) down to 0.

## Return Value

Returns any error details upon failure.

## Remarks

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_play.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_pitch](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_pitch.md)  
[music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_loop.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_switch_to.md)  
[music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_crossfade.md)  
