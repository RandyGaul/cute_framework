# audio_load_wav

Loads an wav file from disk.

## Syntax

```cpp
audio_t* audio_load_wav(const char* path, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
path | The [virtual path](https://github.com/RandyGaul/cute_framework/blob/master/doc/fill_me_in) to the wav file on disk.
user_allocator_context | Used for custom allocators, this can be set to `NULL`. See (TODO) for more details.

## Return Value

Returns an instance of the raw audio samples.

## Remarks

This function is synchronous. For the async io please try using [audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md).

## Related Functions

[audio_load_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_ogg.md)  
[audio_load_ogg_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_ogg_from_memory.md)  
[audio_load_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_wav_from_memory.md)  
[audio_stream_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg.md)  
[audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md)  
[audio_stream_ogg_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg_from_memory.md)  
[audio_stream_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav_from_memory.md)  
[audio_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_destroy.md)  
[audio_ref_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_ref_count.md)  
