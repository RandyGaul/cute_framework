#include <cute.h>
using namespace Cute;

int main(int argc, const char** argv)
{
	// Create a window with a resolution of 640 x 480.
	int options = APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_DEFAULT_GFX_CONTEXT;
	Result result = make_app("Fancy Window Title", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	while (app_is_running())
	{
		float dt = calc_dt();
		app_update(dt);
		// All your game logic and updates go here...
		app_present();
	}

	destroy_app();

	return 0;
}
