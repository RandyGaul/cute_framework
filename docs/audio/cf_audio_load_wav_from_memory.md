[](../header.md ':include')

# cf_audio_load_wav_from_memory

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Loads a .wav audio file from memory.

```cpp
CF_Audio* cf_audio_load_wav_from_memory(void* memory, int byte_count);
```

Parameters | Description
--- | ---
memory | A buffer containing the bytes of a .wav file.
byte_count | The number of bytes in `memory`.

## Return Value

Returns a pointer to [CF_Audio](/audio/cf_audio.md). Free it up with [cf_audio_destroy](/audio/cf_audio_destroy.md) when done.

## Related Pages

[CF_Audio](/audio/cf_audio.md)  
[cf_audio_load_ogg](/audio/cf_audio_load_ogg.md)  
[cf_audio_load_ogg_from_memory](/audio/cf_audio_load_ogg_from_memory.md)  
[cf_audio_load_wav](/audio/cf_audio_load_wav.md)  
[cf_audio_destroy](/audio/cf_audio_destroy.md)  
