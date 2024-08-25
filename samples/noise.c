#include <cute.h>
#include <cimgui/cimgui.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Noise Sample", 0, 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui(false);

	float scale = 1.5f;
	float lacunarity = 2.0f;
	int octaves = 3;
	float falloff = 0.5f;
	float frequency = 1.0f;
	float time_amplitude = 0.1f;

	int w = 128;
	int h = 128;

	//CF_Pixel* pix = cf_noise_pixels_wrapped(w, h, 0, scale, 0, time_amplitude);
	CF_Pixel* pix = cf_noise_fbm_pixels_wrapped(w, h, 0, scale, lacunarity, octaves, falloff, 0, time_amplitude);
	CF_Sprite noise = cf_make_easy_sprite_from_pixels(pix, w, h);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		igBegin("Noise Sample", NULL, 0);
		igSliderFloat("scale", &scale, 0.001f, 10.0f, "%.3f", 0);
		igSliderFloat("lacunarity", &lacunarity, 0.001f, 10.0f, "%.3f", 0);
		igSliderInt("octaves", &octaves, 1, 6, "%d", 0);
		igSliderFloat("falloff", &falloff, 0.001f, 1.0f, "%.3f", 0);
		igSliderFloat("frequency", &frequency, 0.1f, 10.0f, "%.3f", 0);
		igSliderFloat("time_amplitude", &time_amplitude, 0.01f, 2.0f, "%.3f", 0);
		igEnd();

		srand(0);
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				noise.transform.p.y = (float)(i * h) - (h*2*0.5f);
				noise.transform.p.x = (float)(j * w) - (w*2*0.5f);
				cf_draw_sprite(&noise);
			}
		}

		cf_app_draw_onto_screen(true);

		static float time = 0;
		time += CF_DELTA_TIME / frequency;
		cf_free(pix);
		//pix = cf_noise_pixels_wrapped(w, h, 0, scale, time, time_amplitude);
		pix = cf_noise_fbm_pixels_wrapped(w, h, 0, scale, lacunarity, octaves, falloff, time, time_amplitude);
		cf_easy_sprite_update_pixels(&noise, pix);
	}

	cf_destroy_app();

	return 0;
}
