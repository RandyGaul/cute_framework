[](../header.md ':include')

# cf_sound_get_is_paused

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Returns whether or not a sound is paused.

```cpp
bool cf_sound_get_is_paused(CF_Sound sound);
```

Parameters | Description
--- | ---
sound | The sound.

## Remarks

You can set a sound to paused with [cf_sound_set_is_paused](/audio/cf_sound_set_is_paused.md), or upon creation with [cf_play_sound](/audio/cf_play_sound.md).

## Related Pages

[CF_SoundParams](/audio/cf_soundparams.md)  
[CF_Sound](/audio/cf_sound.md)  
[cf_sound_params_defaults](/audio/cf_sound_params_defaults.md)  
[cf_play_sound](/audio/cf_play_sound.md)  
[cf_sound_is_active](/audio/cf_sound_is_active.md)  
[cf_sound_stop](/audio/cf_sound_stop.md)  
[cf_sound_get_is_looped](/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_sample_index](/audio/cf_sound_set_sample_index.md)  
[cf_sound_set_is_paused](/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](/audio/cf_sound_set_is_looped.md)  
[cf_sound_set_volume](/audio/cf_sound_set_volume.md)  
