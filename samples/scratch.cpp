#include <cute.h>
using namespace Cute;

#include <imgui.h>

int main(int argc, const char** argv)
{
	int w = 640/1;
	int h = 480/1;
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	Result result = make_app("Development Scratch", 0, 0, w, h, options, argv[0]);
	if (is_error(result)) return -1;

	app_init_imgui();

	camera_dimensions((float)w, (float)h);
	int draw_calls = 0;

	while (app_is_running()) {
		app_update();

		static float t = 0;
		t += CF_DELTA_TIME;

		//draw_push_antialias(true);
		//draw_circle_fill(V2(sinf(t*0.25f) * 250.0f,0), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
		//draw_circle(V2(sinf(t+2.0f) * 50.0f,0), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
		//draw_circle(V2(0,50), 20.0f);
		//draw_line(V2(0,0), V2(cosf(t*0.5f+CUTE_PI),sinf(t*0.5f+CUTE_PI))*100.0f,10);
		//draw_pop_antialias();
		//
		//draw_circle_fill(V2(sinf(t*0.25f) * 250.0f,100), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
		//draw_circle(V2(sinf(t+2.0f) * 50.0f,100), (cosf(t) * 0.5f + 0.5f) * 50.0f + 50.0f);
		//draw_circle(V2(0,0), 20.0f);
		//draw_line(V2(0,0), V2(cosf(t*0.5f),sinf(t*0.5f))*100.0f,10);

		draw_quad(cf_make_aabb(V2(-20,-20), V2(20,20)));

		draw_calls = app_present();
	}

	destroy_app();

	return 0;
}
