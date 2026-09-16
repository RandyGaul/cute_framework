// One draw with 9 textures: u_image plus 8 user samplers.
// Pass: solid green window.
// Fail: black window, and the browser console says
//   GL_INVALID_OPERATION: glDrawArraysInstanced: Two textures of different types use the same sampler location.
#include <cute.h>
using namespace Cute;

const char* s_shd = R"(
	layout(set = 2, binding = 1) uniform sampler2D u_tex1;
	layout(set = 2, binding = 2) uniform sampler2D u_tex2;
	layout(set = 2, binding = 3) uniform sampler2D u_tex3;
	layout(set = 2, binding = 4) uniform sampler2D u_tex4;
	layout(set = 2, binding = 5) uniform sampler2D u_tex5;
	layout(set = 2, binding = 6) uniform sampler2D u_tex6;
	layout(set = 2, binding = 7) uniform sampler2D u_tex7;
	layout(set = 2, binding = 8) uniform sampler2D u_tex8;

	vec4 shader(vec4 color, ShaderParams params)
	{
		vec2 uv = params.screen_uv;
		return (texture(u_tex1, uv) + texture(u_tex2, uv) + texture(u_tex3, uv) + texture(u_tex4, uv)
		      + texture(u_tex5, uv) + texture(u_tex6, uv) + texture(u_tex7, uv) + texture(u_tex8, uv)) / 8.0;
	}
)";

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Nine Textures", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) return -1;

	CF_Shader shader = make_draw_shader_from_source(s_shd);
	CF_Texture green = make_texture(texture_defaults(1, 1));
	CF_Pixel pixel = pixel_green();
	texture_update(green, &pixel, sizeof(pixel));

	while (app_is_running()) {
		app_update();

		draw_push_shader(shader);
		draw_set_texture("u_tex1", green);
		draw_set_texture("u_tex2", green);
		draw_set_texture("u_tex3", green);
		draw_set_texture("u_tex4", green);
		draw_set_texture("u_tex5", green);
		draw_set_texture("u_tex6", green);
		draw_set_texture("u_tex7", green);
		draw_set_texture("u_tex8", green);
		draw_box_fill(V2(0, 0), 640, 480);

		app_draw_onto_screen(true);
	}

	destroy_texture(green);
	destroy_shader(shader);
	destroy_app();

	return 0;
}
