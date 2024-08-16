#include <cute.h>
using namespace Cute;

// This isn't really a sample, but a scratch pad for the CF author to experiment.

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	make_app("Development Scratch", 0, 0, 0, w, h, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	while (app_is_running()) {
		app_update();

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
