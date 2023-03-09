[](../header.md ':include')

# cf_audio_set_pan

Category: [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Sets the global stereo pan for all audio.

```cpp
void cf_audio_set_pan(float pan);
```

Parameters | Description
--- | ---
pan | 0.5f means perfect balance for left/right speakers. 0.0f means only left speaker, 1.0f means only right speaker.

## Related Pages

[cf_audio_set_pause](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_pause.md)  
[cf_audio_set_global_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_global_volume.md)  
[cf_audio_set_sound_volume](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_set_sound_volume.md)  
