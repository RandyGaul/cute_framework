# cf_sound_set_sample_index | [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio_readme.md) | [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)

Sets the sample index for the sound to control which sample to play next.

```cpp
void cf_sound_set_sample_index(CF_Sound sound, uint64_t sample_index);
```

Parameters | Description
--- | ---
sound | The sound.
sample_index | The index of the sample to play the sound from.

## Remarks

You can get a sound's playing index with [cf_sound_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_sample_index.md). This can be useful to sync a dynamic audio system that
can turn on/off different instruments or sounds.

## Related Pages

[CF_SoundParams](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_soundparams.md)  
[CF_Sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound.md)  
[cf_sound_params_defaults](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_params_defaults.md)  
[cf_play_sound](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_play_sound.md)  
[cf_sound_is_active](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_is_active.md)  
[cf_sound_get_is_paused](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_is_paused.md)  
[cf_sound_get_is_looped](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_is_looped.md)  
[cf_sound_get_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_volume.md)  
[cf_sound_get_sample_index](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_get_sample_index.md)  
[cf_sound_set_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_volume.md)  
[cf_sound_set_is_paused](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_is_paused.md)  
[cf_sound_set_is_looped](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_sound_set_is_looped.md)  
