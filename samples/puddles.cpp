#include <cute.h>
#include <imgui/imgui.h>

#include "puddles_data/puddles_shader.h"

using namespace Cute;

struct Offscreen
{
	int w, h;
	CF_Texture target;
	CF_Texture depth_stencil;
	CF_Canvas canvas;
};

void mount_content_directory_as(const char* dir)
{
	Path path = fs_get_base_directory();
	path.normalize();
	path += "/puddles_data";
	fs_mount(path.c_str(), dir);
}

int main(int argc, char* argv[])
{
	int options = APP_OPTIONS_DEFAULT_GFX_CONTEXT | APP_OPTIONS_WINDOW_POS_CENTERED | APP_OPTIONS_RESIZABLE;
	CF_Result result = make_app("Puddles Sample", 0, 0, 640, 480, options, argv[0]);
	if (is_error(result)) return -1;

	mount_content_directory_as("/");
	app_init_imgui();

	CF_Png png;
	cf_png_cache_load("/noise.png", &png);
	CF_ASSERT(png.w == 128);
	CF_ASSERT(png.h == 128);

	CF_Shader shader = CF_MAKE_SOKOL_SHADER(puddles_shader);

	CF_TextureParams tex_params = texture_defaults(128, 128);
	tex_params.initial_data = png.pix;
	tex_params.initial_data_size = sizeof(CF_Pixel) * png.w * png.h;
	CF_Texture tex = make_texture(tex_params);

	CF_Sprite water1 = make_sprite("/water1.ase");
	CF_Sprite water2 = make_sprite("/water2.ase");
	CF_Sprite water3 = make_sprite("/water3.ase");
	water1.scale *= 2;
	water2.scale *= 2;
	water3.scale *= 2;

	CF_Canvas offscreen = make_canvas(canvas_defaults(640, 480));

	while (app_is_running())
	{
		app_update();

		ImGui::SetNextWindowSize(V2(250,100));
		ImGui::Begin("Puddles Sample");
		static float amplitude = 5.0f;
		ImGui::SliderFloat("amplitude", &amplitude, 0, 20);
		static float speed = 1.0f;
		ImGui::SliderFloat("speed", &speed, 0, 5, "%.3f");
		static bool show_noise = false;
		ImGui::Checkbox("show noise", &show_noise);
		ImGui::End();

		if (!show_noise) {
			srand(0);
			for (int i = 0; i <= 480/32; ++i) {
				for (int j = 0; j <= 640/32; ++j) {
					CF_Sprite water;
					switch (rand() % 3) {
					case 0: water = water1; break;
					case 1: water = water2; break;
					case 2: water = water3; break;
					}
					water.transform.p.y = (float)(i * 32) - 240.0f;
					water.transform.p.x = (float)(j * 32) - 320.0f;
					draw_sprite(water);
				}
			}
			render_to(offscreen);
		}

		static float time;
		time += CF_DELTA_TIME * 0.125f * speed;
		render_settings_push_shader(shader);
		render_settings_push_uniform("amplitude", amplitude);
		render_settings_push_uniform("time", time);
		render_settings_push_uniform("show_noise", show_noise ? 1.0f : 0.0f);
		render_settings_push_texture("water_tex", canvas_get_target(offscreen));
		render_settings_push_texture("noise_tex", tex);
		draw_push_antialias(false);
		draw_box_fill(make_aabb(V2(0,0), 640, 480));

		app_draw_onto_screen();
	}

	destroy_shader(shader);
	destroy_app();

	return 0;
}
