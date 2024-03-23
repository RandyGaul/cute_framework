#include <cute.h>
#include <imgui/imgui.h>

#include "shallow_water_data/shallow_water_shader.h"

using namespace Cute;

void mount_content_directory_as(const char* dir)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += "/shallow_water_data";
	fs_mount(path.c_str(), dir);
}

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	CF_Result result = make_app("Shallow Water Sample", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	mount_content_directory_as("/");
	app_init_imgui();

	CF_Shader shader = CF_MAKE_SOKOL_SHADER(shallow_water_shader);
	CF_Canvas offscreen = make_canvas(canvas_defaults(160, 120));

	struct Wavelet
	{
		v2 p = V2(0,0);
		float r = 1.0f;
		float blur = 3.0f;
		float opacity = 1.0f;
	};

	Array<Wavelet> wavelets;

	auto add_wavelet = [&](int x, int y) {
		Wavelet w;
		w.p = V2(x*0.25f - 80, (-y+480)*0.25f - 60);
		wavelets.add(w);
	};

	while (app_is_running()) {
		app_update();
		camera_dimensions(160, 120);

		if (mouse_just_pressed(MOUSE_BUTTON_LEFT)) {
			add_wavelet(mouse_x(), mouse_y());
		}

		for (int i = 0; i < wavelets.size(); ++i) {
			Wavelet& w = wavelets[i];
			draw_push_antialias_scale(w.blur);
			draw_push_color(make_color(1.0f, 1.0f, 1.0f, w.opacity));
			draw_circle(w.p, w.r, 0);
			w.r += 10.0f * CF_DELTA_TIME;
			w.opacity -= 0.5f * CF_DELTA_TIME;
			w.blur += 20.0f * CF_DELTA_TIME;
			if (w.opacity < 0) {
				wavelets.unordered_remove(i++);
			}
		}

		app_draw_onto_screen();
	}

	destroy_shader(shader);
	destroy_app();

	return 0;
}
