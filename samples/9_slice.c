#include <cute.h>
#include <dcimgui.h>

void mount_content_directory_as(const char* dir)
{
	const char* path = cf_fs_get_base_directory();
	path = cf_path_normalize(path);
	path = cf_string_append(path, "/9_slice_data");
	cf_fs_mount(path, dir, false);
	cf_string_free(path);
}

int main(int argc, char* argv[])
{
	cf_make_app("9 Slice", 0, 0, 0, 1024, 768, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	mount_content_directory_as("/");

	cf_app_init_imgui();

	CF_Sprite sprite = cf_make_sprite("/9_slice.ase");
	
	CF_Sprite demo = cf_make_demo_sprite();
	cf_sprite_play(&demo, "spin");
	float rotation = 0.0f;
	bool is_tiled = false;

	cf_htbl const char** animations = (const char**)cf_hashtable_keys(sprite.animations);
	int animation_index = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		cf_draw_scale(4, 4);

		cf_sprite_update(&demo);

		if (is_tiled) {
			cf_draw_sprite_9_slice_tiled(&sprite);
		}
		else {
			cf_draw_sprite_9_slice(&sprite);
		}

		ImGui_Begin("9 Slice", NULL, ImGuiWindowFlags_None);
		if (ImGui_ComboChar("Animation", &animation_index, animations, cf_hashtable_count(sprite.animations))) {
			cf_sprite_play(&sprite, animations[animation_index]);
		}

		ImGui_Checkbox("Tiled?", &is_tiled);
		ImGui_SliderFloat2("Position", (float*)&sprite.transform.p, -20, 20);
		ImGui_SliderFloat2("Scale", (float*)&sprite.scale, -2, 2);
		ImGui_SliderFloat("Rotation", &rotation, 0, 360);
		sprite.transform.r = cf_sincos(rotation * CF_PI / 180.0f);
		ImGui_End();

		cf_sprite_draw(&demo);

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
