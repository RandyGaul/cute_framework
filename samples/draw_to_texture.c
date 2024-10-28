#include <cute.h>
#include <cimgui.h>

int main(int argc, char* argv[])
{
	float w = 640.0f;
	float h = 480.0f;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Draw to Texture", 0, 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui();

	// Create an offscreen canvas.
	CF_Canvas offscreen = cf_make_canvas(cf_canvas_defaults((int)(w*0.5f), (int)(h*0.5f)));

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// Draw offscreen.
		cf_draw_circle2(cf_v2(0,0), 100, 5);
		cf_render_to(offscreen, true);

		// Draw twice at half-size on the left and right of the screen.
		cf_draw_canvas(offscreen, cf_v2(-w*0.25f,0), cf_v2(w*0.5f,h*0.5f));
		cf_draw_canvas(offscreen, cf_v2( w*0.25f,0), cf_v2(w*0.5f,h*0.5f));

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_canvas(offscreen);
	cf_destroy_app();

	return 0;
}
