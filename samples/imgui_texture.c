#include <cute.h>
#include <dcimgui.h>

static inline ImTextureRef imgui_texture_ref_from_id(ImTextureID tex_id) {
	ImTextureRef tex_ref = { ._TexData = NULL, ._TexID = tex_id };
	return tex_ref;
}

ImTextureRef imgui_texture_ref_from_cf_texture(CF_Texture tex) {
	ImTextureID tex_id = (ImTextureID)(uintptr_t)cf_texture_handle(tex);
	return imgui_texture_ref_from_id(tex_id);
}

CF_Texture cf_texture_from_sprite(
	CF_Sprite* sprite,
	CF_Canvas canvas,
	int canvas_width,
	int canvas_height
) {
	cf_draw_push();
	float scale_x = cf_floor((float)canvas_width / (float)sprite->w);
	float scale_y = cf_floor((float)canvas_height / (float)sprite->h);
	float scale = (scale_x < scale_y) ? scale_x : scale_y;
	cf_draw_scale(scale, scale);
	cf_sprite_update(sprite);
	cf_draw_sprite(sprite);
	cf_clear_color(0.0f, 0.0f, 0.0f, 0.0f);
	cf_render_to(canvas, true);
	CF_Texture tex = cf_canvas_get_target(canvas);
	cf_draw_pop();
	return tex;
}

int main(int argc, char* argv[]) {
	CF_Result result = cf_make_app(
		"ImGui Texture",
		0,
		0,
		0,
		640,
		360,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT,
		argv[0]
	);
	if (cf_is_error(result)) {
		return -1;
	}

	cf_app_init_imgui();
	int canvas_width = 512;
	int canvas_height = 512;
	CF_Sprite demo_sprite = cf_make_demo_sprite();
	cf_sprite_play(&demo_sprite, "spin");
	CF_Canvas canvas = cf_make_canvas(
		cf_canvas_defaults(canvas_width, canvas_height)
	);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		CF_Texture demo_tex = cf_texture_from_sprite(
			&demo_sprite,
			canvas,
			canvas_width,
			canvas_height
		);

		ImGui_SetNextWindowSize((ImVec2){ 128, 128 }, ImGuiCond_FirstUseEver);
		if (ImGui_Begin("Textures", NULL, ImGuiWindowFlags_None)) {
			ImTextureRef tex_ref = imgui_texture_ref_from_cf_texture(demo_tex);
			ImVec2 avail = ImGui_GetContentRegionAvail();
			ImGui_Image(tex_ref, avail);
		}
		ImGui_End();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_canvas(canvas);
	cf_destroy_app();

	return 0;
}
