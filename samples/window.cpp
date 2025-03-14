#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	// Create a window with a resolution of 640 x 480.
	CF_Result result = make_app("Fancy Window Title", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;

	while (app_is_running()) {
		app_update();
		// All your game logic and updates go here...
		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
