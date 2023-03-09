# cf_music_switch_to

Category: [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Switches from the currently playing music to another track.

```cpp
void cf_music_switch_to(CF_Audio* audio_source, float fade_out_time, float fade_in_time);
```

Parameters | Description
--- | ---
fade_out_time | A number of seconds to fade the currently playing track out. Can be 0.0f to instantly stop your music.
fade_in_time | A number of seconds to fade the next track in. Can be 0.0f to instantly play your music.

## Remarks

The currently playing track is faded out, then the second track is faded in.

## Related Pages

[cf_music_play](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_play.md)  
[cf_music_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_stop.md)  
[cf_music_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_volume.md)  
[cf_music_set_loop](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_loop.md)  
[cf_music_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_pause.md)  
[cf_music_resume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_resume.md)  
[cf_music_set_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_set_sample_index.md)  
[cf_music_crossfade](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_crossfade.md)  
[cf_music_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_music_get_sample_index.md)  
