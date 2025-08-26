#include <cute.h>
#include <cmath>
#include <cstdint>
using namespace Cute;

float w = 300*2;
float h = 300*2;

// This shader is just a bunch of ChatGPT code.
// Would be nice to replace it with some more cool.
#define STR(X) #X
const char* s_shd = STR(
	layout(set = 2, binding = 0) uniform sampler2D tex;

	layout(set = 3, binding = 1) uniform shd_uniforms {
		float dt;
	};

	vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
	{
		// --- Read current state ---
		vec4 C = texture(tex, screen_uv);
		ivec2 ts = textureSize(tex, 0);
		vec2 texel = 1.0 / vec2(ts);

		// Neighbor samples for diffusion/gradients.
		vec2 uvL = clamp(screen_uv + vec2(-texel.x, 0.0), 0.0, 1.0);
		vec2 uvR = clamp(screen_uv + vec2( texel.x, 0.0), 0.0, 1.0);
		vec2 uvD = clamp(screen_uv + vec2(0.0, -texel.y), 0.0, 1.0);
		vec2 uvU = clamp(screen_uv + vec2(0.0,  texel.y), 0.0, 1.0);

		vec4 L = texture(tex, uvL);
		vec4 R = texture(tex, uvR);
		vec4 D = texture(tex, uvD);
		vec4 U = texture(tex, uvU);

		// Unpack velocity from RG in [-1,1].
		vec2 v = C.rg * 2.0 - 1.0;

		// --- Advection ---
		// Backtrace in UV space. Scale down for stability.
		float adv_scale = 0.6;
		vec2 backUV = clamp(screen_uv - v * dt * adv_scale, 0.0, 1.0);
		vec4 adv = texture(tex, backUV);
		vec2 v_adv = adv.rg * 2.0 - 1.0;
		float dye_adv = adv.b;

		// --- Simple diffusion ---
		vec2 v_lap = (L.rg + R.rg + U.rg + D.rg - 4.0 * C.rg);
		float d_lap = (L.b  + R.b  + U.b  + D.b  - 4.0 * C.b);
		float visc = 0.15;          // velocity diffusion
		float diff = 0.12;          // dye diffusion

		// --- Add a bit of vorticity from dye gradient for a "cool" look ---
		vec2 dye_grad = vec2(R.b - L.b, U.b - D.b) * 0.5;
		vec2 vort = vec2(-dye_grad.y, dye_grad.x);  // perpendicular to gradient
		float vort_strength = 1.2;
		v += vort * vort_strength * dt;

		// --- Damping / viscosity ---
		v += v_lap * visc * dt;
		v = mix(v, v_adv, 0.65);           // semi-Lagrangian advection of velocity
		v *= (1.0 - 0.25 * dt);            // simple drag

		float dye = dye_adv + d_lap * diff * dt;
		dye = clamp(dye, 0.0, 1.0);

		// Pack velocity back to RG in [0,1], dye in B, A=1.
		vec2 v01 = v * 0.5 + 0.5;
		return vec4(clamp(v01, 0.0, 1.0), dye, 1.0);
	}
);
int main(int argc, char* argv[])
{
	make_app("Fluid Sim", 0, 0, 0, (int)w, (int)h, CF_APP_OPTIONS_RESIZABLE_BIT | CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	CF_Shader shd = cf_make_draw_shader_from_source(s_shd);
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
