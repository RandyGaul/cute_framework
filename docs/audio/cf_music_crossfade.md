# cf_music_crossfade | [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio_readme.md) | [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)

Crossfades the currently playing track out with the next track in.

```cpp
void cf_music_crossfade(CF_Audio* audio_source, float cross_fade_time);
```

Parameters | Description
--- | ---
cross_fade_time | A number of seconds to crossfade to the next track. Can be 0.0f to instantly switch to the next track.

## Related Pages

[cf_music_play](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_play.md)  
[cf_music_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_stop.md)  
[cf_music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_volume.md)  
[cf_music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_loop.md)  
[cf_music_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_pause.md)  
[cf_music_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_resume.md)  
[cf_music_switch_to](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_switch_to.md)  
[cf_music_set_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_sample_index.md)  
[cf_music_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_get_sample_index.md)  
