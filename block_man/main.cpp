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

#include <cute/cute_coroutine.h>

spritebatch_t* sb;

struct Level
{
	int start_x;
	int start_y;
	array<array<char>> data;
} level;

struct Animation
{
	string_t name;
	string_t* frames;
    int frame = 0;
    int frame_count = 0;
	float t = 0;
	float delay = 0.25f;
};

struct Hero
{
	int x, y;
	bool initialized = false;
    int xdir = 0;
    int ydir = -1;
    bool holding = false;

	Animation anim;
    dictionary<string_t, Animation> anims;
	void add_anim(Animation& a) { anims.insert(a.name, a); }
	void switch_anim(string_t name) { error_t err = anims.find(name, &anim); if (err.is_error()) CUTE_ASSERT(false); }
	string_t frame() { return anim.frames[anim.frame]; }
} hero;

string_t GirlForward[] = {
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png", // Hacky delay. Should have an array of delays.
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward.png", "data/girl_forward.png", "data/girl_forward.png",
    "data/girl_forward1.png",
    "data/girl_forward2.png",
    "data/girl_forward3.png",
    "data/girl_forward4.png",
    "data/girl_forward5.png",
    "data/girl_forward6.png",
    "data/girl_forward7.png",
    "data/girl_forward8.png",
    "data/girl_forward9.png",
    "data/girl_forward10.png",
    "data/girl_forward11.png",
    "data/girl_forward12.png",
    "data/girl_forward13.png",
    "data/girl_forward14.png",
    "data/girl_forward15.png",
    "data/girl_forward16.png",
};

string_t GirlHoldSide[] = {
    "data/girl_hold_side1.png",
    "data/girl_hold_side2.png",
};

string_t GirlHoldUp[] = {
    "data/girl_hold_up1.png",
    "data/girl_hold_up2.png",
};

string_t GirlHoldDown[] = {
    "data/girl_hold_down1.png",
    "data/girl_hold_down2.png",
};

string_t GirlSide = "data/girl_side.png";
string_t GirlUp = "data/girl_up.png";

string_t level1_raw_data[] = {
	"111111111111111",
	"1p0100xx0000011",
	"100x00xx0010xx1",
    "111111111111xx1",
    "1000xxxxxxx0001",
    "101111111111111",
    "1000010x0010e01",
    "100000x10000001",
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
	return sprite;
}

void UpdateAnimation(Animation& anim, float dt)
{
    if (anim.t >= anim.delay)
    {
        anim.t = 0; // reset

        // advance the animation
        if (anim.frame + 1 < anim.frame_count)
            anim.frame++;
        else
            anim.frame = 0;
    }
    else
        anim.t += dt;
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

void DrawLevel(const Level& level, float dt)
{
	static coroutine_t s_co;
	coroutine_t* co = &s_co;
	static float floating_offset = 0;

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
				sprite.transform.p = tile2world(sprite.scale_y, j, i);
                break;

            case 'x':
                sprite = AddSprite("data/ice_block.png");
				sprite.transform.p = tile2world(sprite.scale_y, j, i);
                break;

            case 'c':
			{
				float delay = 0.35f;
				COROUTINE_START(co);
				floating_offset = 0;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 1.0f;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 2.0f;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 1.0f;
				COROUTINE_WAIT(co, delay, dt);
				COROUTINE_END(co);

				sprite = AddSprite("data/ice_block.png");
				sprite.transform.p = tile2world(sprite.scale_y, j, i);
				sprite.transform.p.y += floating_offset;
			}	break;

            case 'e':
                sprite = AddSprite("data/ladder.png");
				sprite.transform.p = tile2world(sprite.scale_y, j, i);
                break;

			case '0':
				empty = true;
				break;

			case 'p':
                UpdateAnimation(hero.anim, dt);
                sprite = AddSprite(hero.frame());

                if (hero.xdir == 1 && hero.ydir == 0)
                {
                    sprite.scale_x *= -1;
                }

				sprite.transform.p = tile2world(sprite.scale_y, j, i);
				break;
			}

			if (!empty) {
				sprite_batch_push(sb, sprite);
			}
		}
	}
}

void SetHeroAnimBasedOnFacingDir()
{
	if (hero.holding) {
		if (hero.xdir) {
			hero.switch_anim("hold_side");
		} else {
			if (hero.ydir > 0) {
				hero.switch_anim("hold_up");
			} else {
				hero.switch_anim("hold_down");
			}
		}
	} else {
		if (hero.xdir) {
			hero.switch_anim("girl_side");
		} else if (hero.ydir) {
			if (hero.ydir > 0) {
				hero.switch_anim("girl_up");
			} else {
				hero.switch_anim("idle");
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
				SetHeroAnimBasedOnFacingDir();
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
			SetHeroAnimBasedOnFacingDir();
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
			bool update_hero_animation = false;

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

					update_hero_animation = true;
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
					update_hero_animation = true;
                }
            }

			if (update_hero_animation) SetHeroAnimBasedOnFacingDir();
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

    Animation idle;
	idle.name = "idle";
    idle.delay = 0.10f;
	idle.frames = GirlForward;
    idle.frame_count = sizeof(GirlForward) / sizeof(*GirlForward);

	Animation hold_side;
	hold_side.name = "hold_side";
    hold_side.delay = 0.10f;
	hold_side.frames = GirlHoldSide;
    hold_side.frame_count = sizeof(GirlHoldSide) / sizeof(*GirlHoldSide);

	Animation hold_up;
	hold_up.name = "hold_up";
    hold_up.delay = 0.10f;
	hold_up.frames = GirlHoldUp;
    hold_up.frame_count = sizeof(GirlHoldUp) / sizeof(*GirlHoldUp);

	Animation hold_down;
	hold_down.name = "hold_down";
    hold_down.delay = 0.10f;
	hold_down.frames = GirlHoldDown;
    hold_down.frame_count = sizeof(GirlHoldDown) / sizeof(*GirlHoldDown);

	Animation girl_side;
	girl_side.name = "girl_side";
    girl_side.delay = 0;
	girl_side.frames = &GirlSide;
    girl_side.frame_count = 1;

	Animation girl_up;
	girl_up.name = "girl_up";
    girl_up.delay = 0;
	girl_up.frames = &GirlUp;
    girl_up.frame_count = 1;

    hero.add_anim(idle);
    hero.add_anim(hold_side);
    hero.add_anim(hold_up);
    hero.add_anim(hold_down);
    hero.add_anim(girl_side);
    hero.add_anim(girl_up);
	hero.switch_anim("idle");

	float t = 0;

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		HandleInput(app, dt);

		DrawLevel(level, dt);

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
