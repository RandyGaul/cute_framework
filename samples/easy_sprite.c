#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED | CF_APP_OPTIONS_RESIZABLE;
	CF_Result result = cf_make_app("Window Events", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	int w = 200;
	int h = 200;
	CF_Pixel* pixels = (CF_Pixel*)cf_alloc(sizeof(CF_Pixel) * w * h);
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(pixels, w, h);
	float t = 0;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		t += CF_DELTA_TIME;
		float s = sinf(t) * 0.5f + 0.5f;
		CF_Pixel c0 = cf_pixel_red();
		CF_Pixel c1 = cf_pixel_blue();
		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j) {
				CF_Pixel p = cf_pixel_lerp(c0, c1, s);
				pixels[i * w + j] = p;
			}
		}

		// This does have a much higher performance cost than a typical sprite.
		// You've been warned! :)
		cf_easy_sprite_update_pixels(&sprite, pixels);

		cf_sprite_draw(&sprite);

		cf_app_draw_onto_screen();
	}

	cf_free(pixels);
	cf_destroy_app();

	return 0;
}
