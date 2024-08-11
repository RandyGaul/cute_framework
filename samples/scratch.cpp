#include <cute.h>
using namespace Cute;

// This isn't really a sample, but a scratch pad for the CF author to experiment.

int main(int argc, char* argv[])
{
	int w = 640;
	int h = 480;
	make_app("Development Scratch", 0, 0, 0, w, h, APP_OPTIONS_WINDOW_POS_CENTERED, argv[0]);

	const char* vert = R"(
		#version 450
		layout (location = 0) in vec2 in_pos;
		layout (location = 0) out vec4 v_color;

		layout(binding = 0) uniform uniform_block {
			vec4 u_color;
		};

		void main()
		{
			v_color = u_color;
			gl_Position = vec4(in_pos, 0, 1);
		})";

	const char* frag = R"(
		#version 450
		layout (location = 0) in vec4 v_color;
		layout (location = 0) out vec4 result;

		layout(binding = 0) uniform uniform_block {
			vec4 u_params;
		};

		void main()
		{
			vec4 color = v_color * u_params;
			result = color;
		})";

	CF_Shader shd = cf_make_shader(CF_SHADER_FORMAT_SPIRV, vert, frag);
	CF_Material mat = cf_make_material();
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));

	CF_Color color = cf_color_blue();
	CF_Color params = { 0.5f, 0.5f, 0.5f, 0.5f };
	cf_material_set_uniform_vs(mat, "u_color", &color, CF_UNIFORM_TYPE_FLOAT4, 0);
	cf_material_set_uniform_fs(mat, "u_params", &params, CF_UNIFORM_TYPE_FLOAT4, 0);

	while (app_is_running()) {
		app_update();

		cf_clear_screen(1,0,0,1);

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
