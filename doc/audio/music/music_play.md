# music_play

Starts playing raw audio as a music track.

## Syntax

```cpp
error_t music_play(app_t* app, audio_t* audio_source, float fade_in_time = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
audio_source | The raw audio samples to reference while playing.
fade_in_time | Number of milliseconds to slowly ramp up the volume from 0 to the volume set by [music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md) (defaults to 1).

## Return Value

Returns any error details upon failure.

## Remarks

Any previously playing music will be stopped instantly.

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_stop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_stop.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_pitch](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_pitch.md)  
[music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_loop.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_switch_to.md)  
[music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_crossfade.md)  
