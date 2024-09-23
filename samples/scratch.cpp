#include <cute.h>
using namespace Cute;

// This isn't really a sample, but a scratch pad for the CF author to experiment.

// Visualize screen uv coordinate.
#define STR(X) #X
const char* s_screen_uv = STR(
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
	return vec4(screen_uv, 0, 1);
}
);

// Written by ChatGPT.
// A bad CRT filter for testing.
#define STR(X) #X
const char* s_crt = STR(
vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params)
{
	float scanline_intensity = params.x;
	float distortion_factor = params.y;
	float vignette_intensity = params.z;
	float brightness_boost = params.w;

	vec2 centered_uv = screen_uv * 2.0 - 1.0;
	centered_uv *= 1.0 + distortion_factor * length(centered_uv) * length(centered_uv);
	screen_uv = (centered_uv + 1.0) / 2.0;
	color = texture(u_image, screen_uv);
	float vignette = smoothstep(0.7, 1.0, length(centered_uv)) * vignette_intensity;
	color.rgb *= mix(1.0, 1.0 - vignette, vignette_intensity);
	float scanline = sin(pos.y * 800.0) * 0.5 + 0.5;
	color.rgb *= mix(1.0, scanline, scanline_intensity);
	color.rgb *= brightness_boost;

	return color;
}
);

void copy_canvas_to(CF_Canvas src, CF_Canvas dst)
{
	canvas_blit(src, V2(0,0), V2(1,1), dst, V2(0,0), V2(1,1));
}

int main(int argc, char* argv[])
{
	float w = 640;
	float h = 480;
	make_app("Development Scratch", 0, 0, 0, (int)w, (int)h, APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	CF_Shader screen_uv = make_draw_shader_from_source(s_screen_uv);
	CF_Shader crt_filter = make_draw_shader_from_source(s_crt);
	CF_Canvas canvas1 = make_canvas(canvas_defaults((int)w, (int)h));
	CF_Canvas canvas2 = make_canvas(canvas_defaults((int)w, (int)h));
	CF_Canvas canvas3 = make_canvas(canvas_defaults((int)w, (int)h));
	cf_clear_color(0,0,0,0);

	while (app_is_running()) {
		app_update();

		// Test out drawing on out-of-order layers.
		// Larger layer should appear on top.
		draw_push_layer(2);
		draw_circle(V2(-200,200), 10, 3);
		draw_circle(V2( 200,200), 10, 3);
		draw_pop_layer();

		// Default layer is 0 (underneath everything else here).
		draw_push_shader(screen_uv);
		draw_box(make_aabb(V2(-1000,-1000), V2(1000,1000)));
		draw_pop_shader();

		// Should appear under the white circle.
		draw_push_layer(1);
		draw_push_color(color_blue());
		draw_circle(V2(-220,200), 20, 3);
		draw_circle(V2( 220,200), 20, 3);
		draw_pop_color();
		draw_pop_layer();
		render_to(canvas1, true);

		// Apply a CRT filter to the smaller canvas.
		draw_canvas(canvas1, 0,0, w,h);
		draw_push_vertex_attributes(0.3f, 0.05f, 0.5f, 1.2f); // Could have also used uniforms.
		draw_push_shader(crt_filter);
		draw_canvas(canvas1, 0,0*0.5f, w*0.5f,h*0.5f);
		draw_pop_shader();
		draw_pop_vertex_attributes();
		render_to(canvas2, true);

		// Draw a huge white circle over everything else.
		// Make sure alpha compositing is working as intended.
		draw_circle(V2(0,0), 600*(cosf((float)CF_SECONDS)*0.5f+0.5f), 5);
		render_to(canvas3, true);
		draw_canvas(canvas3, 0,0, w,h);

		// See if blitting directory to the app has any issues, there used to be a y-flip bug here.
		render_to(canvas2, false);
		copy_canvas_to(canvas2, app_get_canvas());

		app_draw_onto_screen();
	}

	destroy_shader(screen_uv);
	destroy_shader(crt_filter);
	destroy_app();

	return 0;
}
