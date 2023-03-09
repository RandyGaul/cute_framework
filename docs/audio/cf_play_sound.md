[](../header.md ':include')

# cf_play_sound

Category: [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Plays a sound.

```cpp
CF_Sound cf_play_sound(CF_Audio* audio_source, CF_SoundParams params);
```

Parameters | Description
--- | ---
audio_source | The [CF_Audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio.md) samples for the sound to play.
params | [CF_SoundParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_soundparams.md) on how to play the sound. You can use default values by calling [cf_sound_params_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_params_defaults.md).

## Return Value

Returns a playing sound [CF_Sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound.md).

## Related Pages

[CF_SoundParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_soundparams.md)  
[CF_Sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound.md)  
[cf_sound_params_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_params_defaults.md)  
[cf_sound_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_volume.md)  
[cf_sound_is_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_is_active.md)  
[cf_sound_get_is_paused](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_is_paused.md)  
[cf_sound_get_is_looped](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_sample_index.md)  
[cf_sound_set_is_paused](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_is_looped.md)  
