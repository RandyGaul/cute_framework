# music_set_pitch

Sets the pitch for music.

## Syntax

```cpp
void music_set_pitch(app_t* app, float pitch);
```

## Function Parameters

Parameter Name | Description
--- | ---
app | The application.
pitch | A number from 0 to 1, where 0.5f means no pitch adjustment.

## Remarks

Adjusting pitch happens in real-time and is a very expensive operation, and is *only an approximation*. You've been warned.

Pitch values farther from 0.5f create more distortion. Stay around 0.5f for the best quality pitch adjustment.

The music API is a higher level version of the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function, and is mostly for convenience when wanting to fade or crossfade one or two music tracks together. For more fine-grained and custom control, use the [sound_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/sound/sound_play.md) function.

## Related Functions

[music_play](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_play.md)  
[music_stop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_stop.md)  
[music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_volume.md)  
[music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_set_loop.md)  
[music_pause](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_pause.md)  
[music_resume](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_resume.md)  
[music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_switch_to.md)  
[music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/music/music_crossfade.md)  
