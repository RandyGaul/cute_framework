/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <imgui/imgui.h>
#include <cute.h>
using namespace cute;

spritebatch_t* sb;
array<sprite_t> sprites; // not sure if we need this

struct Level
{
	int start_x;
	int start_y;
	array<array<char>> data;
} level;

struct Hero
{
	int x, y;
	sprite_t sprite;
	bool initialized = false;
} hero;

struct Tile
{
	int x, y;
	sprite_t sprite;
	bool empty = true;
};

array<array<Tile>> level_sprites;

string_t level1_raw_data[] = {
	"11111111",
	"10000011",
	"10100p01",
	"10100001",
	"11111111",
};

v2 tile2world(int x, int y)
{
	float w = 16;
	float h = 16;
	float y_offset = level_sprites.count() * h;
	return v2((float)x * w, -(float)y * h + y_offset);
}

sprite_t AddSprite(string_t path)
{
	sprite_t sprite;
	error_t err = sprite_batch_easy_sprite(sb, path.c_str(), &sprite);
	if (err.is_error()) {
		printf("%s\n", err.details);
	}
	sprites.add(sprite);
	return sprite;
}

void LoadLevel(string_t* l, int vcount)
{
	for (int i = 0; i < vcount; ++i)
	{
		for (int j = 0; j < l[i].len(); ++j)
		{
			Tile tile;
			tile.x = j;
			tile.y = i;

			switch (l[i][j])
			{
			case '1':
				tile.empty = false;
				tile.sprite = AddSprite("data/tile68.png");
				tile.sprite.transform.p = tile2world(j, i);
				break;

			case '0':
				tile.empty = true;
				break;

			case 'p':
				CUTE_ASSERT(!hero.initialized);
				hero.x = j;
				hero.y = i;
				hero.sprite = AddSprite("data/tile0.png");
				hero.sprite.transform.p = tile2world(j, i);
				level.start_x = j;
				level.start_y = i;
				hero.initialized = true;
				break;
			}

			level_sprites[i].add(tile);
			level.data[i].add(l[i][j]); // add everything into the level data
		}
	}
}

void BatchLevel(const array<array<Tile>>& grid)
{
	for (int i = 0; i < grid.count(); ++i)
	{
		for (int j = 0; j < grid[i].count(); ++j)
		{
			if (!grid[i][j].empty) {
				sprite_batch_push(sb, grid[i][j].sprite);
			}
		}
	}
}

void HandleInput(app_t* app, float dt)
{
	int x = hero.x;
	int y = hero.y;

	if (key_was_pressed(app, KEY_R)) {
		x = level.start_x;
		y = level.start_y;
	}

	if (key_was_pressed(app, KEY_W))
		y -= 1;
	if (key_was_pressed(app, KEY_S))
		y += 1;

	if (key_was_pressed(app, KEY_D))
		x += 1;
	if (key_was_pressed(app, KEY_A))
		x -= 1;

	// check for collisions
	// if we did't collide, assign the new position
	if (level_sprites[y][x].empty) {
		hero.x = x;
		hero.y = y;
		hero.sprite.transform.p = tile2world(x, y);
	}
}

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Block Man", 0, 0, 640, 480, options);
	file_system_mount(file_system_get_base_dir(), "", 1);
	gfx_init(app);
	gfx_init_upscale(app, 320, 240, GFX_UPSCALE_MAXIMUM_ANY);
	ImGui::SetCurrentContext(app_init_imgui(app));

	sb = sprite_batch_easy_make(app, "data");

	int vcount = sizeof(level1_raw_data) / sizeof(level1_raw_data[0]);
	level_sprites.ensure_count(vcount);
	level.data.ensure_count(vcount);
	LoadLevel(level1_raw_data, vcount);

	float t = 0;

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		HandleInput(app, dt);

		sprite_batch_push(sb, hero.sprite);

		BatchLevel(level_sprites);

		sprite_batch_flush(sb);

		static bool hello_open = true;
		if (hello_open) {
			ImGui::Begin("Hello", &hello_open);
			static bool push_me;
			ImGui::Checkbox("Push Me", &push_me);
			if (push_me) {
				ImGui::Separator();
				ImGui::Indent();
				ImGui::Text("This is a Dear ImGui Window!");
			}
			ImGui::End();
		}

		gfx_flush(app);
	}

	app_destroy(app);

	return 0;
}
