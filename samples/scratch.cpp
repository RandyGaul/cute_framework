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

		layout(binding = 0) uniform fs_params {
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

		layout(binding = 0) uniform fs_params {
			vec4 u_params;
		};

		void main()
		{
			vec4 color = v_color * u_params;
			result = color;
		})";

	CF_Shader shdv = cf_make_shader(CF_SHADER_FORMAT_SPIRV, vert, CF_SHADER_STAGE_VERTEX);
	CF_Shader shdf = cf_make_shader(CF_SHADER_FORMAT_SPIRV, frag, CF_SHADER_STAGE_FRAGMENT);

	while (app_is_running()) {
		app_update();

		cf_clear_screen(1,0,0,1);

		app_draw_onto_screen();
	}

	destroy_app();

	return 0;
}
