[](../header.md ':include')

# cf_play_sound

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Plays a sound.

```cpp
CF_Sound cf_play_sound(CF_Audio audio_source, CF_SoundParams params);
```

Parameters | Description
--- | ---
audio_source | The [CF_Audio](/audio/cf_audio.md) samples for the sound to play.
params | [CF_SoundParams](/audio/cf_soundparams.md) on how to play the sound. You can use default values by calling [cf_sound_params_defaults](/audio/cf_sound_params_defaults.md).

## Return Value

Returns a playing sound [CF_Sound](/audio/cf_sound.md).

## Related Pages

[CF_SoundParams](/audio/cf_soundparams.md)  
[CF_Sound](/audio/cf_sound.md)  
[cf_sound_params_defaults](/audio/cf_sound_params_defaults.md)  
[cf_sound_set_volume](/audio/cf_sound_set_volume.md)  
[cf_sound_is_active](/audio/cf_sound_is_active.md)  
[cf_sound_get_is_paused](/audio/cf_sound_get_is_paused.md)  
[cf_sound_get_is_looped](/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_sample_index](/audio/cf_sound_set_sample_index.md)  
[cf_sound_set_is_paused](/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](/audio/cf_sound_set_is_looped.md)  
