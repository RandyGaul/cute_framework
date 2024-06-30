[](../header.md ':include')

# cf_music_play

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Plays audio as music.

```cpp
void cf_music_play(CF_Audio audio_source, float fade_in_time);
```

Parameters | Description
--- | ---
audio_source | A [CF_Audio](/audio/cf_audio.md) containing your music.
fade_in_time | A number of seconds to fade the music in. Can be 0.0f to instantly play your music.

## Related Pages

[cf_music_set_pitch](/audio/cf_music_set_pitch.md)  
[cf_music_stop](/audio/cf_music_stop.md)  
[cf_music_set_volume](/audio/cf_music_set_volume.md)  
[cf_music_set_loop](/audio/cf_music_set_loop.md)  
[cf_music_pause](/audio/cf_music_pause.md)  
[cf_music_resume](/audio/cf_music_resume.md)  
[cf_music_switch_to](/audio/cf_music_switch_to.md)  
[cf_music_crossfade](/audio/cf_music_crossfade.md)  
[cf_music_get_sample_index](/audio/cf_music_get_sample_index.md)  
[cf_music_set_sample_index](/audio/cf_music_set_sample_index.md)  
