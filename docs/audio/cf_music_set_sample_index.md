# cf_music_set_sample_index | [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio_readme.md) | [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)

Sets the sample index to play at for the music.

```cpp
CF_Result cf_music_set_sample_index(uint64_t sample_index);
```

Parameters | Description
--- | ---
sample_index | Tells where to play music from within the [CF_Audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio.md) for the currently playing music track.

## Remarks

This can be useful to sync a dynamic audio system that can turn on/off different instruments or sounds.

## Related Pages

[cf_music_play](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_play.md)  
[cf_music_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_stop.md)  
[cf_music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_volume.md)  
[cf_music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_loop.md)  
[cf_music_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_pause.md)  
[cf_music_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_resume.md)  
[cf_music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_switch_to.md)  
[cf_music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_crossfade.md)  
[cf_music_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_get_sample_index.md)  
