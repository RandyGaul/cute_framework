/*
	Demonstrates CF's draw filter modes on a tilemap under subpixel camera motion.

	Adjacent 16x16 tiles are drawn as individual sprites while the camera slowly pans by
	fractions of a pixel. Each tile is its own atlas entry, and the atlas pads every entry
	with a 1px transparent ring to keep neighboring images from bleeding into each other.
	A sprite's quad covers exactly its own 16x16 pixels, so quads meet edge to edge -- but
	any filter that samples past the sprite's uv rect reaches that ring, pulls in
	transparency, and opens a seam the background shows through:

	  NEAREST  Takes the texel the sample lands in, which is always one of the sprite's
	           own: no seams, but interiors shimmer as texel rows pop in and out while the
	           camera slides across subpixel positions.
	  LINEAR   Samples raw uvs, so any tap within half a texel of a tile edge blends with
	           the ring. Tile edges go semi-transparent and seams open at subpixel offsets.
	  SMOOTH   Snaps to texel centers with a sub-pixel ramp at texel seams, then clamps the
	           uv into the sprite's own rect so the ring is unreachable. Stable interiors
	           and closed seams at any camera offset or zoom. The tradeoff: silhouettes no
	           longer fade out through the ring, so a rotated sprite's outline is a hard
	           quad edge rather than an antialiased one.

	Controls:
	  SPACE    cycle filter mode
	  P        pause/resume the camera pan
	  UP/DOWN  zoom in/out
	  S        save one png per filter mode (current camera offset)

	A 5x3 tile block slowly rotates in the sky: under SMOOTH its interior pixels should
	stay stable (no shimmering crawl like NEAREST) and its seams stay closed even though
	the shared tile edges are not axis-aligned.

	Run `tilefilters screenshot <prefix> [offset_px] [zoom] [angle_deg]` to save
	deterministic pngs of every filter mode at the given subpixel camera offset (default
	0.5, worst case for seams) plus a zero-offset control, then exit.
*/

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

#define TILE 16
#define MAP_W 32
#define MAP_H 12
#define CANVAS_W 1280
#define CANVAS_H 720

typedef enum TileKind
{
	TILE_NONE,
	TILE_GRASS_L,
	TILE_GRASS_M,
	TILE_GRASS_R,
	TILE_DIRT,
	TILE_KIND_COUNT,
} TileKind;

void mount_content_directory_as(const char* dir)
{
	// cf_string_append reassigns the dynamic string in place: one allocation, one free.
	char* path = cf_path_normalize(cf_fs_get_base_directory());
	cf_string_append(path, "/import_spritesheet_data");
	cf_fs_mount(path, dir, false);
	cf_string_free(path);
}

// Cut one 16x16 tile out of the sheet as its own easy sprite. Each tile becomes a separate
// atlas entry, with its own transparent ring around it -- which is how a real tilemap runs
// into the seam behavior described above.
static CF_Sprite s_cut_tile(const CF_Image* sheet, int col, int row)
{
	CF_Pixel px[TILE * TILE];
	for (int y = 0; y < TILE; ++y) {
		for (int x = 0; x < TILE; ++x) {
			px[y * TILE + x] = sheet->pix[(row * TILE + y) * sheet->w + (col * TILE + x)];
		}
	}
	return cf_make_easy_sprite_from_pixels(px, TILE, TILE);
}

static CF_Sprite s_tiles[TILE_KIND_COUNT];
static TileKind s_map[MAP_H][MAP_W];

static void s_build_map(void)
{
	for (int y = 0; y < MAP_H; ++y) {
		for (int x = 0; x < MAP_W; ++x) {
			if (y == 4) s_map[y][x] = TILE_GRASS_M;
			else if (y > 4) s_map[y][x] = TILE_DIRT;
			else s_map[y][x] = TILE_NONE;
		}
	}
	// A floating platform so isolated caps (transparent rounded corners) are visible too.
	s_map[1][10] = TILE_GRASS_L;
	s_map[1][11] = TILE_GRASS_M;
	s_map[1][12] = TILE_GRASS_M;
	s_map[1][13] = TILE_GRASS_R;
}

// A 5x3 tile block rotating about its center. Verifies two things under rotation: the
// interior stays pixel-stable (the point of SMOOTH), and adjacent tiles stay seam-free
// even when the shared edges are not axis-aligned.
static void s_draw_rotated_block(float angle)
{
	static const TileKind block[3][5] = {
		{ TILE_GRASS_L, TILE_GRASS_M, TILE_GRASS_M, TILE_GRASS_M, TILE_GRASS_R },
		{ TILE_DIRT, TILE_DIRT, TILE_DIRT, TILE_DIRT, TILE_DIRT },
		{ TILE_DIRT, TILE_DIRT, TILE_DIRT, TILE_DIRT, TILE_DIRT },
	};
	CF_V2 center = cf_v2(120, 56);
	CF_SinCos r = cf_sincos(angle);
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 5; ++x) {
			CF_Sprite s = s_tiles[block[y][x]];
			CF_V2 local = cf_v2((x - 2) * (float)TILE, (1 - y) * (float)TILE);
			s.transform.r = r;
			s.transform.p = cf_add_v2(center, cf_mul_sc_v2(r, local));
			cf_draw_sprite(&s);
		}
	}
}

static void s_draw_scene(CF_DrawFilterMode mode, float zoom, float offx, float offy, float angle)
{
	cf_draw_push();
	cf_draw_scale(zoom, zoom);
	cf_draw_translate(offx, offy);
	cf_draw_push_filter(mode);
	for (int y = 0; y < MAP_H; ++y) {
		for (int x = 0; x < MAP_W; ++x) {
			TileKind kind = s_map[y][x];
			if (kind == TILE_NONE) continue;
			CF_Sprite s = s_tiles[kind];
			s.transform.p = cf_v2((x - MAP_W / 2) * (float)TILE, (MAP_H / 2 - y) * (float)TILE);
			cf_draw_sprite(&s);
		}
	}
	s_draw_rotated_block(angle);
	cf_draw_pop_filter();
	cf_draw_pop();
}

static const char* s_mode_name(CF_DrawFilterMode mode)
{
	switch (mode) {
	case CF_DRAW_FILTER_NEAREST: return "NEAREST";
	case CF_DRAW_FILTER_LINEAR: return "LINEAR";
	case CF_DRAW_FILTER_SMOOTH: return "SMOOTH";
	default: return "???";
	}
}

// Renders the queued draw commands into an offscreen canvas and saves it as a png.
static void s_save_png(const char* path)
{
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(CANVAS_W, CANVAS_H));
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);
	CF_Readback rb = cf_canvas_readback(canvas);
	while (!cf_readback_ready(rb)) {}
	CF_Image img;
	img.w = CANVAS_W;
	img.h = CANVAS_H;
	img.pix = (CF_Pixel*)cf_alloc(CANVAS_W * CANVAS_H * sizeof(CF_Pixel));
	cf_readback_data(rb, img.pix, CANVAS_W * CANVAS_H * (int)sizeof(CF_Pixel));
	cf_destroy_readback(rb);
	cf_destroy_canvas(canvas);
	cf_image_save_png(path, &img);
	cf_free(img.pix);
	printf("saved %s\n", path);
}

int main(int argc, char* argv[])
{
	bool screenshot = argc > 1 && CF_STRCMP(argv[1], "screenshot") == 0;
	const char* prefix = argc > 2 ? argv[2] : "tile_filters";
	float shot_off_px = argc > 3 ? (float)atof(argv[3]) : 0.5f;
	float shot_zoom = argc > 4 ? (float)atof(argv[4]) : 3.0f;
	float shot_angle = (argc > 5 ? (float)atof(argv[5]) : 22.5f) * (CF_PI / 180.0f);
	// Screenshot mode disables HiDPI so one draw unit is exactly one canvas pixel,
	// making the half-pixel camera offset land on true half-pixel screen positions.
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | (screenshot ? CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_HIGH_DPI_BIT : CF_APP_OPTIONS_RESIZABLE_BIT);
	CF_Result result = cf_make_app("Tile Filters", 0, 0, 0, CANVAS_W, CANVAS_H, options, argv[0]);
	if (cf_is_error(result)) return -1;
	mount_content_directory_as("/");
	cf_fs_set_write_directory(cf_fs_get_base_directory());

	CF_Image sheet;
	result = cf_image_load_png("/tiles.png", &sheet);
	if (cf_is_error(result)) {
		printf("Failed to load /tiles.png\n");
		return -1;
	}
	s_tiles[TILE_GRASS_L] = s_cut_tile(&sheet, 0, 0);
	s_tiles[TILE_GRASS_M] = s_cut_tile(&sheet, 1, 0);
	s_tiles[TILE_GRASS_R] = s_cut_tile(&sheet, 2, 0);
	s_tiles[TILE_DIRT] = s_cut_tile(&sheet, 1, 1);
	cf_image_free(&sheet);
	s_build_map();

	CF_DrawFilterMode modes[] = {
		CF_DRAW_FILTER_NEAREST,
		CF_DRAW_FILTER_LINEAR,
		CF_DRAW_FILTER_SMOOTH,
	};
	int mode_count = CF_ARRAY_SIZE(modes);
	int mode = 0;
	float zoom = 3.0f;
	float t = 0;
	bool paused = false;
	int shot_frame = 0;
	int interactive_shot = -1;
	float shot_offx = 0, shot_offy = 0;

	cf_clear_color(0.35f, 0.60f, 0.95f, 1.0f);

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		if (screenshot) {
			// One frame per capture: every mode at a half-pixel offset (worst case for
			// seams) plus a zero-offset control. Skip frame 0 so the window settles.
			if (shot_frame > 0) {
				int idx = shot_frame - 1;
				if (idx >= mode_count * 2) break;
				int m = idx / 2;
				bool half = (idx % 2) == 0;
				float off = half ? shot_off_px / shot_zoom : 0;
				s_draw_scene(modes[m], shot_zoom, off, off, shot_angle);
				char path[256];
				snprintf(path, sizeof(path), "%s_%s_%s.png", prefix, s_mode_name(modes[m]), half ? "half" : "zero");
				for (char* c = path; *c; ++c) if (*c == ' ' || *c == '(' || *c == ')') *c = '-';
				s_save_png(path);
				shot_frame++;
				continue;
			}
			shot_frame++;
			cf_app_draw_onto_screen(true);
			continue;
		}

		// One capture per frame while an S-key screenshot burst is in flight.
		if (interactive_shot >= 0) {
			if (interactive_shot < mode_count) {
				s_draw_scene(modes[interactive_shot], zoom, shot_offx, shot_offy, t * 0.25f);
				char path[256];
				snprintf(path, sizeof(path), "%s_%s.png", prefix, s_mode_name(modes[interactive_shot]));
				for (char* c = path; *c; ++c) if (*c == ' ' || *c == '(' || *c == ')') *c = '-';
				s_save_png(path);
				interactive_shot++;
				continue;
			}
			interactive_shot = -1;
		}

		if (cf_key_just_pressed(CF_KEY_SPACE)) mode = (mode + 1) % mode_count;
		if (cf_key_just_pressed(CF_KEY_P)) paused = !paused;
		if (cf_key_just_pressed(CF_KEY_UP)) zoom += 0.25f;
		if (cf_key_just_pressed(CF_KEY_DOWN)) zoom = cf_max(0.5f, zoom - 0.25f);
		if (!paused) t += CF_DELTA_TIME;

		// Slow sinusoidal pan; tile edges continuously cross subpixel screen positions.
		float offx = sinf(t * 0.30f) * 2.0f;
		float offy = cosf(t * 0.23f) * 1.5f;

		if (cf_key_just_pressed(CF_KEY_S)) {
			interactive_shot = 0;
			shot_offx = offx;
			shot_offy = offy;
		}

		s_draw_scene(modes[mode], zoom, offx, offy, t * 0.25f);

		cf_draw_push_color(cf_color_white());
		cf_push_font_size(20);
		char label[256];
		snprintf(label, sizeof(label), "Filter: %s  --  SPACE cycle, P pause, UP/DOWN zoom (%.2fx), S screenshots", s_mode_name(modes[mode]), zoom);
		cf_draw_text(label, cf_v2(-CANVAS_W * 0.5f + 10, CANVAS_H * 0.5f - 10), -1);
		cf_pop_font_size();
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
