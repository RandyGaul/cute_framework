[](../header.md ':include')

# cf_sound_set_is_paused

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Sets the paused state for the sound.

```cpp
void cf_sound_set_is_paused(CF_Sound sound, bool true_for_paused);
```

Parameters | Description
--- | ---
sound | The sound.
true_for_paused | The pause state to set.

## Remarks

You can get a sound's paused state with [cf_sound_get_is_paused](/audio/cf_sound_get_is_paused.md).

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
[cf_sound_set_volume](/audio/cf_sound_set_volume.md)  
[cf_sound_set_is_looped](/audio/cf_sound_set_is_looped.md)  
