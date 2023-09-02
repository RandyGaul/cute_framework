#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED;
	CF_Result result = cf_make_app("Basic Input", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Test for keyboard input.
		if (cf_key_down(CF_KEY_Z)) {
			printf("Key Z is currently DOWN\n");
		}

		// Test for mouse input.
		if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT)) {
			printf("Mouse left is currently DOWN\n");
		}

		// Showcase the "just pressed" API for keyboard/mouse.
		if (cf_key_just_pressed(CF_KEY_X)) {
			printf("Key X was just pressed\n");
		}
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT)) {
			printf("Mouse right is currently DOWN\n");
		}

		// Repeating keys.
		if (cf_key_repeating(CF_KEY_C)) {
			printf("Key C was repeated\n");
		}

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
