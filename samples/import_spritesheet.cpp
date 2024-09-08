#include <cute.h>
using namespace Cute;

void mount_directory_as(const char* dir, const char* as)
{
	CF_Path path = fs_get_base_directory();
	path.normalize();
	path += dir;
	fs_mount(path.c_str(), as);
}

void get_png_wh(const char* path, int* x, int* y)
{
	size_t sz = 0;
	void* data = fs_read_entire_file_to_memory(path, &sz);
	CF_ASSERT(data);
	image_load_png_wh(data, (int)sz, x, y);
	cf_free(data);
}

struct Atlas
{
	CF_Texture texture;
	uint64_t base_id;
};

Atlas load_sub_images(const char* path, int tile_size, int* w_out, int* h_out)
{
	static uint64_t image_id = 0;
	uint64_t base_id = image_id;
	int w, h;
	get_png_wh(path, &w, &h);
	w = w / tile_size;
	h = h / tile_size;
	if (w_out) *w_out = w;
	if (h_out) *h_out = h;

	int sub_image_count = w * h;
	CF_AtlasSubImage* sub_images = (CF_AtlasSubImage*)cf_alloc(sizeof(CF_AtlasSubImage) * sub_image_count);

	for (int i = 0; i < sub_image_count; ++i) {
		CF_AtlasSubImage* sub_image = sub_images + i;
		int x = i % w;
		int y = i / w;

		// Assign a unique id for each sub-image. This id must be unique across *all* sub-images across *all* atlases.
		// Using a static integer and incrementing works well.
		sub_image->image_id = image_id++;
		sub_image->w = tile_size;
		sub_image->h = tile_size;

		// Calculate normalized uv coordinates. Each uv coordinate is a number from
		// 0 to 1, where (0,0) is the top-left of the image, and (1,1) is the bottom right.
		sub_image->minx = (float)x / (float)w;       // Top-left X
		sub_image->miny = (float)y / (float)h;       // Top-left Y
		sub_image->maxx = (float)(x + 1) / (float)w; // Bottom-right X
		sub_image->maxy = (float)(y + 1) / (float)h; // Bottom-right Y
	}

	Atlas atlas;
	atlas.texture = cf_register_premade_atlas(path, sub_image_count, sub_images);
	atlas.base_id = base_id;

	return atlas;
}

// Creates a sprite from an atlas that has been preloaded. The base_id is returned by `load_sub_images`, and index represents
// an x,y coordinate into the atlas. You can do: y * w + x to convert from x,y pair to a single index.
CF_Sprite sub_sprite(uint64_t base_id, int index)
{
	return make_premade_sprite(base_id + (uint64_t)index);
}

// By default CF draws sprites at their center, but for pixel art it's often preferred to
// render sprites where (x,y) is the top-left pixel. This makes it easy to ensure the pixels don't get warped by rendering
// halfway between two pixels.
void draw_sprite(CF_Sprite sprite, float x, float y)
{
	draw_push();
	x = roundf(x) + sprite.w * 0.5f;
	y = roundf(y) + sprite.w * 0.5f;
	draw_translate(x, y);
	draw_sprite(sprite);
	draw_pop();
}

int main(int argc, char* argv[])
{
	make_app("Spritesheet", 0, 0, 0, 480*2, 270*2, APP_OPTIONS_RESIZABLE_BIT | APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	mount_directory_as("import_spritesheet_data", "/");
	set_target_framerate(60);

	// Load up an atlas.
	int w, h;
	Atlas atlas = load_sub_images("tiles.png", 16, &w, &h);

	while (app_is_running()) {
		app_update();

		// Center the tiles on the screen.
		float center_x = w * 16.0f * 0.5f;
		float center_y = h * 16.0f * 0.5f;

		// Draw each tile.
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				int sub_image_index = j * w + i;

				// Fetch a sub-image from the atlas by index.
				CF_Sprite s = sub_sprite(atlas.base_id, sub_image_index);

				// Draw each tile.
				draw_sprite(s, i * 16.0f - center_x, j * 16.0f - center_y);
			}
		}

		app_draw_onto_screen(true);
	}

	destroy_texture(atlas.texture);
	destroy_app();
	return 0;
}
