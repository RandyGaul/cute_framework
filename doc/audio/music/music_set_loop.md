# music_set_loop

Sets the loop setting for music.

## Syntax

```cpp
void music_set_loop(app_t* app, bool true_to_loop);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
true_to_loop | True to loop music, false otherwise.

## Remarks

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_play.md)  
[music_stop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_stop.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_pitch](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_pitch.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_switch_to.md)  
[music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_crossfade.md)  
