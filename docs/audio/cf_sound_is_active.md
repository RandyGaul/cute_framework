[](../header.md ':include')

# cf_sound_is_active

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Returns whether or not a sound is active.

```cpp
bool cf_sound_is_active(CF_Sound sound);
```

Parameters | Description
--- | ---
sound | The sound.

## Return Value

Rreturns true if the sound is active, or false if it finished playing (and was not looped).

## Related Pages

[CF_SoundParams](/audio/cf_soundparams.md)  
[CF_Sound](/audio/cf_sound.md)  
[cf_sound_params_defaults](/audio/cf_sound_params_defaults.md)  
[cf_play_sound](/audio/cf_play_sound.md)  
[cf_sound_set_volume](/audio/cf_sound_set_volume.md)  
[cf_sound_get_is_paused](/audio/cf_sound_get_is_paused.md)  
[cf_sound_get_is_looped](/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_sample_index](/audio/cf_sound_set_sample_index.md)  
[cf_sound_set_is_paused](/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](/audio/cf_sound_set_is_looped.md)  
