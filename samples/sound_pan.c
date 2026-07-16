/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Generates a short mono sine tone as a 16-bit PCM WAV in memory.
// Looping this gives a continuous drone so stereo pan is easy to hear.
static CF_Audio s_make_tone_audio(float frequency_hz, float duration_seconds)
{
	const int sample_rate = 44100;
	const int channels = 1;
	const int bits_per_sample = 16;
	int sample_count = (int)(sample_rate * duration_seconds);
	if (sample_count < 1) sample_count = 1;

	int data_bytes = sample_count * channels * (bits_per_sample / 8);
	int total_bytes = 44 + data_bytes;
	uint8_t* wav = (uint8_t*)cf_alloc(total_bytes);
	memset(wav, 0, total_bytes);

	// RIFF header
	memcpy(wav + 0, "RIFF", 4);
	*(uint32_t*)(wav + 4) = (uint32_t)(36 + data_bytes);
	memcpy(wav + 8, "WAVE", 4);

	// fmt chunk
	memcpy(wav + 12, "fmt ", 4);
	*(uint32_t*)(wav + 16) = 16; // PCM chunk size
	*(uint16_t*)(wav + 20) = 1;  // PCM format
	*(uint16_t*)(wav + 22) = (uint16_t)channels;
	*(uint32_t*)(wav + 24) = (uint32_t)sample_rate;
	*(uint32_t*)(wav + 28) = (uint32_t)(sample_rate * channels * bits_per_sample / 8);
	*(uint16_t*)(wav + 32) = (uint16_t)(channels * bits_per_sample / 8);
	*(uint16_t*)(wav + 34) = (uint16_t)bits_per_sample;

	// data chunk
	memcpy(wav + 36, "data", 4);
	*(uint32_t*)(wav + 40) = (uint32_t)data_bytes;

	int16_t* samples = (int16_t*)(wav + 44);
	const float two_pi = 6.28318530718f;
	const float amplitude = 0.35f; // keep headroom
	for (int i = 0; i < sample_count; ++i) {
		float t = (float)i / (float)sample_rate;
		float s = sinf(two_pi * frequency_hz * t) * amplitude;
		samples[i] = (int16_t)(s * 32767.0f);
	}

	CF_Audio audio = cf_audio_load_wav_from_memory(wav, total_bytes);
	cf_free(wav);
	return audio;
}

static float s_clampf(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

static float s_pan_from_x(float x, float half_width)
{
	// Map world X into [0, 1]: left edge = 0, right edge = 1, center = 0.5.
	return s_clampf((x / half_width) * 0.5f + 0.5f, 0.0f, 1.0f);
}

int main(int argc, char* argv[])
{
	const int W = 640;
	const int H = 480;
	const float half_w = 280.0f;

	CF_Result result = cf_make_app("Sound Pan", 0, 0, 0, W, H, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// ~1s of 220Hz — loops cleanly for continuous panning demos.
	CF_Audio tone = s_make_tone_audio(220.0f, 1.0f);
	if (!tone.id) {
		printf("Failed to create tone audio.\n");
		cf_destroy_app();
		return -1;
	}

	CF_SoundParams params = cf_sound_params_defaults();
	params.looped = true;
	params.volume = 0.8f;
	params.pan = 0.5f;
	CF_Sound sound = cf_play_sound(tone, params);

	float ship_x = 0.0f;
	float ship_y = 0.0f;
	bool auto_move = true;
	float auto_t = 0.0f;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (cf_key_just_pressed(CF_KEY_SPACE)) {
			auto_move = !auto_move;
		}

		if (auto_move) {
			auto_t += CF_DELTA_TIME;
			// Slow sweep left <-> right.
			ship_x = sinf(auto_t * 0.8f) * half_w;
		} else {
			CF_V2 mouse = cf_screen_to_world(cf_v2(cf_mouse_x(), cf_mouse_y()));
			ship_x = s_clampf(mouse.x, -half_w, half_w);
			ship_y = s_clampf(mouse.y, -120.0f, 120.0f);
		}

		float pan = s_pan_from_x(ship_x, half_w);
		cf_sound_set_pan(sound, pan);

		// --- Visuals -------------------------------------------------------

		// Track line.
		cf_draw_push_color(cf_make_color_rgb(60, 60, 80));
		cf_draw_line(cf_v2(-half_w, 0), cf_v2(half_w, 0), 2.0f);
		cf_draw_pop_color();

		// Left / right speaker markers (filled more when that side is louder).
		float left_gain = 1.0f - pan;
		float right_gain = pan;

		cf_draw_push_color(cf_make_color_rgba(80, 180, 255, (uint8_t)(80 + left_gain * 175)));
		cf_draw_circle_fill2(cf_v2(-half_w - 30.0f, 0), 18.0f + left_gain * 12.0f);
		cf_draw_pop_color();

		cf_draw_push_color(cf_make_color_rgba(255, 140, 80, (uint8_t)(80 + right_gain * 175)));
		cf_draw_circle_fill2(cf_v2(half_w + 30.0f, 0), 18.0f + right_gain * 12.0f);
		cf_draw_pop_color();

		// "Ship" body.
		cf_draw_push_color(cf_color_white());
		cf_draw_circle_fill2(cf_v2(ship_x, ship_y), 14.0f);
		cf_draw_pop_color();
		cf_draw_push_color(cf_make_color_rgb(40, 200, 120));
		cf_draw_circle2(cf_v2(ship_x, ship_y), 14.0f, 3.0f);
		// Nose pointing direction of travel in auto mode.
		float nose = auto_move ? cosf(auto_t * 0.8f) : 0.0f;
		cf_draw_line(cf_v2(ship_x, ship_y), cf_v2(ship_x + nose * 28.0f, ship_y), 3.0f);
		cf_draw_pop_color();

		// Drop a vertical guide at the ship.
		cf_draw_push_color(cf_make_color_rgba(255, 255, 255, 40));
		cf_draw_line(cf_v2(ship_x, -140.0f), cf_v2(ship_x, 140.0f), 1.0f);
		cf_draw_pop_color();

		// HUD text.
		char line[128];
		cf_push_font_size(18.0f);

		snprintf(line, sizeof(line), "pan  %.2f   (0 = left, 0.5 = center, 1 = right)", pan);
		float tw = cf_text_width(line, -1);
		cf_draw_text(line, cf_v2(-tw * 0.5f, 180.0f), -1);

		snprintf(line, sizeof(line), "get_pan  %.2f", cf_sound_get_pan(sound));
		tw = cf_text_width(line, -1);
		cf_draw_text(line, cf_v2(-tw * 0.5f, 155.0f), -1);

		const char* mode = auto_move ? "AUTO  (Space: mouse control)" : "MOUSE  (Space: auto sweep)";
		tw = cf_text_width(mode, -1);
		cf_draw_text(mode, cf_v2(-tw * 0.5f, -180.0f), -1);

		cf_draw_text("L", cf_v2(-half_w - 36.0f, -40.0f), -1);
		cf_draw_text("R", cf_v2(half_w + 24.0f, -40.0f), -1);

		cf_app_draw_onto_screen(true);
	}

	cf_sound_stop(sound);
	cf_audio_destroy(tone);
	cf_destroy_app();
	return 0;
}
