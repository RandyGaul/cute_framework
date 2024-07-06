#include <cute.h>
#include "imgui.h"
#include <sokol/sokol_gfx_imgui.h>

// Example program showing cf_fetch_image for rendering textures within Dear ImGui windows

int main(int argc, char *argv[])
{
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED | CF_APP_OPTIONS_RESIZABLE;
	CF_Result result = cf_make_app("cute imgui image test", 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;
	
	cf_app_init_imgui(false);
	
	CF_Sprite sprite = cf_make_demo_sprite();
	sprite.scale = cf_mul_v2_f(sprite.scale, 3.0f);
	
	const char **names = (const char**)cf_hashtable_keys(sprite.animations);
	int count = cf_hashtable_count(sprite.animations);
	
	int animation_index = 0;
	
	while (cf_app_is_running())
	{
		cf_app_update(nullptr);
		
		if (cf_key_just_pressed(CF_KEY_SPACE))
		{
			animation_index = (animation_index + 1) % count;
			cf_sprite_play(&sprite, names[animation_index]);
		}
		
		cf_sprite_update(&sprite);
		cf_draw_sprite(&sprite);

		const char* text = "Press space";
		Cute::draw_text(text, V2(-Cute::text_width(text)*0.5f-8.0f,200));
		
		ImGui::Begin("Sprite");
		{
			CF_TemporaryImage image = cf_fetch_image(&sprite);
			
			ImTextureID id = (ImTextureID)image.tex.id;
			ImVec2 size = { (float)image.w * 5.0f, (float)image.h * 5.0f };
			// y is flipped
			ImVec2 uv0 = { image.u.x, image.v.y };
			ImVec2 uv1 = { image.v.x, image.u.y };
			ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
			ImVec4 border_color = {0.0f, 0.0f, 0.0f, 1.0f};
			
			ImGui::Image(id, size, uv0, uv1, color, border_color);
		}
		ImGui::End();
		
		sg_imgui_t* sg_imgui = cf_app_get_sokol_imgui();
		if (ImGui::BeginMainMenuBar()) 
		{
			if (ImGui::BeginMenu("sokol-gfx", true)) 
			{
				ImGui::MenuItem("Buffers", NULL, &sg_imgui->buffers.open, true);
				ImGui::MenuItem("Images", NULL, &sg_imgui->images.open, true);
				ImGui::MenuItem("Shaders", NULL, &sg_imgui->shaders.open, true);
				ImGui::MenuItem("Pipelines", NULL, &sg_imgui->pipelines.open, true);
				ImGui::MenuItem("Passes", NULL, &sg_imgui->passes.open, true);
				ImGui::MenuItem("Calls", NULL, &sg_imgui->capture.open, true);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		cf_app_draw_onto_screen(true);
	}
	
	cf_destroy_app();
	
	return 0;
}
