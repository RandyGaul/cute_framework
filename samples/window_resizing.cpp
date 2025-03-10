#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	make_app("Window Resizing", 0, 0, 0, w, h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);

	draw_push_antialias(true);

	printf("Press space to toggle on/off app canvas auto-resizing.\n");
	printf("Auto resizing is now enabled.\n");
	bool auto_resize = true;

	while (app_is_running()) {
		app_update();

		if (key_just_pressed(CF_KEY_SPACE)) {
			auto_resize = !auto_resize;
			printf("Canvas auto-resizing is now %s\n", auto_resize ? "enabled" : "disabled");
		}

		if (auto_resize && app_was_resized()) {
			app_get_size(&w, &h);
			w = CF_ALIGN_TRUNCATE(w, 2);
			h = CF_ALIGN_TRUNCATE(h, 2);
			app_set_size(w, h);
			app_set_canvas_size(w, h);
			draw_scale((float)w, (float)h);
			printf("%d, %d\n", w, h);
		}

		draw_circle(V2(0,0), 100.0f, 10.0f);

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
