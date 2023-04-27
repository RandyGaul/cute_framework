[](../header.md ':include')

# cf_music_crossfade

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Crossfades the currently playing track out with the next track in.

```cpp
void cf_music_crossfade(CF_Audio* audio_source, float cross_fade_time);
```

Parameters | Description
--- | ---
cross_fade_time | A number of seconds to crossfade to the next track. Can be 0.0f to instantly switch to the next track.

## Related Pages

[cf_music_play](/audio/cf_music_play.md)  
[cf_music_stop](/audio/cf_music_stop.md)  
[cf_music_set_volume](/audio/cf_music_set_volume.md)  
[cf_music_set_loop](/audio/cf_music_set_loop.md)  
[cf_music_pause](/audio/cf_music_pause.md)  
[cf_music_resume](/audio/cf_music_resume.md)  
[cf_music_switch_to](/audio/cf_music_switch_to.md)  
[cf_music_set_sample_index](/audio/cf_music_set_sample_index.md)  
[cf_music_get_sample_index](/audio/cf_music_get_sample_index.md)  
