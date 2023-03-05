# cf_audio_set_sound_volume | [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio_readme.md) | [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)

Sets the volume for all sound effects.

```cpp
void cf_audio_set_sound_volume(float volume);
```

Parameters | Description
--- | ---
volume | A value from 0.0f to 1.0f, where 0.0f means no volume, and 1.0f means full volume.

## Remarks

Sounds come from [cf_play_sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_play_sound.md), as opposed to music coming from [cf_music_play](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_play.md).

## Related Pages

[cf_audio_set_pan](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_pan.md)  
[cf_audio_set_global_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_global_volume.md)  
[cf_play_sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_play_sound.md)  
[cf_audio_set_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_pause.md)  
