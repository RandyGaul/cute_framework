#include <cute.h>
#include <dcimgui.h>

#define TILE_SIDE 25
#define TILE_SIZE 625

typedef struct Camera
{
	CF_V2 target;
	CF_V2 offset;
	float rotation;
	float scale;
} Camera;

static CF_Sprite make_ground_tile_sprite()
{
	CF_Pixel g = cf_pixel_grey();
	CF_Pixel w = cf_pixel_white();
	CF_Pixel b = cf_pixel_black();
	CF_Pixel pixels[TILE_SIZE] = {
		g, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, g,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		w, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, b,
		g, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, g,
	};
	return cf_make_easy_sprite_from_pixels(pixels, TILE_SIDE, TILE_SIDE);
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Basic Camera", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui();

	CF_Sprite player_sprite = cf_make_demo_sprite();
	cf_sprite_play(&player_sprite, "idle");

	CF_V2 player_pos = { .x = 0.0f, .y = 0.0f };

	CF_Sprite ground_tile = make_ground_tile_sprite();

	int tilemap_width = 9;
	int tilemap_height = 5;
	CF_Aabb tilemap_bounds = {0};
	tilemap_bounds.max.x = cf_floor((float)tilemap_width / 2) * TILE_SIDE - TILE_SIDE * 0.5f * (1 - tilemap_width % 2);
	tilemap_bounds.max.y = cf_floor((float)tilemap_height / 2) * TILE_SIDE - TILE_SIDE * 0.5f * (1 - tilemap_height % 2);
	tilemap_bounds.min.x = -tilemap_bounds.max.x;
	tilemap_bounds.min.y = -tilemap_bounds.max.y;

	Camera camera = {0};
	camera.scale = 2.0f;

	bool show_crosshair = true;
	bool show_boarder = true;

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// update
		if (cf_key_down(CF_KEY_LEFT)) {
			player_pos.x -= 50 * CF_DELTA_TIME;
		}
		if (cf_key_down(CF_KEY_RIGHT)) {
			player_pos.x += 50 * CF_DELTA_TIME;
		}
		if (cf_key_down(CF_KEY_UP)) {
			player_pos.y += 50 * CF_DELTA_TIME;
		}
		if (cf_key_down(CF_KEY_DOWN)) {
			player_pos.y -= 50 * CF_DELTA_TIME;
		}
		player_pos.x = cf_clamp(player_pos.x, tilemap_bounds.min.x, tilemap_bounds.max.x);
		player_pos.y = cf_clamp(player_pos.y, tilemap_bounds.min.y, tilemap_bounds.max.y);

		// make camera follows player
		camera.target = player_pos;

		// imgui
		ImGui_SetNextWindowPos((ImVec2){ 9, 276 }, ImGuiCond_FirstUseEver);
		ImGui_SetNextWindowSize((ImVec2){ 282, 195 }, ImGuiCond_FirstUseEver);
		if (ImGui_Begin("camera", NULL, ImGuiWindowFlags_None)) {
			ImGui_BeginDisabled(true);
			float v[2] = { camera.target.x, camera.target.y };
			ImGui_InputFloat2("target", v);
			ImGui_EndDisabled();
			ImGui_SliderFloatEx("X offset", &camera.offset.x, -128.0f, 128.0f, NULL, ImGuiSliderFlags_ClampOnInput);
			ImGui_SliderFloatEx("Y offset", &camera.offset.y, -128.0f, 128.0f, NULL, ImGuiSliderFlags_ClampOnInput);
			ImGui_SliderAngleEx("rotation", &camera.rotation, -45.0f, 45.0f, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
			ImGui_SliderFloatEx("scale", &camera.scale, 0.5f, 8.0f, NULL, ImGuiSliderFlags_ClampOnInput);
			ImGui_Checkbox("show crosshair", &show_crosshair);
			ImGui_Checkbox("show border", &show_boarder);
		}
		ImGui_End();

		// draw
		cf_draw_push();

		// apply camera
		cf_draw_translate_v2(camera.offset);
		cf_draw_scale(camera.scale, camera.scale);
		cf_draw_rotate(-camera.rotation);
		cf_draw_translate_v2(cf_neg(camera.target));

		// draw tilemap
		for (int x = 0; x < tilemap_width; x++) {
			for (int y = 0; y < tilemap_height; y++) {
				CF_V2 tile_pos = cf_v2(tilemap_bounds.min.x + x * TILE_SIDE, tilemap_bounds.min.y + y * TILE_SIDE);
				cf_draw_push();
				cf_draw_translate_v2(tile_pos);
				cf_draw_sprite(&ground_tile);
				cf_draw_pop();
			}
		}

		// draw player
		cf_draw_push();
		cf_draw_translate_v2(player_pos);
		cf_draw_sprite(&player_sprite);
		cf_draw_pop();

		cf_draw_pop();

		CF_Aabb screen_bounds = cf_screen_bounds_to_world();
		if (show_crosshair) {
			cf_draw_push_color(cf_color_green());
			cf_draw_line(cf_v2(camera.offset.x, screen_bounds.max.y), cf_v2(camera.offset.x, screen_bounds.min.y), 1.0f);
			cf_draw_line(cf_v2(screen_bounds.max.x, camera.offset.y), cf_v2(screen_bounds.min.x, camera.offset.y), 1.0f);
			cf_draw_pop_color();
		}
		if (show_boarder) {
			cf_draw_push_color(cf_color_red());
			cf_draw_box(screen_bounds, 4.0f, 0.0f);
			cf_draw_pop_color();
		}

		cf_draw_text("use arrow keys to move", cf_v2(screen_bounds.min.x + 8, screen_bounds.max.y - 8), -1);

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();

	return 0;
}
