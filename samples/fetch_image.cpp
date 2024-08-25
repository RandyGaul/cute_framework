#include <cute.h>
#include "imgui.h"

// Example program showing cf_fetch_image for rendering textures within Dear ImGui windows

int main(int argc, char *argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute imgui image test", 0, 0, 0, 640, 480, options, argv[0]);
	if (cf_is_error(result)) return -1;
	
	cf_app_init_imgui();
	
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

			ImTextureID id = (ImTextureID)cf_texture_handle(image.tex);
			ImVec2 size = { (float)image.w * 5.0f, (float)image.h * 5.0f };
			// y is flipped
			ImVec2 uv0 = { image.u.x, image.v.y };
			ImVec2 uv1 = { image.v.x, image.u.y };
			ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
			ImVec4 border_color = {0.0f, 0.0f, 0.0f, 1.0f};
			
			ImGui::Image(id, size, uv0, uv1, color, border_color);
		}
		ImGui::End();

		cf_app_draw_onto_screen(true);
	}
	
	cf_destroy_app();
	
	return 0;
}
