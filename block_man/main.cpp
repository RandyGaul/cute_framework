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
	bool initialized = false;
    int xdir = 0;
    int ydir = -1;
    bool holding = false;
} hero;

string_t level1_raw_data[] = {
	"111111111111111",
	"1p0100xx0000011",
	"100x00xx0010xx1",
    "111111111111xx1",
    "1000xxxxxxx0001",
    "101111111111111",
    "1000010x0010001",
    "100000x10000e01",
    "111111111111111",
};

v2 tile2world(float sprite_h, int x, int y)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = level.data.count() * h;
	float y_diff = sprite_h > 16 ? (sprite_h - h) / 2 : 0;
	return v2((float)(x-6) * w, -(float)(y+6) * h + y_offset + y_diff);
}

bool in_grid(int x, int y, int w, int h)
{
    return x >= 0 && y >= 0 && x < w && y < h;
}

sprite_t AddSprite(string_t path)
{
	sprite_t sprite;
	error_t err = sprite_batch_easy_sprite(sb, path.c_str(), &sprite);
	if (err.is_error()) {
		printf("Can't find file %s.\n", path.c_str());
		CUTE_ASSERT(false);
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
			char c = l[i][j];
			if (c == 'p') {
				CUTE_ASSERT(hero.initialized == false);
				hero.x = j;
				hero.y = i;
				hero.initialized = true;
			}
			level.data[i].add(c); // add everything into the level data
		}
	}
}

void DrawLevel(const Level& level)
{
	for (int i = 0; i < level.data.count(); ++i)
	{
		for (int j = 0; j < level.data[i].count(); ++j)
		{
			sprite_t sprite;
			bool empty = false;

			switch (level.data[i][j])
			{
            case '1':
                sprite = AddSprite("data/tile68.png");
                break;

            case 'x':
                sprite = AddSprite("data/ice_block.png");
                break;

            case 'c':
                sprite = AddSprite("data/tile4.png");
                break;

            case 'e':
                sprite = AddSprite("data/hero.png");
                break;

			case '0':
				empty = true;
				break;

			case 'p':
				sprite = AddSprite("data/girl_forward.png");
				break;
			}

			if (!empty) {
				sprite.transform.p = tile2world(sprite.scale_y, j, i);
				sprite_batch_push(sb, sprite);
			}
		}
	}
}

void HandleInput(app_t* app, float dt)
{
	int x = hero.x;
	int y = hero.y;

    /*if (key_was_pressed(app, KEY_R)) {
        x = level.start_x;
        y = level.start_y;
    }*/
    if (key_was_pressed(app, KEY_SPACE)) {

        if (!hero.holding)
        {
            // search forward from player to look for blocks to pick up
            int sx = x + hero.xdir, sy = y - hero.ydir;
            bool found = false;
            while (in_grid(sy, sx, level.data.count(), level.data[0].count()))
            {
                if (level.data[sy][sx] == 'x')
                {
                    found = true;
                    break;
                }
                else if (level.data[sy][sx] == '1')
                {
                    break;
                }
                sx += hero.xdir;
                sy -= hero.ydir;
            }
            if (found)
            {
                level.data[sy][sx] = '0';
                level.data[y - hero.ydir][x + hero.xdir] = 'c';
                hero.holding = true;
            }
        }
        else // hero.holding is true
        {
            // so we need to throw the block we are holding
            int sx = x + hero.xdir * 2, sy = y - hero.ydir * 2;
            while (in_grid(sy, sx, level.data.count(), level.data[0].count()))
            {
                if (level.data[sy][sx] != '0')
                    break;
                sx += hero.xdir;
                sy -= hero.ydir;
            }
            level.data[y - hero.ydir][x + hero.xdir] = '0';
            level.data[sy + hero.ydir][sx - hero.xdir] = 'x';
            hero.holding = false;
        }

    }

    key_button_t keycodes[4] = { KEY_W , KEY_S, KEY_D, KEY_A };
    int xdirs[4] = { 0 , 0, 1, -1 };
    int ydirs[4] = { 1, -1, 0, 0 };
    int xmove[4] = { 0, 0, 1, -1 };
    int ymove[4] = { -1, 1, 0, 0 };

    for (int i = 0; i < 4; ++i)
    {
        if (key_was_pressed(app, keycodes[i]))
        {
            if (hero.holding)
            {
                // if moving backwards, keep same direction, just back up
                if (hero.xdir == -xdirs[i] && hero.ydir == -ydirs[i])
                {
                    // make sure we don't push block through a wall
                    if (level.data[y + hero.ydir][x - hero.xdir] == '0')
                    {
                        // move backward
                        x += xmove[i];
                        y += ymove[i];

                        // update hero position
                        level.data[hero.y][hero.x] = '0';
                        hero.x = x;
                        hero.y = y;
                        level.data[y][x] = 'p';

                        // then, move the block
                        level.data[y - hero.ydir * 2][x + hero.xdir * 2] = '0';
                        level.data[y - hero.ydir][x + hero.xdir] = 'c';
                    }
                }
                // if moving forwards
                else if (hero.xdir == xdirs[i] && hero.ydir == ydirs[i])
                {
                    // make sure we don't push block through a wall
                    if (level.data[y - hero.ydir * 2][x + hero.xdir * 2] == '0')
                    {
                        // first, move the block
                        level.data[y - hero.ydir][x + hero.xdir] = '0';
                        level.data[y - hero.ydir * 2][x + hero.xdir * 2] = 'c';

                        // move forward
                        x += xmove[i];
                        y += ymove[i];

                        // update hero position
                        level.data[hero.y][hero.x] = '0';
                        hero.x = x;
                        hero.y = y;
                        level.data[y][x] = 'p';
                    }
                }
                else // if turning 90 degrees
                {
                    // turn
                    int xdirtemp = xdirs[i];
                    int ydirtemp = ydirs[i];

                    // check for something
                    if (level.data[y - ydirtemp][x + xdirtemp] == '0')
                    {
                        // also check to make sure we aren't turning through a block
                        if (level.data[y - hero.ydir - ydirtemp][x + hero.xdir + xdirtemp] == '0')
                        {
                            // remove block
                            level.data[y - hero.ydir][x + hero.xdir] = '0';

                            // turn hero
                            hero.xdir = xdirtemp;
                            hero.ydir = ydirtemp;

                            // and move block
                            level.data[y - hero.ydir][x + hero.xdir] = 'c';
                        }
                    }

                }
            }
            else // not holding a block
            {
                if (hero.xdir == xdirs[i] && hero.ydir == ydirs[i])
                {
                    // move forward
                    x += xmove[i];
                    y += ymove[i];
                    
                    // check for collisions
                    // if we did't collide, assign the new position
                    if (level.data[y][x] == '0') {
                        // update hero position
                        level.data[hero.y][hero.x] = '0';
                        hero.x = x;
                        hero.y = y;
                        level.data[y][x] = 'p';
                    }
                }
                else
                {
                    // turn 
                    hero.xdir = xdirs[i];
                    hero.ydir = ydirs[i];
                }
            }
        }
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
	level.data.ensure_count(vcount);
	LoadLevel(level1_raw_data, vcount);

	float t = 0;

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		HandleInput(app, dt);

		DrawLevel(level);

		sprite_batch_flush(sb);

		static bool hello_open = false;
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
