#include <cute.h>
using namespace Cute;

const char* s_glitch = R"(
	vec4 shader(vec4 color, ShaderParams params)
	{
		float time = params.attributes.x;
		float band_count = params.attributes.y;
		float intensity = params.attributes.z;

		// Need valid UV bounds for the clamp-to-edge feature.
		vec2 uv_range = params.uv_max - params.uv_min;

		// Normalize UV to 0-1 within the sprite's atlas region.
		vec2 uv = v_uv;
		vec2 local_uv = (uv - params.uv_min) / uv_range;

		// Slice into horizontal bands based on local position.
		float band = floor(local_uv.y * band_count);
		float hash = fract(sin(band * 43.758 + floor(time * 12.0)) * 4758.5453);

		// Only glitch ~40% of bands; offset horizontally scaled to sprite width.
		if (hash > 0.6) {
			uv.x += (hash - 0.6) * 2.5 * intensity * uv_range.x;
		}

		// Clamp to sprite atlas bounds -- prevents bleeding from neighboring atlas entries.
		uv = clamp(uv, params.uv_min, params.uv_max);

		// Re-sample the texture at the glitched+clamped UV.
		return texture(u_image, smooth_uv(uv, u_texture_size));
	}
)";

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Glitch", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;
	CF_Shader shd = make_draw_shader_from_source(s_glitch);

	CF_Sprite girl = cf_make_demo_sprite();
	sprite_play(girl, "spin");
	girl.scale = V2(4, 4);

	float time = 0;

	while (app_is_running()) {
		app_update();
		time += CF_DELTA_TIME;

		draw_push_shader(shd);
		draw_push_vertex_attributes(time, 20.0f, 0.3f, 0);
		sprite_update(girl);
		sprite_draw(girl);

		app_draw_onto_screen(true);
	}

	destroy_app();
	return 0;
}
