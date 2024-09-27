#include <cute.h>
using namespace Cute;

// This isn't really a sample, but a scratch pad for the CF author to experiment.

#define STR(X) #X
const char* s_shadow = STR(
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
	return vec4(vec3(min(color.r, 0.15)*color.a), color.a);
}
);

int main(int argc, char* argv[])
{
	float w = 640;
	float h = 480;
	make_app("Development Scratch", 0, 0, 0, (int)w, (int)h, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	CF_Shader shadow_shd = make_draw_shader_from_source(s_shadow);
	CF_Canvas plain = make_canvas(canvas_defaults((int)w, (int)h));
	CF_Canvas shadows = make_canvas(canvas_defaults((int)w, (int)h));
	cf_clear_color(0,0,0,0);

	while (app_is_running()) {
		app_update();

		draw_circle(V2(-200,0), 30, 5);
		draw_circle(V2( 200,0), 30, 5);
		render_to(plain, true);

		draw_push_shader(shadow_shd);
		draw_canvas(plain, 0, 0, w, h);
		draw_pop_shader();
		render_to(shadows, true);

		draw_canvas(shadows, 5, -5, w, h);
		draw_canvas(plain, 0, 0, w, h);
		render_to(app_get_canvas(), true);

		app_draw_onto_screen();
	}

	destroy_shader(shadow_shd);
	destroy_canvas(plain);
	destroy_canvas(shadows);
	destroy_app();

	return 0;
}
