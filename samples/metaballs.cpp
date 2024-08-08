#include <cute.h>
using namespace Cute;

#include "metaballs_data/metaballs_shader.h"

int w = 480*2;
int h = 270*2;
float scale = 1;

int main(int argc, char* argv[])
{
	make_app("Metaballs", 0, 0, 0, (int)(w*scale), (int)(h*scale), APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);
	CF_Canvas soft_circles = make_canvas(canvas_defaults(w, h));
	CF_Shader shd = CF_MAKE_SOKOL_SHADER(metaballs_shader);
	clear_color(0,0,0,1);
	float t = 0;

	while (app_is_running())
	{
		app_update();
		draw_scale(scale, scale);
		t = t + DELTA_TIME;

		// Draw soft-circles.
		Rnd rnd = rnd_seed(0);
		for (int i = 0; i < 100; ++i)
		{
			float o = rnd_range(rnd, -10.0f,10.0f);
			float x = rnd_range(rnd, -w/scale*0.5f, w/scale*0.5f) + cosf(t+o) * 10;
			float y = rnd_range(rnd, -h/scale*0.5f, h/scale*0.5f) + sinf(t+o) * 10;
			float r = rnd_range(rnd, 10.0f,60.0f);

			// Render a perfect soft-circle using anti-alias scaling and zero-radius.
			draw_push_antialias_scale(r);
			draw_circle_fill(make_circle(V2(x, y), 0));
			draw_pop_antialias_scale();
		}

		static int toggle = false;
		if (key_just_pressed(KEY_SPACE)) {
			toggle = !toggle;
		}
		if (!toggle) {
			// Render soft circles onto their own canvas, to feed into the metaballs filter shader.
			render_to(soft_circles, true);

			// Apply the metaball filter to the soft circle render.
			render_settings_push_shader(shd);
			render_settings_push_texture("tex", canvas_get_target(soft_circles));
			draw_box_fill(make_aabb(V2(-w/scale, -h/scale), V2(w/scale, h/scale)));
			render_to(app_get_canvas());
			render_settings_pop_shader();
		}

		draw_text("press space", -V2(text_width("press_soace") * 0.5f, 0));

		app_draw_onto_screen(toggle);
	}

	destroy_canvas(soft_circles);
	destroy_app();
	return 0;
}
