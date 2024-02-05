[](../header.md ':include')

# cf_audio_cull_duplicates

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Turns on/off duplicate culling.

```cpp
void cf_audio_cull_duplicates(bool true_to_cull_duplicates);
```

## Remarks

This is turned on by default. If the exact same sound is played more than once on a given audio update it will simply
amplify the volume of the sound. This can create an audio bug where the sound plays way louder than intended. This
setting simply culls away any extra calls to `play_sound` for a particular sound.

The audio update rate is determined by how quickly the audio mixing thread runs, meaning exactly how many sounds can
get culled within a particular timespan is variable -- but usually you still want this option turned on.

When on this applies to both sound FX and music.

## Related Pages

[CF_SoundParams](/audio/cf_soundparams.md)  
[CF_Sound](/audio/cf_sound.md)  
[cf_sound_params_defaults](/audio/cf_sound_params_defaults.md)  
[cf_play_sound](/audio/cf_play_sound.md)  
[cf_sound_is_active](/audio/cf_sound_is_active.md)  
[cf_sound_get_is_paused](/audio/cf_sound_get_is_paused.md)  
[cf_sound_get_is_looped](/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_sample_index](/audio/cf_sound_set_sample_index.md)  
[cf_sound_set_is_paused](/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](/audio/cf_sound_set_is_looped.md)  
[cf_sound_set_volume](/audio/cf_sound_set_volume.md)  
