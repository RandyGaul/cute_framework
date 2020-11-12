# music_crossfade

Switches the music from one track to another simultaneously with a crossfade effect.

## Syntax

```cpp
error_t music_crossfade(app_t* app, audio_t* audio_source, float cross_fade_time = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
audio_source | The raw audio samples to switch to.
cross_fade_time | Time in milliseconds to crossfade the tracks.

## Remarks

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_play.md)  
[music_stop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_stop.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_pitch](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_pitch.md)  
[music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_loop.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_switch_to.md)  
