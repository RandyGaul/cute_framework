#include <cute.h>
#include <cimgui.h>
#include <sokol/sokol_gfx_imgui.h>

int main(int argc, char* argv[])
{
	float w = 640.0f;
	float h = 480.0f;
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED | CF_APP_OPTIONS_RESIZABLE;
	CF_Result result = cf_make_app("Draw to Texture", 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui(false);
	sg_imgui_t* sg_imgui = cf_app_get_sokol_imgui();

	// Create an offscreen canvas.
	CF_Canvas offscreen = cf_make_canvas(cf_canvas_defaults((int)(w*0.5f), (int)(h*0.5f)));

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Draw offscreen.
		cf_draw_circle2(cf_v2(0,0), 100, 5);
		cf_render_to(offscreen, true);

		// Fetch this each frame, as it's invalidated during window-resizing.
		CF_Canvas app_canvas = cf_app_get_canvas();

		cf_canvas_blit(offscreen, cf_v2(0,0), cf_v2(1,1), app_canvas, cf_v2(0.0f,0.25f), cf_v2(0.5f,0.75f));
		cf_canvas_blit(offscreen, cf_v2(0,0), cf_v2(1,1), app_canvas, cf_v2(0.5f,0.25f), cf_v2(1.0f,0.75f));

		// Show debug views of graphics primitives.
		if (igBeginMainMenuBar()) {
			if (igBeginMenu("sokol-gfx", true)) {
				igMenuItem_BoolPtr("Buffers", NULL, &sg_imgui->buffers.open, true);
				igMenuItem_BoolPtr("Images", NULL, &sg_imgui->images.open, true);
				igMenuItem_BoolPtr("Shaders", NULL, &sg_imgui->shaders.open, true);
				igMenuItem_BoolPtr("Pipelines", NULL, &sg_imgui->pipelines.open, true);
				igMenuItem_BoolPtr("Passes", NULL, &sg_imgui->passes.open, true);
				igMenuItem_BoolPtr("Calls", NULL, &sg_imgui->capture.open, true);
				igEndMenu();
			}
			igEndMainMenuBar();
		}

		// Send the app's canvas to the screen.
		cf_app_draw_onto_screen(false);
	}

	cf_destroy_canvas(offscreen);
	cf_destroy_app();

	return 0;
}
