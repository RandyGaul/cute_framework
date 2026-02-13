#include <cute.h>
#include <cmath>
#include <cstdint>
using namespace Cute;

float w = 300*2;
float h = 300*2;

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "fluid_sim_data/fluid_sim_shd.h"
#endif

void mount_content_directory_as(const char* dir)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += "/fluid_sim_data";
	fs_mount(path.c_str(), dir);
}

int main(int argc, char* argv[])
{
	make_app("Fluid Sim", 0, 0, 0, (int)w, (int)h, CF_APP_OPTIONS_RESIZABLE_BIT | CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	mount_content_directory_as("/");
	cf_shader_directory("/");
#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_Shader shd = cf_make_draw_shader("fluid_sim.shd");
#else
	CF_Shader shd = cf_make_draw_shader_from_bytecode(s_fluid_sim_shd_bytecode);
#endif
	CF_CanvasParams canvas_params = canvas_defaults((int)w, (int)h);
	canvas_params.target.stream = true;
	CF_Canvas canvas_a = make_canvas(canvas_params);
	CF_Canvas canvas_b = make_canvas(canvas_params);

	set_target_framerate(200);

	CF_Pixel* initial_data = (CF_Pixel*)malloc(sizeof(CF_Pixel) * (int)w * (int)h);

	// This initial data is also just a bunch of ChatGPT code.
	// RG = velocity (packed [-1,1] -> [0,255]), B = dye density, A = 255.
	uint8_t* bytes = (uint8_t*)initial_data;
	int W = (int)w, H = (int)h;

	auto clamp01 = [](float x){ return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
	auto hash01  = [](int x, int y){
		// small deterministic noise for speckled starts
		uint32_t n = (uint32_t)x * 1973u ^ (uint32_t)y * 9277u + 0x9e3779b9u;
		n ^= n >> 15; n *= 0x85ebca6bu; n ^= n >> 13; n *= 0xc2b2ae35u; n ^= n >> 16;
		return (n & 0xFFFFFFu) / 16777215.0f;
	};

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			float u = (x + 0.5f) / w;
			float v = (y + 0.5f) / h;

			// Base big swirl around center
			float cx = 0.5f, cy = 0.5f;
			float dx = u - cx, dy = v - cy;
			float r2 = dx*dx + dy*dy;
			float fall = std::exp(-r2 * 10.0f);
			float vx = -dy * fall * 0.6f;
			float vy =  dx * fall * 0.6f;

			// Add four local swirls (match shader emitters)
			auto gauss = [](float qx, float qy, float s){ return std::exp(-(qx*qx + qy*qy) * s); };
			float w1 = gauss(u - 0.25f, v - 0.50f, 120.0f);
			float w2 = gauss(u - 0.75f, v - 0.50f, 120.0f);
			float w3 = gauss(u - 0.50f, v - 0.25f, 110.0f);
			float w4 = gauss(u - 0.50f, v - 0.75f, 110.0f);

			vx +=  1.0f * (-(v - 0.50f)) * w1;
			vy +=  1.0f * (  (u - 0.25f)) * w1;
			vx += -0.9f * (-(v - 0.50f)) * w2;
			vy += -0.9f * (  (u - 0.75f)) * w2;
			vx +=  0.8f * (-(v - 0.25f)) * w3;
			vy +=  0.8f * (  (u - 0.50f)) * w3;
			vx += -0.7f * (-(v - 0.75f)) * w4;
			vy += -0.7f * (  (u - 0.50f)) * w4;

			// Gentle horizontal shear to diversify R/G
			vx += 0.08f * std::sin(6.283185f * v);
			vy += 0.06f * std::sin(6.283185f * u);

			// Pack velocity to [0,1]
			float vx01 = clamp01(0.5f * vx + 0.5f);
			float vy01 = clamp01(0.5f * vy + 0.5f);

			// Dye: center ring + four blobs + a little noise
			float ring = std::exp(-std::abs(r2 - 0.12f) * 60.0f);
			float dye  = ring + 0.9f*w1 + 0.9f*w2 + 0.8f*w3 + 0.8f*w4;
			dye += 0.08f * hash01(x, y);
			dye = clamp01(dye);

			size_t idx = (size_t)(y * W + x) * 4;
			bytes[idx + 0] = (uint8_t)(vx01 * 255.0f + 0.5f);  // R = vel.x
			bytes[idx + 1] = (uint8_t)(vy01 * 255.0f + 0.5f);  // G = vel.y
			bytes[idx + 2] = (uint8_t)(dye  * 255.0f + 0.5f);  // B = dye
			bytes[idx + 3] = 255;                              // A
		}
	}

	texture_update(canvas_get_target(canvas_a), initial_data, sizeof(CF_Pixel) * (int)w * (int)h);

	while (app_is_running()) {
		app_update();

		draw_push_alpha_discard(false);

		draw_push_shader(shd);
		draw_set_uniform("dt", CF_DELTA_TIME);
		draw_set_texture("tex", canvas_get_target(canvas_a));
		draw_box(V2(0,0), w, h);
		render_to(canvas_b, true);
		draw_pop_shader();

		draw_canvas(canvas_b, V2(0,0), V2(w,h));
		app_draw_onto_screen(true);

		CF_Canvas tmp = canvas_a;
		canvas_a = canvas_b;
		canvas_b = tmp;
	}

	free(initial_data);
	destroy_shader(shd);
	destroy_canvas(canvas_a);
	destroy_canvas(canvas_b);
	destroy_app();
	return 0;
}
