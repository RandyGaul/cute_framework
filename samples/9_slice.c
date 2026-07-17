#include <cute.h>
#include <dcimgui.h>

// Demonstrates 9-slice for both aseprite sprites (center patch from .ase slices)
// and easy sprites (center patch via cf_sprite_set_center_patch).
// ASE (left) and easy (right) share the same art and controls for comparison.
// Easy frames are built from the ASE pixel data via cf_sprite_get_pixels — no separate PNGs.

void mount_content_directory_as(const char* dir)
{
	char* path = cf_path_normalize(cf_fs_get_base_directory());
	char* mounted = cf_string_append(path, "/9_slice_data");
	cf_fs_mount(mounted, dir, false);
	cf_string_free(mounted);
}

// Build an easy sprite from one frame of an ASE animation (same pixels as the .ase).
static CF_Sprite make_easy_from_ase_frame(CF_Sprite* ase, const char* animation)
{
	CF_Image img = cf_sprite_get_pixels(ase, 0, animation, 0);
	CF_Sprite easy = cf_make_easy_sprite_from_pixels(img.pix, img.w, img.h);
	cf_image_free(&img);

	// Copy the 9-slice center patch the ASE loader read from the slice data.
	cf_sprite_play(ase, animation);
	cf_sprite_set_center_patch(&easy, cf_sprite_get_center_patch(ase));
	return easy;
}

int main(int argc, char* argv[])
{
	cf_make_app("9 Slice", 0, 0, 0, 1024, 768, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	mount_content_directory_as("/");

	cf_app_init_imgui();

	CF_Sprite ase = cf_make_sprite("/9_slice.ase");

	int animation_count = cf_sprite_animation_count(&ase);
	const char** animation_names = NULL;
	for (int i = 0; i < animation_count; i++) {
		apush(animation_names, cf_sprite_animation_name_at(&ase, i));
	}
	int animation_index = 0;

	// One easy sprite per ASE animation frame (pixels pulled from the .ase).
	CF_Sprite* easy = NULL;
	afit(easy, animation_count);
	for (int i = 0; i < animation_count; i++) {
		apush(easy, make_easy_from_ase_frame(&ase, animation_names[i]));
	}
	if (animation_count > 0) {
		cf_sprite_play(&ase, animation_names[0]);
	}

	CF_Sprite demo = cf_make_demo_sprite();
	cf_sprite_play(&demo, "spin");
	float rotation = 0.0f;
	bool is_tiled = false;

	// Shared draw state for ASE and easy.
	CF_V2 position = cf_v2(0, 0);
	CF_V2 scale = cf_v2(1, 1);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		cf_draw_scale(4, 4);

		cf_sprite_update(&demo);

		ase.transform.p = position;
		ase.scale = scale;
		ase.transform.r = cf_sincos(rotation * CF_PI / 180.0f);

		CF_Sprite* easy_cur = &easy[animation_index];
		easy_cur->transform.p = position;
		easy_cur->scale = scale;
		easy_cur->transform.r = ase.transform.r;

		// ASE left, easy right — same scale/rotation for comparison.
		const float side_offset = 40.0f;
		ase.transform.p.x = position.x - side_offset;
		easy_cur->transform.p.x = position.x + side_offset;

		if (is_tiled) {
			cf_draw_sprite_9_slice_tiled(&ase);
			cf_draw_sprite_9_slice_tiled(easy_cur);
		} else {
			cf_draw_sprite_9_slice(&ase);
			cf_draw_sprite_9_slice(easy_cur);
		}

		cf_draw_text("ASE", cf_v2(position.x - side_offset - 6.0f, position.y - 30.0f), -1);
		cf_draw_text("Easy", cf_v2(position.x + side_offset - 6.0f, position.y - 30.0f), -1);

		ImGui_Begin("9 Slice", NULL, ImGuiWindowFlags_None);
		ImGui_TextWrapped("ASE (left) vs easy (right). Easy frames come from cf_sprite_get_pixels; center patch via cf_sprite_set_center_patch.");
		if (ImGui_ComboChar("Animation", &animation_index, animation_names, animation_count)) {
			cf_sprite_play(&ase, animation_names[animation_index]);
		}
		ImGui_Checkbox("Tiled?", &is_tiled);
		ImGui_SliderFloat2("Position", (float*)&position, -20, 20);
		ImGui_SliderFloat2("Scale", (float*)&scale, -2, 2);
		ImGui_SliderFloat("Rotation", &rotation, 0, 360);
		ImGui_End();

		cf_sprite_draw(&demo);

		cf_app_draw_onto_screen(true);
	}

	for (int i = 0; i < asize(easy); i++) {
		cf_easy_sprite_unload(&easy[i]);
	}
	afree(easy);
	afree(animation_names);
	cf_destroy_app();

	return 0;
}
