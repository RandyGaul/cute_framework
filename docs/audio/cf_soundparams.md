[](../header.md ':include')

# CF_SoundParams

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Parameters for the function [cf_play_sound](/audio/cf_play_sound.md).

Struct Members | Description
--- | ---
`bool paused` | Default: false. True to start the sound in a paused state. False to start immediately playing the sound.
`bool looped` | Default: false. True to loop the sound.
`float volume` | Default: 0.5f. A volume control from 0.0f to 1.0f. 0.0f meaning silent, 1.0f meaning max volume.
`float pan` | Default: 0.5f. A stereo pan control from 0.0f to 1.0f. 0.0f means left-speaker, 1.0f means right speaker, 0.5f means equal both.
`float delay` | Default: 0.0f. A number of seconds to wait before hearing the sound play.

## Remarks

You can use default settings from the [cf_sound_params_defaults](/audio/cf_sound_params_defaults.md) function.

## Related Pages

[cf_sound_stop](/audio/cf_sound_stop.md)  
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
