#include <cute.h>
using namespace Cute;

int w = 480*2;
int h = 270*2;
float scale = 1;

#define STR(X) #X
const char* s_shd = STR(
	layout(set = 2, binding = 1) uniform sampler2D tex;

	vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
	{
		float d = texture(tex, screen_uv).x;
		d = d > 0.5 ? 1.0 : 0.0;
		return vec4(vec3(d), 1);
	}
);

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "metaballs_shd.h"
#endif

CF_Canvas soft_circles;
CF_Shader shd;
float frame_times[10];
float frame_time;
int frame_index;
float fps;

void update()
{
	while (app_is_running()) {
		app_update();

		frame_times[frame_index] = CF_DELTA_TIME;
		frame_index = (frame_index + 1) % 10;
		float sum = 0;
		for (int i = 0; i < 10; ++i) {
			sum += frame_times[i];
		}
		sum /= 10.0f;
		fps = (1.0f / sum);
		if (frame_index % 10 == 0) printf("%f\n", fps);

		draw_scale(scale, scale);

		// Draw soft-circles.
		CF_Rnd rnd = rnd_seed(0);
		for (int i = 0; i < 200; ++i) {
			float o = rnd_range(rnd, -10.0f,10.0f);
			float x = rnd_range(rnd, -w/scale*0.5f, w/scale*0.5f) + cosf((float)CF_SECONDS+o) * 10;
			float y = rnd_range(rnd, -h/scale*0.5f, h/scale*0.5f) + sinf((float)CF_SECONDS+o) * 10;
			float r = rnd_range(rnd, 10.0f,60.0f);
		
			// Render a perfect soft-circle using anti-alias scaling and zero-radius.
			draw_push_antialias_scale(r);
			draw_circle_fill(make_circle(V2(x, y), 0));
			draw_pop_antialias_scale();
		}

		static int toggle = false;
		if (key_just_pressed(CF_KEY_SPACE)) {
			toggle = !toggle;
		}
		if (!toggle) {
			// Render soft circles onto their own canvas, to feed into the metaballs filter shader.
			render_to(soft_circles, true);
		
			// Apply the metaball filter to the soft circle render.
			draw_push_shader(shd);
			draw_set_texture("tex", canvas_get_target(soft_circles));
			draw_box_fill(make_aabb(V2(-w/scale, -h/scale), V2(w/scale, h/scale)));
			render_to(app_get_canvas());
			draw_pop_shader();
		}

		draw_text("press space", -V2(text_width("press_space") * 0.5f, 0));

		app_draw_onto_screen(toggle ? true : false);
	}
}

int main(int argc, char* argv[])
{
	make_app("Metaballs", 0, 0, 0, (int)(w*scale), (int)(h*scale), CF_APP_OPTIONS_RESIZABLE_BIT | CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	soft_circles = make_canvas(canvas_defaults(w, h));

	
#ifdef CF_RUNTIME_SHADER_COMPILATION
	shd = cf_make_draw_shader_from_source(s_shd);
#else
	shd = cf_make_draw_shader_from_bytecode(metaballs);
#endif

	frame_index = 0;
	fps = 0;

	set_target_framerate(200);

#ifdef CF_EMSCRIPTEN
	emscripten_set_main_loop(update, 60, true);
#else
	while (app_is_running()) {
		update();
	}
#endif

	destroy_canvas(soft_circles);
	destroy_app();
	return 0;
}
