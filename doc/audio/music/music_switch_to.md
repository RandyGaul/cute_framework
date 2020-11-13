# music_switch_to

Switches the music from one track to another.

## Syntax

```cpp
error_t music_switch_to(app_t* app, audio_t* audio_source, float fade_out_time = 0, float fade_in_time = 0);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
audio_source | The raw audio samples to switch to.
fade_out_time | Time in milliseconds to fade out the previously playing music.
fade_in_time | Time in milliseconds to fade in the new music.

## Remarks

This function does not perform a [music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md) effect. One track is stopped completely before the next is started.

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_play.md)  
[music_stop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_stop.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_pitch](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_pitch.md)  
[music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_loop.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_crossfade.md)  
