#include <cute.h>
#include <imgui/imgui.h>

using namespace Cute;

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "waves_data/waves_shd.h"
#endif

void mount_content_directory_as(const char* dir)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += "/waves_data";
	fs_mount(path.c_str(), dir);
}

int main(int argc, char* argv[])
{
	make_app("Waves Sample", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	mount_content_directory_as("/");
	cf_shader_directory("/");
	app_init_imgui();

	CF_Png png;
	cf_png_cache_load("/noise.png", &png);
	CF_ASSERT(png.w == 128);
	CF_ASSERT(png.h == 128);

#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_Shader shader = cf_make_draw_shader("waves.shd");
#else
	CF_Shader shader = cf_make_draw_shader_from_bytecode(s_waves_shd_bytecode);
#endif

	CF_TextureParams tex_params = texture_defaults(128, 128);
	CF_Texture tex = make_texture(tex_params);
	texture_update(tex, png.pix, sizeof(CF_Pixel) * png.w * png.h);

	CF_Sprite water1 = make_sprite("/water1.ase");
	CF_Sprite water2 = make_sprite("/water2.ase");
	CF_Sprite water3 = make_sprite("/water3.ase");
	water1.scale *= 2;
	water2.scale *= 2;
	water3.scale *= 2;

	CF_Canvas offscreen = make_canvas(canvas_defaults(640, 480));

	while (app_is_running()) {
		app_update();

		ImGui::SetNextWindowSize(V2(250,100));
		ImGui::Begin("Waves Sample");
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
		draw_push_shader(shader);
		draw_set_uniform("amplitude", amplitude);
		draw_set_uniform("time", time);
		draw_set_uniform("show_noise", show_noise ? 1.0f : 0.0f);
		draw_set_texture("water_tex", canvas_get_target(offscreen));
		draw_set_texture("noise_tex", tex);
		draw_push_shape_aa(0);
		draw_box_fill(make_aabb(V2(0,0), 640, 480));

		app_draw_onto_screen(true);
	}

	destroy_shader(shader);
	destroy_app();

	return 0;
}
