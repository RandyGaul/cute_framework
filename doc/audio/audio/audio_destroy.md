# audio_destroy

Destroys an instance of raw audio samples.

## Syntax

```cpp
error_t audio_destroy(audio_t* audio);
```

## Function Parameters

Parameter Name | Description
--- | ---
audio | The raw audio samples to destroy.

## Return Value

Returns an `error_t` containing any error details on failure. This function will fail if the reference count for the audio instance is not 0.

## Related Functions

[audio_load_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_ogg.md)  
[audio_load_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_wav.md)  
[audio_load_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_wav_from_memory.md)  
[audio_stream_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg.md)  
[audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md)  
[audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md)  
[audio_stream_ogg_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg_from_memory.md)  
[audio_stream_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav_from_memory.md)  
[audio_ref_count](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_ref_count.md)  
