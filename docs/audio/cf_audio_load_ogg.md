[](../header.md ':include')

# cf_audio_load_ogg

Category: [audio](/api_reference?id=audio)  
GitHub: [cute_audio.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_audio.h)  
---

Loads a .ogg audio file.

```cpp
CF_Audio cf_audio_load_ogg(const char* path);
```

Parameters | Description
--- | ---
path | The virtual path to a .ogg file. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

## Return Value

Returns a pointer to [CF_Audio](/audio/cf_audio.md). Free it up with [cf_audio_destroy](/audio/cf_audio_destroy.md) when done.

## Related Pages

[CF_Audio](/audio/cf_audio.md)  
[cf_audio_destroy](/audio/cf_audio_destroy.md)  
[cf_audio_load_ogg_from_memory](/audio/cf_audio_load_ogg_from_memory.md)  
[cf_audio_load_wav](/audio/cf_audio_load_wav.md)  
[cf_audio_load_wav_from_memory](/audio/cf_audio_load_wav_from_memory.md)  
