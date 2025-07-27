# Audio

Cute Framework supports both stereo music and stereo sound FX. Music is great for switching/fading from one track to another, while sound FX are good for playing individual clips of audio. All audio files are expected to be in either .wav format (16-bit pcm) or .ogg format. It's recommended you save your audio files in 44100hz.

## Music

First, load some audio from either a .wav file (16-bit pcm) or .ogg format. You can load up .wav files with [`cf_audio_load_wav`](../audio/cf_audio_load_wav.md). For .ogg files use [`cf_audio_load_ogg`](../audio/cf_audio_load_ogg.md). Both functions will give you back a [`CF_Audio`](../audio/cf_audio.md) representing the raw audio samples loaded into memory. This is _not_ a playable instance.

To actually play the audio as a music track call [`cf_music_play`](../audio/cf_music_play.md). You may switch or fade between tracks using [`cf_music_switch_to`](../audio/cf_music_switch_to.md) or [`cf_music_crossfade`](../audio/cf_music_crossfade.md). Only one music track can be played at any time; switching to or playing another music track will cause the previous music to stop playing.

> Loading some music from a .ogg file and playing it.

```cpp
CF_Audio my_song;

void LoadMusic()
{
	my_song = cf_audio_load_ogg("/music/song.ogg");
}

void PlayMusic(float fade_in_time = 0)
{
	cf_music_play(my_song, fade_in_time);
}
```

You can control music specific settings such as volume, looping, and pause with the following functions (these functions do not affect sound FX at all):

- [`cf_music_set_volume`](../audio/cf_music_set_volume.md)
- [`cf_music_set_loop`](../audio/cf_music_set_loop.md)
- [`cf_music_set_pitch`](../audio/cf_music_set_pitch.md)
- [`cf_music_pause`](../audio/cf_music_pause.md)

## Sound FX

To play a sound call [`cf_play_sound`](../audio/cf_play_sound.md). This takes a [`CF_Audio`](../audio/cf_audio.md) pointer and [`CF_SoundParams`](../audio/cf_soundparams.md). You get back a [`CF_Sound`](../audio/cf_sound.md) handle representing an actual playing instance of a sound. The instance will stay alive as long as the sound effect is still playing. This is different than the raw audio samples represented by [`CF_Audio`](../audio/cf_audio.md). Many different sound FX can reference a single [`CF_Audio`](../audio/cf_audio.md).

> Playing a few different sound FX. [`CF_SoundParams`](../audio/cf_soundparams.md) contains all of the initialization parameters for a sound effect.

```cpp
CF_Audio jump;
CF_Audio bonk;

void LoadSounds()
{
	jump = cf_audio_load_wav("/sounds/jump.wav");
	bonk = cf_audio_load_wav("/sounds/bonk.wav");
}

// Play a sound based on an enum in your game called `SoundFX`, with an optional
// setting for looping (on by default).
void PlaySound(SoundFX fx, bool loop = false)
{
	CF_SoundParams params = cf_sound_params_defaults();
	params.loop = loop;

	switch (fx) {
	case JUMP_FX: cf_play_sound(jump, params); break;
	case BONK_FX: cf_play_sound(bonk, params); break;
	default: CF_ASSERT(false); break; // Unknown sound type.
	}
}
```

See the page [`CF_SoundParams`](../audio/cf_soundparams.md) to view all the different settings available. There's more than just looping, including pan, volume, pause state, pitch, etc.

You can play many sound FX all simultaneously, up to many thousands without hitting any kind of performance difference on many platforms.

?> For the web unfortunately the entire application is single threaded, making audio significantly more expensive than other platforms.

You may globally control sound FX volume with [`cf_audio_set_sound_volume`](../audio/cf_audio_set_sound_volume.md).

## Global Controls

Global controls are available, of which affect both music and sound FX.

- [`cf_audio_set_global_volume`](../audio/cf_audio_set_global_volume.md)
- [`cf_audio_set_pan`](../audio/cf_audio_set_pan.md)
- [`cf_audio_set_pause`](../audio/cf_audio_set_pause.md)
