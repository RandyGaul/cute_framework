#include <cute.h>
using namespace Cute;

#include <imgui.h>

int w = 480/2;
int h = 270/2;
float scale = 4;

void set_imgui_scale(float scale)
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale);
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = scale;
}

int main(int argc, char* argv[])
{
	//int options = APP_OPTIONS_GFX_VULKAN_BIT;
	//int options = APP_OPTIONS_GFX_D3D11_BIT;
	//int options = APP_OPTIONS_GFX_D3D12_BIT;
	make_app("Stencil Outline", 0, 0, 0, (int)(w*scale), (int)(h*scale), options | APP_OPTIONS_RESIZABLE_BIT | APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	set_target_framerate(200);
	app_init_imgui();
	set_imgui_scale(scale/2);
	Sprite sprite = make_demo_sprite();
	sprite.play("spin");

	while (app_is_running()) {
		app_update();

		draw_scale(scale, scale);
		sprite.update();

		static bool corners = false;
		ImGui::Begin("Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Checkbox("corners", &corners);
		ImGui::End();

		// Draw the border of the sprite onto the stencil buffer.
		CF_RenderState state = cf_render_state_defaults();
		state.stencil.enabled = true;
		state.stencil.back = state.stencil.front = CF_StencilFunction {
			.compare = CF_COMPARE_FUNCTION_ALWAYS,
			.pass_op = CF_STENCIL_OP_REPLACE,
		};
		state.stencil.read_mask = 0;
		state.stencil.write_mask = 0xFF;
		state.stencil.reference = 1;
		render_settings_push_render_state(state);
		{
			// Draw sprite up/down/left/right to fill in border pixels.
			sprite.transform.p.x = -1;
			sprite.draw();
			sprite.transform.p.x = 1;
			sprite.draw();
			sprite.transform.p.x = 0;
			sprite.transform.p.y = -1;
			sprite.draw();
			sprite.transform.p.y = 1;
			sprite.draw();
			if (corners) {
				sprite.transform.p = V2(-1,-1);
				sprite.draw();
				sprite.transform.p = V2( 1,-1);
				sprite.draw();
				sprite.transform.p = V2(-1, 1);
				sprite.draw();
				sprite.transform.p = V2( 1, 1);
				sprite.draw();
			}
			render_to(app_get_canvas(), true);
			sprite.transform.p = V2(0, 0);
		}
		render_settings_pop_render_state();
		
		// Color in the border pixels as white.
		state = cf_render_state_defaults();
		state.stencil.enabled = true;
		state.stencil.back = state.stencil.front = CF_StencilFunction {
			.compare = CF_COMPARE_FUNCTION_EQUAL,
			.pass_op = CF_STENCIL_OP_KEEP,
		};
		state.stencil.read_mask = 0xFF;
		state.stencil.write_mask = 0;
		state.stencil.reference = 1;
		render_settings_push_render_state(state);
		{
			draw_box_fill(V2(0,0), (float)w, (float)h);
			render_to(app_get_canvas());
		}
		render_settings_pop_render_state();
		
		// Draw the sprite normally, centered within the border.
		sprite.draw();
		app_draw_onto_screen();
	}

	destroy_app();
	return 0;
}
