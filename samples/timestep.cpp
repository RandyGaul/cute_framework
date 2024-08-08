#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	Result result = make_app("Tint Demo", 0, 0, 0, w, h, APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);
	if (is_error(result)) return -1;

	Sprite s = cf_make_demo_sprite();
	s.play("idle");
	float t = 0;
	float fps = 0;

	while (app_is_running()) {
		app_update();
		t += DELTA_TIME;

		if (fps == 0) {
			fps = 1.0f / CF_DELTA_TIME;
		} else {
			fps = Cute::lerp(fps, 1.0f / CF_DELTA_TIME, 1.0f / 500.0f);
		}
		static float t = 0;

		String s = fps;
		draw_text(s.c_str(), V2(-w/2.0f,-h/2.0f)+V2(2,35));
		s = CF_DELTA_TIME;
		draw_text(s.c_str(), V2(-w/2.0f,-h/2.0f)+V2(2,20));

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
