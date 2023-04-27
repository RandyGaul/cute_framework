[](../header.md ':include')

<br>

Cute Framework supports both stereo music and stereo sound FX. Music is great for switching/fading from one track to another, while sound FX are good for playing individual clips of audio. All audio files are expected to be in either .wav format (16-bit pcm) or .ogg format. It's recommended you save your audio files in 44100hz.

## Music

First, load some audio from either a .wav file (16-bit pcm) or .ogg format. You can load up .wav files with [`cf_audio_load_wav`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_load_wav). For .ogg files use [`cf_audio_load_ogg`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_load_ogg). Both functions will give you back a [`CF_Audio`](https://randygaul.github.io/cute_framework/#/audio/cf_audio) representing the raw audio samples loaded into memory. This is _not_ a playable instance.

To actually play the audio as a music track call [`cf_music_play`](https://randygaul.github.io/cute_framework/#/audio/cf_music_play). You may switch or fade between tracks using [`cf_music_switch_to`](https://randygaul.github.io/cute_framework/#/audio/cf_music_switch_to) or [`cf_music_crossfade`](https://randygaul.github.io/cute_framework/#/audio/cf_music_crossfade). Only one music track can be played at any time; switching to or playing another music track will cause the previous music to stop playing.

> Loading some music from a .ogg file and playing it.

```cpp
CF_Audio* my_song = NULL;

void LoadMusic()
{
	my_song = cf_audio_load_ogg("/music/song.ogg");
	CF_ASSERT(my_song);
}

void PlayMusic(float fade_in_time = 0)
{
	cf_music_play(my_song, fade_in_time);
}
```

You can control music specific settings such as volume, looping, and pause with the following functions (these functions do not affect sound FX at all):

- [`cf_music_set_volume`](https://randygaul.github.io/cute_framework/#/audio/cf_music_set_volume)
- [`cf_music_set_loop`](https://randygaul.github.io/cute_framework/#/audio/cf_music_set_loop)
- [`cf_music_pause`](https://randygaul.github.io/cute_framework/#/audio/cf_music_pause)

## Sound FX

To play a sound call [`cf_play_sound`](https://randygaul.github.io/cute_framework/#/audio/cf_play_sound). This takes a [`CF_Audio`](https://randygaul.github.io/cute_framework/#/audio/cf_audio) pointer and [`CF_SoundParams`](https://randygaul.github.io/cute_framework/#/audio/cf_soundparams). You get back a [`CF_Sound`](https://randygaul.github.io/cute_framework/#/audio/cf_sound) handle representing an actual playing instance of a sound. The instance will stay alive as long as the sound effect is still playing. This is different than the raw audio samples represented by [`CF_Audio`](https://randygaul.github.io/cute_framework/#/audio/cf_audio). Many different sound FX can reference a single [`CF_Audio`](https://randygaul.github.io/cute_framework/#/audio/cf_audio).

> Playing a few different sound FX. [`CF_SoundParams`](https://randygaul.github.io/cute_framework/#/audio/cf_soundparams) contains all of the initialization parameters for a sound effect.

```cpp
CF_Audio* jump = NULL;
CF_Audio* bonk = NULL;

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

See the page [`CF_SoundParams`](https://randygaul.github.io/cute_framework/#/audio/cf_soundparams) to view all the different settings available. There's more than just looping, including pan, volume, pause state, etc.

You can play many sound FX all simultaneously, up to many thousands without hitting any kind of performance difference on many platforms.

?> For the web unfortunately the entire application is single threaded, making audio significantly more expensive than other platforms.

You may globally control sound FX volume with [`cf_audio_set_sound_volume`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_set_sound_volume).

## Global Controls

Global controls are available, of which affect both music and sound FX.

- [`cf_audio_set_global_volume`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_set_global_volume)
- [`cf_audio_set_pan`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_set_pan)
- [`cf_audio_set_pause`](https://randygaul.github.io/cute_framework/#/audio/cf_audio_set_pause)
