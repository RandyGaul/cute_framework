#include <cute.h>
#include <cimgui.h>

int main(int argc, char* argv[])
{
	int w = 640, h = 480;
	cf_make_app("Stencil Outline", 0, 0, 0, w, h, CF_APP_OPTIONS_RESIZABLE_BIT | CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_set_target_framerate(200);

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		CF_RenderState state = cf_render_state_defaults();
		state.stencil.enabled = true;

		cf_draw_push_antialias(false);
		cf_render_settings_set_uniform_int("no_discard", 0); // "Hidden" feature to turn off alpha-discard in the draw API.

		// Render a full white circle.
		// Increment stencil buffer.
		state.stencil.back = state.stencil.front = (CF_StencilFunction) {
			.compare = CF_COMPARE_FUNCTION_ALWAYS,
			.pass_op = CF_STENCIL_OP_INCREMENT_CLAMP,
		};
		state.stencil.read_mask = 0x0;
		state.stencil.write_mask = 0xFF;
		state.stencil.reference = 1;
		cf_render_settings_push_render_state(state);
		{
			cf_draw_circle_fill2(cf_v2(0, 0), 100);
			cf_render_to(cf_app_get_canvas(), true);
		}
		cf_render_settings_pop_render_state();

		// Draw the wedge as an oversized triangle.
		// Increment stencil buffer again; 0->1, 1->2, where 1->2 is the intersection of the wedge + circle.
		state.stencil.back = state.stencil.front = (CF_StencilFunction) {
			.compare = CF_COMPARE_FUNCTION_ALWAYS,
			.pass_op = CF_STENCIL_OP_INCREMENT_CLAMP,
		};
		state.stencil.read_mask = 0x0;
		state.stencil.write_mask = 0xFF;
		state.stencil.reference = 1;
		cf_render_settings_push_render_state(state);
		{
			cf_draw_push_color(cf_color_red());
			cf_draw_tri_fill(cf_v2(0,0), cf_v2(150,150), cf_v2(-150,150), 0);
			cf_render_to(cf_app_get_canvas(), false);
		}
		cf_render_settings_pop_render_state();

		// Clear everything except the intersection.
		cf_render_settings_set_uniform_int("no_discard", 1);
		state.stencil.back = state.stencil.front = (CF_StencilFunction) {
			.compare = CF_COMPARE_FUNCTION_NOT_EQUAL,
			.pass_op = CF_STENCIL_OP_ZERO,
		};
		state.stencil.read_mask = 0xFF;
		state.stencil.write_mask = 0x0;
		state.stencil.reference = 2;
		cf_render_settings_push_render_state(state);
		{
			cf_draw_push_color(cf_color_invisible());
			cf_draw_box_fill(cf_make_aabb(cf_v2(-1000,-1000), cf_v2(1000,1000)), 0);
			cf_render_to(cf_app_get_canvas(), false);
		}
		cf_render_settings_pop_render_state();

		// Draw yellow border.
		cf_draw_push_antialias(true);
		cf_render_settings_set_uniform_int("no_discard", 0);
		cf_draw_push_color(cf_color_yellow());
		cf_draw_circle2(cf_v2(0,0), 100, 3);

		// Draw the result onto the screen
		cf_app_draw_onto_screen(false);
	}

	cf_destroy_app();

	return 0;
}
