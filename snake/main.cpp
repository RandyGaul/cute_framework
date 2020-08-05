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
    v2 pstart;
    array<array<char>> data;
} level;

struct Hero
{
    sprite_t sprite;
    bool initialized = false;
} hero;

struct Tile
{
    sprite_t sprite;
    bool empty = true;
};

array<array<Tile>> level_sprites; // level sprites

string_t level1[] = {
"11111111",
"10000001",
"10000p01",
"10000001",
"11111111",
};

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
    Tile tile;
    for (int i = 0; i < vcount; ++i)
    {
        for (int j = 0; j < l[i].len(); ++j)
        {
            switch (l[i][j])
            {
            case '1':
                tile.empty = false;
                tile.sprite = AddSprite("data/tile68.png");
                tile.sprite.transform.p.x = j * tile.sprite.h;
                tile.sprite.transform.p.y = -i * tile.sprite.w + vcount * tile.sprite.h;
                level_sprites[i].add(tile);
                break;
            case '0':
                tile.empty = true;
                level_sprites[i].add(tile);
                break;
            case 'p':
                CUTE_ASSERT(!hero.initialized);
                hero.sprite = AddSprite("data/tile0.png");
                hero.sprite.transform.p.x = j * tile.sprite.h;
                hero.sprite.transform.p.y = -i * tile.sprite.w + vcount * tile.sprite.h;
                pstart = hero.sprite.transform.p;
                hero.initialized = true;
                break;
            }
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
            sprite_batch_push(sb, grid[i][j].sprite);
        }
    }
}

void HandleInput(app_t* app, float dt)
{
    v2 newpos = hero.sprite.transform.p;

    if (key_was_pressed(app, KEY_R))
        newpos = level.pstart;

    if (key_was_pressed(app, KEY_W))
        newpos.y += hero.sprite.h;
    if (key_was_pressed(app, KEY_S))
        newpos.y -= hero.sprite.h;

    if (key_was_pressed(app, KEY_D))
        newpos.x += hero.sprite.w;
    if (key_was_pressed(app, KEY_A))
        newpos.x -= hero.sprite.w;

    /*if (key_was_pressed(app, KEY_R))
        newpos = level.pstart;

    if (key_was_pressed(app, KEY_W))
        newpos.y += hero.sprite.h;
    if (key_was_pressed(app, KEY_S))
        newpos.y -= hero.sprite.h;

    if (key_was_pressed(app, KEY_D))
        newpos.x += hero.sprite.w;
    if (key_was_pressed(app, KEY_A))
        newpos.x -= hero.sprite.w;*/

    // check for collisions
    //int posToIndexY = int(hero.sprite.transform.p.y / level_sprites[0][0].sprite.w);
    //int posToIndexX = int(hero.sprite.transform.p.x / level_sprites[0][0].sprite.w);

    //if (level_sprites[posToIndexY][posToIndexX].empty) // if we did't collide, assign the new position
        hero.sprite.transform.p = newpos;
}

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Cute ImGui", 0, 0, 640, 480, options);

	file_system_mount(file_system_get_base_dir(), "", 1);

	gfx_init(app);
    //gfx_init_upscale(app, 320, 240, GFX_UPSCALE_MAXIMUM_ANY);
	sb = sprite_batch_easy_make(app, "data");

	ImGuiContext* imgui_context = app_init_imgui(app);
	if (!imgui_context) {
		printf("Unable to initialize ImGui.\n");
		return -1;
	}
	ImGui::SetCurrentContext(imgui_context);
    
    int vcount = sizeof(level1) / sizeof(level1[0]);
    level_sprites.ensure_count(vcount);
    level.data.ensure_count(vcount);
    LoadLevel(level1, vcount);

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
