#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED | CF_APP_OPTIONS_RESIZABLE;
	CF_Result result = cf_make_app("Window Events", 0, 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Listen to all window events and trace them.
		if (cf_app_gained_focus()) {
			printf("App gained focus.\n");
		}
		if (cf_app_lost_focus()) {
			printf("App has lost focus.\n");
		}
		if (cf_app_was_maximized()) {
			printf("App maximized.\n");
		}
		if (cf_app_was_minimized()) {
			printf("App minimized.\n");
		}
		if (cf_app_mouse_entered()) {
			printf("Mouse coordinates entered the window area.\n");
		}
		if (cf_app_mouse_exited()) {
			printf("Mouse coordinates exited the window area.\n");
		}
		if (cf_app_was_moved()) {
			printf("App was moved.\n");
		}
		if (cf_app_was_resized()) {
			printf("App was resized.\n");
		}
		if (cf_app_was_restored()) {
			printf("App was restored.\n");
		}

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
