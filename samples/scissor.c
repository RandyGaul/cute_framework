#include <cute.h>

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	cf_make_app("Scissor", 0, 0, 0, w, h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_draw_push_scissor((CF_Rect){
			.x = 0,
			.y = 0,
			.w = 320,
			.h = 240
		});

		cf_draw_circle((CF_Circle){ .p = { 0, 0 }, .r = 100.0f}, 10.0f);

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
