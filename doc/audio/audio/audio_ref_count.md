# audio_ref_count

Returns the reference count of an audio instance.

## Syntax

```cpp
int audio_ref_count(audio_t* audio);
```

## Function Parameters

Parameter Name | Description
--- | ---
audio | The raw audio samples to fetch the reference count of.

## Return Value

Returns the reference count of an audio instance. For every playing sound or music that references these raw audio samples, it's internal reference count will be incremented by one. Once the playing instance finishes or destructs the reference count will be decremented by one.

## Related Functions

[audio_load_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_ogg.md)  
[audio_load_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_wav.md)  
[audio_load_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_load_wav_from_memory.md)  
[audio_stream_ogg](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg.md)  
[audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md)  
[audio_stream_wav](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav.md)  
[audio_stream_ogg_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_ogg_from_memory.md)  
[audio_stream_wav_from_memory](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_stream_wav_from_memory.md)  
[audio_destroy](https://github.com/RandyGaul/cute_framework/blob/master/doc/audio/audio/audio_destroy.md)  
