#include <cute.h>
#include <imgui/imgui.h>
#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "shallow_water_data/shader.h"
#endif

using namespace Cute;

void mount_content_directory_as(const char* dir)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += "/shallow_water_data";
	fs_mount(path.c_str(), dir);
}

CF_Pixel* get_noise(int w, int h, float time)
{
	float scale = 2.5f;
	float lacunarity = 2.0f;
	int octaves = 3;
	float falloff = 0.5f;
	float frequency = 0.5f;
	float time_amplitude = 0.05f;
	return cf_noise_fbm_pixels_wrapped(w, h, 0, scale, lacunarity, octaves, falloff, time * frequency, time_amplitude);
}

int main(int argc, char* argv[])
{
	make_app("Shallow Water Sample", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	cf_shader_directory("/shallow_water_data");
	mount_content_directory_as("/");
	app_init_imgui();

	int W = 160;
	int H = 120;

#if CF_RUNTIME_SHADER_COMPILATION
	char* shader_source = cf_fs_read_entire_file_to_memory_and_nul_terminate("/shallow_water_data/shader.shd", NULL);
	CF_Shader shader = cf_make_draw_shader_from_source(shader_source);
	cf_free(shader_source);
#else
	CF_Shader shader = cf_make_draw_shader_from_bytecode(s_shd_bytecode);
#endif
	CF_Canvas offscreen = make_canvas(canvas_defaults(160, 120));
	CF_Canvas scene_canvas = make_canvas(canvas_defaults(160, 120));

	struct Wavelet
	{
		v2 p = V2(0,0);
		float r = 1.0f;
		float blur = 3.0f;
		float opacity = 1.0f;
	};

	Array<Wavelet> wavelets;

	struct Spawner
	{
		v2 p = V2(0,0);
		int count = 3;
		float opacity = 1.0f;
		CF_Routine rt = { };
	};

	Array<Spawner> spawners;

	auto add_wavelet = [&](v2 p, float opacity) {
		Wavelet& w = wavelets.add();
		w.p = p;
		w.opacity = opacity;
	};

	auto add_spawner = [&](int x, int y) {
		Spawner& s = spawners.add();
		s.p = V2(x*0.25f - 80, (-y+480)*0.25f - 60);
	};

	float time = 0;
	CF_TextureParams tex_params = texture_defaults(W, H);
	CF_Texture noise_tex = make_texture(tex_params);
	CF_Sprite scene = make_sprite("/scene.ase");

	while (app_is_running()) {
		time += CF_DELTA_TIME;
		CF_Pixel* noise = get_noise(W, H, time);
		texture_update(noise_tex, noise, sizeof(CF_Pixel) * W * H);
		cf_free(noise);

		app_update();
		draw_scale(4,4);

		ImGui::Begin("Shallow Water Sample");
		static bool show_noise;
		ImGui::Checkbox("Show noise", &show_noise);
		static bool show_normals;
		ImGui::Checkbox("Show normals", &show_normals);
		ImGui::End();

		if (mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			add_spawner((int)mouse_x(), (int)mouse_y());
		}

		for (int i = 0; i < spawners.size(); ++i) {
			Spawner& s = spawners[i];
			s.opacity = max(0.0f, s.opacity - 0.6f * CF_DELTA_TIME);

			rt_begin(s.rt, CF_DELTA_TIME)
			{
				add_wavelet(s.p, s.opacity);
				s.count--;
				if (!s.count) {
					spawners.unordered_remove(i--);
				}
			}
			rt_wait(0.75f)
			{
				nav_restart();
			}
			rt_end();
		}

		for (int i = 0; i < wavelets.count(); ++i) {
			Wavelet& w = wavelets[i];
			draw_push_antialias_scale(w.blur);
			draw_push_color(make_color(1.0f, 1.0f, 1.0f, w.opacity));
			draw_circle(w.p, w.r, 0);
			w.r += 10.0f * CF_DELTA_TIME;
			w.opacity -= 0.4f * CF_DELTA_TIME;
			w.blur += 20.0f * CF_DELTA_TIME;
			if (w.opacity < 0) {
				wavelets.unordered_remove(i--);
			}
		}

		render_to(offscreen, true);

		draw_sprite(scene);
		render_to(scene_canvas, true);

		draw_push_shader(shader);
		draw_set_texture("wavelets_tex", canvas_get_target(offscreen));
		draw_set_texture("noise_tex", noise_tex);
		draw_set_texture("scene_tex", canvas_get_target(scene_canvas));
		draw_set_uniform("show_noise", show_noise ? 1.0f : 0.0f);
		draw_set_uniform("show_normals", show_normals ? 1.0f : 0.0f);
		draw_push_antialias(false);
		draw_box(V2(0,0), (float)W, (float)H);
		app_draw_onto_screen(false);
	}

	destroy_shader(shader);
	destroy_app();

	return 0;
}
