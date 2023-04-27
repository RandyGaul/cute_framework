#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, w, h, options, argv[0]);
	if (is_error(result)) return -1;

	draw_push_antialias(true);

	printf("Press space to toggle on/off app canvas auto-resizing.\n");
	printf("Auto resizing is now enabled.\n");
	bool auto_resize = true;

	while (app_is_running()) {
		app_update();

		if (key_just_pressed(KEY_SPACE)) {
			auto_resize = !auto_resize;
			printf("Canvas auto-resizing is now %s\n", auto_resize ? "enabled" : "disabled");
		}

		if (auto_resize && app_was_resized()) {
			app_get_size(&w, &h);
			w = CF_ALIGN_TRUNCATE(w, 2);
			h = CF_ALIGN_TRUNCATE(h, 2);
			app_set_size(w, h);
			app_set_canvas_size(w, h);
			camera_dimensions((float)w, (float)h);
			printf("%d, %d\n", w, h);
		}

		draw_circle(V2(0,0), 100.0f, 100, 10.0f);

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
