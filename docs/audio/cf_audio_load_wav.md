# cf_audio_load_wav | [audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio_readme.md) | [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)

Loads a .wav audio file.

```cpp
CF_Audio* cf_audio_load_wav(const char* path);
```

Parameters | Description
--- | ---
path | The virtual path TODO_LINK to a .wav file.

## Return Value

Returns a pointer to [CF_Audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio.md). Free it up with [cf_audio_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_destroy.md) when done.

## Related Pages

[CF_Audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio.md)  
[cf_audio_load_ogg](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_load_ogg.md)  
[cf_audio_load_ogg_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_load_ogg_from_memory.md)  
[cf_audio_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_destroy.md)  
[cf_audio_load_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/docs/audio/cf_audio_load_wav_from_memory.md)  
