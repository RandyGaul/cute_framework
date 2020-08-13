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
int level_index = 0;

struct Animation
{
	string_t name;
	array<string_t>* frames;
	int frame = 0;
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

	// ----------------------------
	// For animating between tiles.
	int x0, y0;
	bool moving = false;
	float move_t = 0;
	const float move_delay = 0.125f;
	// For animating between tiles.
	// ----------------------------

	struct HeldBlock
	{
		int x0, y0;
		int x, y;
	} held_block;

	struct RotatingBlock
	{
		bool is_rotating = false;
		int x, y;
		v2 v;
		float a;
		float t = 0;
		const float delay = 0.125f;
	} rotating_block;

	struct SlidingBlock
	{
		bool is_sliding = false;
		int x0, y0;
		int x, y;
		float t = 0;
		float delay;
		const float delay_per_tile = 0.125f;
	} sliding_block;

	Animation anim;
	dictionary<string_t, Animation> anims;
	void add_anim(Animation& a) { anims.insert(a.name, a); }
	void switch_anim(string_t name) { error_t err = anims.find(name, &anim); if (err.is_error()) CUTE_ASSERT(false); }
	string_t frame() { return (*anim.frames)[anim.frame]; }
} hero;

array<string_t> GirlForward = {
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

array<string_t> GirlHoldSide = {
	"data/girl_hold_side1.png",
	"data/girl_hold_side2.png",
};

array<string_t> GirlHoldUp = {
	"data/girl_hold_up1.png",
	"data/girl_hold_up2.png",
};

array<string_t> GirlHoldDown = {
	"data/girl_hold_down1.png",
	"data/girl_hold_down2.png",
};

array<string_t> GirlSide = { "data/girl_side.png" };
array<string_t> GirlUp = { "data/girl_up.png" };

array<string_t> GirlSpin = {
	"data/girl_spin1.png",
	"data/girl_spin2.png",
	"data/girl_spin3.png",
	"data/girl_spin4.png",
	"data/girl_spin5.png",
	"data/girl_spin6.png",
	"data/girl_spin7.png",
	"data/girl_spin8.png",
	"data/girl_spin9.png",
	"data/girl_spin10.png",
	"data/girl_spin11.png",
	"data/girl_spin12.png",
	"data/girl_spin13.png",
	"data/girl_spin14.png",
	"data/girl_spin15.png",
	"data/girl_spin16.png",
	"data/girl_spin17.png",
	"data/girl_spin18.png",
	"data/girl_spin19.png",
	"data/girl_spin20.png",
	"data/girl_spin21.png",
	"data/girl_spin22.png",
	"data/girl_spin23.png",
	"data/girl_spin24.png",
	"data/girl_spin25.png",
	"data/girl_spin26.png",
	"data/girl_spin27.png",
	"data/girl_spin28.png",
	"data/girl_spin29.png",
	"data/girl_spin30.png",
	"data/girl_spin31.png",
	"data/girl_spin32.png",
	"data/girl_spin33.png",
	"data/girl_spin34.png",
	"data/girl_spin35.png",
	"data/girl_spin36.png",
	"data/girl_spin37.png",
	"data/girl_spin38.png",
	"data/girl_spin39.png",
	"data/girl_spin40.png",
	"data/girl_spin41.png",
	"data/girl_spin42.png",
	"data/girl_spin43.png",
	"data/girl_spin44.png",
	"data/girl_spin45.png",
	"data/girl_spin46.png",
	"data/girl_spin47.png",
};

array<string_t> level1_raw_data = {
	"011110",
	"1p00x1",
	"100xx1",
	"10xxx1",
	"1xxxe1",
	"011110",
};

array<string_t> level2_raw_data = {
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

array<string_t> level3_raw_data = {
	"01110",
	"1px01",
	"10xe1",
	"01110",
};

array<string_t> level4_raw_data = {
	"01110",
	"1pxe1",
	"10x01",
	"01110",
};

array<string_t> level5_raw_data = {
	"01110",
	"1pxe1",
	"10x01",
	"10xx1",
	"01110",
};

array<string_t> level6_raw_data = {
	"01110",
	"1p001",
	"1x011",
	"1xxe1",
	"01110",
};

array<string_t> level7_raw_data = {
	"01110",
	"1p001",
	"1x001",
	"1x111",
	"1x0e1",
	"01110",
};

array<string_t> level8_raw_data = {
	"011110",
	"1p00x1",
	"1x00x1",
	"1111x1",
	"1e00x1",
	"011110",
};

array<array<string_t>> levels = {
	level1_raw_data,
	level2_raw_data,
	level3_raw_data,
	level4_raw_data,
	level5_raw_data,
	level6_raw_data,
	level7_raw_data,
	level8_raw_data,
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
		if (anim.frame + 1 < anim.frames->count())
			anim.frame++;
		else
			anim.frame = 0;
	}
	else
		anim.t += dt;
}

void LoadLevel(const array<string_t>& l)
{
	level.data.clear();
	level.data.ensure_count(l.count());

	for (int i = 0; i < l.count(); ++i)
	{
		const char* s = l[i].c_str();
		int len = l[i].len();

		for (int j = 0; j < len; ++j)
		{
			char c = s[j];
			if (c == 'p') {
				CUTE_ASSERT(hero.initialized == false);
				hero.x = j;
				hero.y = i;
				hero.initialized = true;
			}
			level.data[i].add(c);
		}
	}
}

void DrawAnimatingHeldBlocks()
{
	if (hero.moving && hero.holding) {
		// Calculate the t value for interpolating, and a y_offset.
		float y_offsets[5] = { 0, 1, 2, 1, 0 };
		float t = smoothstep(hero.move_t / hero.move_delay);
		int i = (int)(t * CUTE_ARRAY_SIZE(y_offsets));
		float y_offset = y_offsets[i];

		// Draw the held block.
		sprite_t sprite = AddSprite("data/ice_block.png");
		v2 p0 = tile2world((float)sprite.h, hero.held_block.x0, hero.held_block.y0);
		v2 p = tile2world((float)sprite.h, hero.held_block.x, hero.held_block.y);
		v2 p_delta = round(lerp(p0, p, t)) + v2(0, y_offset);
		sprite.transform.p = p_delta;
		sprite_batch_push(sb, sprite);
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
				floating_offset = 1;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 2.0f;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 3.0f;
				COROUTINE_WAIT(co, delay, dt);
				floating_offset = 2.0f;
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

			default:
				empty = true;
				break;

			case 'p':
				UpdateAnimation(hero.anim, dt);
				sprite = AddSprite(hero.frame());
				if (hero.xdir == 1 && hero.ydir == 0) sprite.scale_x *= -1;
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
		level.data.clear();
		LoadLevel(level1_raw_data, CUTE_ARRAY_SIZE(level1_raw_data));
	}*/
	if (key_was_pressed(app, KEY_SPACE)) {

		if (!hero.holding)
		{
			// search forward from player to look for blocks to pick up
			int sx = x + hero.xdir, sy = y - hero.ydir;
			bool found = false;
			int distance = 0;
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
				++distance;
			}

			if (found)
			{
				level.data[sy][sx] = '0';
				//level.data[y - hero.ydir][x + hero.xdir] = 'c';
				hero.sliding_block.is_sliding = true;
				hero.sliding_block.x0 = sx;
				hero.sliding_block.y0 = sy;
				hero.sliding_block.x = x + hero.xdir;
				hero.sliding_block.y = y - hero.ydir;
				hero.sliding_block.t = 0;
				hero.sliding_block.delay = hero.sliding_block.delay_per_tile * distance;
				hero.holding = true;
				SetHeroAnimBasedOnFacingDir();
			}
		}
		else // hero.holding is true
		{
			// so we need to throw the block we are holding
			int sx = x + hero.xdir * 2, sy = y - hero.ydir * 2;
			int distance = 0;
			while (in_grid(sy, sx, level.data.count(), level.data[0].count()))
			{
				if (level.data[sy][sx] != '0')
					break;
				sx += hero.xdir;
				sy -= hero.ydir;
				++distance;
			}
			level.data[y - hero.ydir][x + hero.xdir] = '0';
			//level.data[sy + hero.ydir][sx - hero.xdir] = 'x';
			hero.sliding_block.is_sliding = true;
			hero.sliding_block.x0 = x + hero.xdir;
			hero.sliding_block.y0 = y - hero.ydir;
			hero.sliding_block.x = sx - hero.xdir;
			hero.sliding_block.y = sy + hero.ydir;
			hero.sliding_block.t = 0;
			hero.sliding_block.delay = hero.sliding_block.delay_per_tile * distance;
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
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;
						level.data[y][x] = 'P'; // Big 'P' means player animating between tiles now.

						// then, move the block
						level.data[y - hero.ydir * 2][x + hero.xdir * 2] = '0';
						//level.data[y - hero.ydir][x + hero.xdir] = 'c';
						hero.held_block.x0 = x + hero.xdir * 2;
						hero.held_block.y0 = y - hero.ydir * 2;
						hero.held_block.x = x + hero.xdir;
						hero.held_block.y = y - hero.ydir;
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
						//level.data[y - hero.ydir * 2][x + hero.xdir * 2] = 'c';
						hero.held_block.x0 = x + hero.xdir;
						hero.held_block.y0 = y - hero.ydir;
						hero.held_block.x = x + hero.xdir * 2;
						hero.held_block.y = y - hero.ydir * 2;

						// move forward
						x += xmove[i];
						y += ymove[i];

						// update hero position
						level.data[hero.y][hero.x] = '0';
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;
						level.data[y][x] = 'P'; // Big 'P' means player animating between tiles now.
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
							hero.rotating_block.v = v2((float)hero.xdir, (float)hero.ydir);

							// turn hero
							hero.xdir = xdirtemp;
							hero.ydir = ydirtemp;

							// and move block
							//level.data[y - hero.ydir][x + hero.xdir] = 'c';
							hero.rotating_block.x = x + hero.xdir;
							hero.rotating_block.y = y - hero.ydir;
							hero.rotating_block.a = shortest_arc(hero.rotating_block.v, v2((float)hero.xdir, (float)hero.ydir));

							hero.rotating_block.is_rotating = true;
							hero.rotating_block.t = 0;
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
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;
						level.data[y][x] = 'P'; // Big 'P' means player animating between tiles now.
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

void UpdateGame(app_t* app, float dt)
{
	static coroutine_t s_co;
	coroutine_t* co = &s_co;

	COROUTINE_START(co);

	COROUTINE_CASE(co, update_game);
		HandleInput(app, dt);
		DrawLevel(level, dt);
		if (hero.moving) {
			goto hero_moving;
		} else if (hero.rotating_block.is_rotating) {
			goto hero_turning;
		} else if (hero.sliding_block.is_sliding) {
			goto sliding_block;
		}
	COROUTINE_YIELD(co);
	goto update_game;

	COROUTINE_CASE(co, hero_moving);
		hero.move_t += dt;

		if (hero.move_t < hero.move_delay) {
			// Animating the player from one tile to another.
			DrawLevel(level, dt);

			// Calculate the t value for interpolating, and a y_offset.
			float y_offsets[5] = { 0, 1, 2, 1, 0 };
			float t = smoothstep(hero.move_t / hero.move_delay);
			int i = (int)(t * CUTE_ARRAY_SIZE(y_offsets));
			float y_offset = y_offsets[i];

			// Draw the hero.
			UpdateAnimation(hero.anim, dt);
			DrawAnimatingHeldBlocks();
			sprite_t sprite = AddSprite(hero.frame());
			v2 p0 = tile2world((float)sprite.h, hero.x0, hero.y0);
			v2 p = tile2world((float)sprite.h, hero.x, hero.y);
			v2 p_delta = round(lerp(p0, p, t)) + v2(0, y_offset);
			sprite.transform.p = p_delta;
			if (hero.xdir == 1 && hero.ydir == 0) sprite.scale_x *= -1;
			sprite_batch_push(sb, sprite);
		} else {
			// Hero finished animating from one tile to another.
			hero.move_t = 0;
			hero.moving = false;
			level.data[hero.y][hero.x] = 'p';
			if (hero.holding) {
				level.data[hero.held_block.y][hero.held_block.x] = 'c';
			}
			DrawLevel(level, dt);
			DrawAnimatingHeldBlocks();
			COROUTINE_YIELD(co);
			goto update_game;
		}
	COROUTINE_YIELD(co);
	goto hero_moving;

	COROUTINE_CASE(co, hero_turning);
		hero.rotating_block.t += dt;

		if (hero.rotating_block.t < hero.rotating_block.delay) {
			DrawLevel(level, dt);
			UpdateAnimation(hero.anim, dt);

			float t = smoothstep(hero.rotating_block.t / hero.rotating_block.delay);
			float a = lerp(0, hero.rotating_block.a, t);
			rotation_t r = make_rotation(a);
			v2 v = mul(r, hero.rotating_block.v) * 16.0f;
			sprite_t sprite = AddSprite("data/ice_block.png");
			sprite.transform.p = tile2world((float)sprite.h, hero.x, hero.y) + v;
			sprite_batch_push(sb, sprite);
		} else {
			hero.rotating_block.is_rotating = false;
			level.data[hero.rotating_block.y][hero.rotating_block.x] = 'c';
			DrawLevel(level, dt);
			UpdateAnimation(hero.anim, dt);
			COROUTINE_YIELD(co);
			goto update_game;
		}
	COROUTINE_YIELD(co);
	goto hero_turning;

	COROUTINE_CASE(co, sliding_block);
		hero.sliding_block.t += dt;

		if (hero.sliding_block.t < hero.sliding_block.delay) {
			DrawLevel(level, dt);
			float t = smoothstep(hero.sliding_block.t / hero.sliding_block.delay);
			sprite_t sprite = AddSprite("data/ice_block.png");
			v2 p0 = tile2world((float)sprite.h, hero.sliding_block.x0, hero.sliding_block.y0);
			v2 p = tile2world((float)sprite.h, hero.sliding_block.x, hero.sliding_block.y);
			v2 p_delta = round(lerp(p0, p, t)) + v2(0, 2);
			sprite.transform.p = p_delta;
			sprite_batch_push(sb, sprite);
		} else {
			level.data[hero.sliding_block.y][hero.sliding_block.x] = hero.holding ? 'c' : 'x';
			hero.sliding_block.is_sliding = false;
			DrawLevel(level, dt);
			COROUTINE_YIELD(co);
			goto update_game;
		}
	COROUTINE_YIELD(co);
	goto sliding_block;

	COROUTINE_END(co);
}

#include <vector>

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_RESIZABLE;
	app_t* app = app_make("Block Man", 0, 0, 640, 480, options);
	file_system_mount(file_system_get_base_dir(), "", 1);
	gfx_init(app);
	gfx_init_upscale(app, 320, 240, GFX_UPSCALE_MAXIMUM_ANY);
	ImGui::SetCurrentContext(app_init_imgui(app));

	sb = sprite_batch_easy_make(app, "data");

	LoadLevel(level7_raw_data);

	Animation idle;
	idle.name = "idle";
	idle.delay = 0.10f;
	idle.frames = &GirlForward;

	Animation hold_side;
	hold_side.name = "hold_side";
	hold_side.delay = 0.10f;
	hold_side.frames = &GirlHoldSide;

	Animation hold_up;
	hold_up.name = "hold_up";
	hold_up.delay = 0.10f;
	hold_up.frames = &GirlHoldUp;

	Animation hold_down;
	hold_down.name = "hold_down";
	hold_down.delay = 0.10f;
	hold_down.frames = &GirlHoldDown;

	Animation girl_side;
	girl_side.name = "girl_side";
	girl_side.delay = 0;
	girl_side.frames = &GirlSide;

	Animation girl_up;
	girl_up.name = "girl_up";
	girl_up.delay = 0;
	girl_up.frames = &GirlUp;

	Animation girl_spin;
	girl_spin.name = "girl_spin";
	girl_spin.delay = hero.move_delay / (float)GirlSpin.count();
	girl_spin.frames = &GirlSpin;

	hero.add_anim(idle);
	hero.add_anim(hold_side);
	hero.add_anim(hold_up);
	hero.add_anim(hold_down);
	hero.add_anim(girl_side);
	hero.add_anim(girl_up);
	hero.add_anim(girl_spin);
	hero.switch_anim("idle");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		UpdateGame(app, dt);

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
