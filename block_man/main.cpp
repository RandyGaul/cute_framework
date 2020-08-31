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

#include <cute.h>
using namespace cute;

#include <stdio.h>
#include <imgui/imgui.h>

#include <cute/cute_coroutine.h>

aseprite_cache_t* sprite_cache;

struct Level
{
	int start_x;
	int start_y;
	int w, h;
	array<array<char>> data;
} level;
int level_index = 0;
bool loaded_level_into_editor = false;

array<batch_quad_t> ice_block_masks;
sprite_t ice_block;
sprite_t ice_block_sheen;
array<batch_quad_t> background_bricks;

void IceBlockMask(sprite_t ice_block_sprite)
{
	batch_quad_t mask = ice_block_sprite.quad();
	uint64_t id = mask.id;
	mask.id = id;
	ice_block_masks.add(mask);
}

struct Hero
{
	int x, y;
	bool initialized = false;
	int xdir = 0;
	int ydir = -1;
	bool holding = false;
	bool won = false;
    int moves = 0;
	batch_quad_t reflection;
	sprite_t sprite;

	// ----------------------------
	// For animating between tiles.
	int x0, y0;
	bool moving = false;
	float move_t = 0;
	const float move_delay = 0.125f;
	// For animating between tiles.
	// ----------------------------

	// ----------------------------
	// For spinning upon level load.
	v2 spin_p0, spin_p;
	float spin_t = 0;
	float spin_delay;
	const float spin_delay_per_tile = 0.35f;
	// For spinning upon level load.
	// ----------------------------

	struct HeldBlock
	{
		int x0, y0;
		int x, y;
		float floating_offset = 0;
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

	void switch_anim(string_t name) { sprite.play(name); }
} hero;

array<string_t> BricksEven = {
	"data/bricks_even1.png",
	"data/bricks_even2.png",
	"data/bricks_even3.png",
	"data/bricks_even4.png",
	"data/bricks_even5.png",
	"data/bricks_even6.png",
};

array<string_t> BricksOdd = {
	"data/bricks_odd1.png",
	"data/bricks_odd2.png",
	"data/bricks_odd3.png",
	"data/bricks_odd4.png",
	"data/bricks_odd5.png",
	"data/bricks_odd6.png",
};

array<array<string_t>> BackgroundMaps = {
	{
		"XXXX0000XXXX00000XXX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"00000000000XXX0000XX",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXXX0X0XX00XXXXX00X0",
		"XXX0000XX000XXX0000X",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
	{
		"00000000000XXX0000XX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"XXXX0X0XX00XXXXX00X0",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"XXXX0000XXXX00000XXX",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX000XXX0000X",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
};

array<array<string_t>> levels = {
	{
		"01111110",
		"1p0x0001",
		"01101110",
		"01101110",
		"100x0001",
		"1000xxx1",
		"1000xex1",
		"1000xxx1",
		"01111110",
	},
	{
		"00000010",
		"000001p1",
		"00000101",
		"011111x1",
		"1exxx001",
		"1xxx0001",
		"1xx00001",
		"1x000001",
		"01111110",
	},
	{
		"011110",
		"1e0x01",
		"10x0p1",
		"1x0001",
		"011110",
	},
	{
		"011100",
		"1px010",
		"1xxx01",
		"10xe10",
		"011100",
	},
	{ // 12
		"01110",
		"1px01",
		"10xe1",
		"01110",
	},
	{ // 13
		"01110",
		"1p001",
		"1x011",
		"1xxe1",
		"01110",
	},
	{ // 26
		"01110",
		"1p001",
		"1x001",
		"1x111",
		"1x0e1",
		"01110",
	},
	{ // 28
		"011110",
		"1p00x1",
		"1x00x1",
		"1111x1",
		"1e00x1",
		"011110",
	},
	{ // 23
		"011110",
		"1p00x1",
		"100xx1",
		"10xxx1",
		"1xxxe1",
		"011110",
	},
	{ // 44
		"01111110",
		"1100xx11",
		"1p001xe1",
		"1100xx11",
		"01111110",
	},
	{ // 39
		"011110",
		"1p0001",
		"1xxxx1",
		"1xx101",
		"1000e1",
		"011110",
	},
	{ // 57
		"111111111",
		"1p0011111",
		"10xxxxxx1",
		"1000111e1",
		"111111111",
	},
};

v2 tile2world(int sprite_h, int x, int y)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = level.data.count() * h;
	float y_diff = sprite_h > 16 ? (sprite_h - h) / 2 : 0;
	return v2((float)(x-6) * w, -(float)(y+6) * h + y_offset + y_diff);
}

void world2tile(int sprite_h, v2 p, int* x_out, int* y_out)
{
	float w = 16; // width of tiles in pixels
	float h = 16;
	float y_offset = level.data.count() * h;
	float y_diff = sprite_h > 16 ? (sprite_h - h) / 2 : 0;
	float x = p.x / w + 6;
	float y = -((p.y - y_offset - y_diff) / h) - 6;
	*x_out = (int)round(x);
	*y_out = (int)round(y);
}

int sort_bits(batch_quad_t q)
{
	int x, y;
	world2tile(q.h, q.transform.p, &x, &y);
	return level.w * y + x;
}

bool in_grid(int x, int y, int w, int h)
{
	return x >= 0 && y >= 0 && x < w && y < h;
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

void InitBackgroundBricks(int seed)
{
	rnd_t rnd = rnd_seed(seed);
	int background_index = rnd_next_range(rnd, 0, BackgroundMaps.count() - 1);

	background_bricks.clear();
	for (int i = 0; i < 15; ++i) {
		for (int j = 0; j < 20; ++j) {
			string_t path;
			if (BackgroundMaps[background_index][i][j] == 'X') {
				if ((i & 1) ^ (j & 1)) {
					path = BricksEven[rnd_next_range(rnd, 0, BricksEven.count() - 1)];
				} else {
					path = BricksOdd[rnd_next_range(rnd, 0, BricksOdd.count() - 1)];
				}
			} else {
				path = "data/brick_empty.png";
			}
			sprite_t s = AddSprite(path);
			s.transform.p = v2((float)(j * 16 + 8 - 320/2), (float)((15 - 1 - i) * 16 + 8 - 240/2));
			background_bricks.add(s);
		}
	}
}

void DrawBackgroundBricks()
{
	for (int i = 0; i < background_bricks.count(); ++i) {
		spritebatch_push(sb, background_bricks[i]);
	}
	spritebatch_flush(sb);
}

void LoadLevel(const array<string_t>& l)
{
	level.data.clear();
	level.data.ensure_count(l.count());
	level.w = l[0].len();
	level.h = l.count();
	hero.initialized = false;
	hero.moves = 0;
	loaded_level_into_editor = false;

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
				hero.xdir = 0;
				hero.ydir = -1;
				hero.holding = 0;
				hero.moving = 0;
				hero.initialized = true;
				hero.won = false;
				hero.rotating_block.is_rotating = false;
				hero.sliding_block.is_sliding = false;
				SetHeroAnimBasedOnFacingDir();
			}
			level.data[i].add(c);
		}
	}

	InitBackgroundBricks(level_index);
}

void DrawAnimatingHeldBlocks()
{
	if (hero.moving && hero.holding) {
		float t = smoothstep(hero.move_t / hero.move_delay);
		sprite_t sprite = ice_block.current_sprite();
		v2 p0 = tile2world(sprite.h, hero.held_block.x0, hero.held_block.y0);
		v2 p = tile2world(sprite.h, hero.held_block.x, hero.held_block.y);
		v2 p_delta = round(lerp(p0, p, t)) + v2(0, hero.held_block.floating_offset);
		sprite.transform.p = p_delta;
		sprite.sort_bits = sort_bits(sprite);
		IceBlockMask(sprite);
		spritebatch_push(sb, sprite);

		sprite = ice_block_sheen.current_sprite();
		sprite.transform.p = p_delta;
		sprite.sort_bits = sort_bits(sprite);
		spritebatch_push(sb, sprite);
	}
}

void DrawLevel(const Level& level, float dt)
{
	static coroutine_t s_co;
	coroutine_t* co = &s_co;

	for (int i = 0; i < level.data.count(); ++i)
	{
		for (int j = 0; j < level.data[i].count(); ++j)
		{
			sprite_t sprite;

			switch (level.data[i][j])
			{
			case '1':
				sprite = AddSprite("data/tile68.png");
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.sort_bits = i * level.w + j;
				spritebatch_push(sb, sprite);
				break;

			case 'x':
				sprite = ice_block.current_sprite();
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.sort_bits = i * level.w + j;
				IceBlockMask(sprite);
				spritebatch_push(sb, sprite);
				break;

			case 'c':
			{
				float delay = 0.35f;
				COROUTINE_START(co);
				hero.held_block.floating_offset = 1.0f;
				COROUTINE_PAUSE(co, delay, dt);
				hero.held_block.floating_offset = 2.0f;
				COROUTINE_PAUSE(co, delay, dt);
				hero.held_block.floating_offset = 3.0f;
				COROUTINE_PAUSE(co, delay, dt);
				hero.held_block.floating_offset = 2.0f;
				COROUTINE_PAUSE(co, delay, dt);
				COROUTINE_END(co);

				sprite = ice_block.current_sprite();
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.transform.p.y += hero.held_block.floating_offset;
				sprite.sort_bits = i * level.w + j;
				spritebatch_push(sb, sprite);

				sprite = ice_block_sheen.current_sprite();
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.transform.p.y += hero.held_block.floating_offset;
				sprite.sort_bits = i * level.w + j;
				IceBlockMask(sprite);
				spritebatch_push(sb, sprite);
			}	break;

			case 'e':
				sprite = AddSprite("data/ladder.png");
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.sort_bits = i * level.w + j;
				spritebatch_push(sb, sprite);
				break;

			default:
				break;

			case 'p':
				UpdateAnimation(hero.anim, dt);
				sprite = hero.anim.current_sprite();
				if (hero.xdir == 1 && hero.ydir == 0) sprite.scale_x *= -1;
				sprite.transform.p = tile2world(sprite.h, j, i) + v2(0, 2);
				sprite.sort_bits = i * level.w + j;
				hero.reflection = sprite;
				spritebatch_push(sb, sprite);
				break;
			}
		}
	}
}

void HandleInput(app_t* app, float dt)
{
	int x = hero.x;
	int y = hero.y;

	if (key_was_pressed(app, KEY_R)) {
		LoadLevel(levels[level_index]);
	}

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
                ++hero.moves;
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
            ++hero.moves;
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
			ice_block_sheen.t = 0;
			ice_block_sheen.frame = 0;
			SetHeroAnimBasedOnFacingDir();
		}
	}

	key_button_t keycodes[4] = { KEY_W , KEY_S, KEY_D, KEY_A };
	key_button_t keycodes_arrows[4] = { KEY_UP , KEY_DOWN, KEY_RIGHT, KEY_LEFT };
	int xdirs[4] = {  0,  0,  1, -1 };
	int ydirs[4] = {  1, -1,  0,  0 };
	int xmove[4] = {  0,  0,  1, -1 };
	int ymove[4] = { -1,  1,  0,  0 };

	for (int i = 0; i < 4; ++i)
	{
		if (key_was_pressed(app, keycodes[i]) || key_was_pressed(app, keycodes_arrows[i]))
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
                        ++hero.moves;

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
                        ++hero.moves;

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
                            ++hero.moves;

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
					if (level.data[y][x] == '0' || level.data[y][x] == 'e') {
                        ++hero.moves;

						bool won = level.data[y][x] == 'e';

						// update hero position
						level.data[hero.y][hero.x] = '0';
						hero.x0 = hero.x;
						hero.y0 = hero.y;
						hero.x = x;
						hero.y = y;
						hero.moving = true;

						if (won) {
							hero.won = true;
						}
					}
				}
				else
				{
                    ++hero.moves;
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
	{
		HandleInput(app, dt);
		DrawLevel(level, dt);
		if (hero.moving) {
			goto hero_moving;
		} else if (hero.rotating_block.is_rotating) {
			goto hero_turning;
		} else if (hero.sliding_block.is_sliding) {
			goto sliding_block;
		}
	}
	COROUTINE_YIELD(co);
	goto update_game;

	COROUTINE_CASE(co, hero_moving);
	{
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
			sprite_t sprite = hero.anim.current_sprite();
			v2 p0 = tile2world(sprite.h, hero.x0, hero.y0);
			v2 p = tile2world(sprite.h, hero.x, hero.y);
			v2 p_delta = round(lerp(p0, p, t)) + v2(0, y_offset);
			sprite.transform.p = p_delta + v2(0, 2);
			if (hero.xdir == 1 && hero.ydir == 0) sprite.scale_x *= -1;
			sprite.sort_bits = sort_bits(sprite);
			hero.reflection = sprite;
			spritebatch_push(sb, sprite);
		} else {
			// Hero finished animating from one tile to another.
			hero.move_t = 0;
			hero.moving = false;
			if (!hero.won) {
				level.data[hero.y][hero.x] = 'p';
				if (hero.holding) {
					level.data[hero.held_block.y][hero.held_block.x] = 'c';
				}
			} else {
				goto hero_won;
			}
			DrawLevel(level, dt);
			DrawAnimatingHeldBlocks();
			COROUTINE_YIELD(co);
			goto update_game;
		}
	}
	COROUTINE_YIELD(co);
	goto hero_moving;

	COROUTINE_CASE(co, hero_turning);
	{
		hero.rotating_block.t += dt;

		if (hero.rotating_block.t < hero.rotating_block.delay) {
			DrawLevel(level, dt);
			UpdateAnimation(hero.anim, dt);

			float t = smoothstep(hero.rotating_block.t / hero.rotating_block.delay);
			float a = lerp(0, hero.rotating_block.a, t);
			rotation_t r = make_rotation(a);
			v2 v = mul(r, hero.rotating_block.v) * 16.0f;
			sprite_t sprite = ice_block.current_sprite();
			sprite.transform.p = tile2world(sprite.h, hero.x, hero.y) + v;
			sprite.sort_bits = sort_bits(sprite);
			IceBlockMask(sprite);
			spritebatch_push(sb, sprite);
		} else {
			hero.rotating_block.is_rotating = false;
			level.data[hero.rotating_block.y][hero.rotating_block.x] = 'c';
			DrawLevel(level, dt);
			UpdateAnimation(hero.anim, dt);
			COROUTINE_YIELD(co);
			goto update_game;
		}
	}
	COROUTINE_YIELD(co);
	goto hero_turning;

	COROUTINE_CASE(co, sliding_block);
	{
		hero.sliding_block.t += dt;

		if (hero.sliding_block.t < hero.sliding_block.delay) {
			DrawLevel(level, dt);
			float t = smoothstep(hero.sliding_block.t / hero.sliding_block.delay);
			sprite_t sprite = ice_block.current_sprite();
			v2 p0 = tile2world(sprite.h, hero.sliding_block.x0, hero.sliding_block.y0);
			v2 p = tile2world(sprite.h, hero.sliding_block.x, hero.sliding_block.y);
			v2 p_delta = round(lerp(p0, p, t)) + v2(0, 2);
			sprite.transform.p = p_delta;
			sprite.sort_bits = sort_bits(sprite);
			IceBlockMask(sprite);
			spritebatch_push(sb, sprite);
		} else {
			level.data[hero.sliding_block.y][hero.sliding_block.x] = hero.holding ? 'c' : 'x';
			hero.sliding_block.is_sliding = false;
			DrawLevel(level, dt);
			COROUTINE_YIELD(co);
			goto update_game;
		}
	}
	COROUTINE_YIELD(co);
	goto sliding_block;

	COROUTINE_CASE(co, hero_won);
	hero.switch_anim("girl_ladder");

	COROUTINE_SEQUENCE_POINT(co);
	{
		DrawLevel(level, dt);
		UpdateAnimation(hero.anim, dt);
		sprite_t sprite = hero.anim.current_sprite();
		sprite.transform.p = tile2world(sprite.h, hero.x, hero.y);
		sprite.sort_bits = sort_bits(sprite);
		hero.reflection = sprite;
		spritebatch_push(sb, sprite);
		float delay = hero.anim.delay * hero.anim.frames->count();
		COROUTINE_WAIT(co, delay, dt);
		level_index = (level_index + 1) % levels.count();
		LoadLevel(levels[level_index]);
		goto hero_falling;
	}
	COROUTINE_YIELD(co);
	goto hero_won;

	COROUTINE_CASE(co, hero_falling);
	hero.switch_anim("girl_spin");
	level.data[hero.y][hero.x] = '0';
	hero.spin_p = tile2world(hero.anim.current_sprite().h, hero.x, hero.y);
	hero.spin_p0 = v2(hero.spin_p.x, 120 - 16);
	hero.spin_delay = (hero.spin_p0.y - hero.spin_p.y) * hero.spin_delay_per_tile * (1.0f / 16.0f);

		COROUTINE_SEQUENCE_POINT(co);
		{
			hero.spin_t += dt;
			float t = ease_out_sin(hero.spin_t / hero.spin_delay);

			// Animating the player from one tile to another.
			DrawLevel(level, dt);

			// Draw the hero.
			UpdateAnimation(hero.anim, dt);
			DrawAnimatingHeldBlocks();
			sprite_t sprite = hero.anim.current_sprite();
			v2 p_delta = round(lerp(hero.spin_p0, hero.spin_p, t));
			sprite.transform.p = p_delta + v2(0, 2);
			sprite.sort_bits = 100000;
			hero.reflection = sprite;
			spritebatch_push(sb, sprite);
			COROUTINE_WAIT(co, hero.spin_delay, dt);
		}

		hero.spin_t = 0;
		level.data[hero.y][hero.x] = 'p';
		hero.switch_anim("idle");
		COROUTINE_YIELD(co);
		goto update_game;
	COROUTINE_YIELD(co);
	goto update_game;

	COROUTINE_END(co);
}

void LoadLevelIntoEditor(char* buf)
{
	int index = 0;
	for (int i = 0; i < level.data.count(); ++i)
	{
		for (int j = 0; j < level.data[i].count(); ++j)
		{
			CUTE_ASSERT(index < 1024 * 10);
			char c = level.data[i][j];
			buf[index++] = c;
		}
		buf[index++] = '\n';
		CUTE_ASSERT(index < 1024 * 10);
	}
	buf[index++] = 0;
}

void DoImguiStuff(app_t* app, float dt)
{
	static bool open = true;
	if (key_was_pressed(app, KEY_E)) {
		open = true;
	}
	if (open) {
		ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_FirstUseEver);
		ImGui::Begin("Level Editor", &open);
		static char editor_buf[1024 * 10];
		if (!loaded_level_into_editor) {
			LoadLevelIntoEditor(editor_buf);
			loaded_level_into_editor = true;
		}
		int flags = ImGuiInputTextFlags_AllowTabInput;
		ImGui::Text("Level %d", level_index + 1);
		ImGui::InputTextMultiline("", editor_buf, 1024 * 10, ImVec2(0, 200), ImGuiInputTextFlags_AllowTabInput);
		if (ImGui::Button("Sync Editor Text")) {
			LoadLevelIntoEditor(editor_buf);
		}
		if (ImGui::Button("Commit")) {
			array<char> buf;
			levels[level_index].clear();
			int i = 0;
			char c;
			while ((c = editor_buf[i++])) {
				if (c == '\n') {
					buf.add(0);
					levels[level_index].add(buf.data());
					buf.clear();
				} else {
					buf.add(c);
				}
			}
			LoadLevel(levels[level_index]);
		}
		static bool copied = false;
		if (ImGui::Button("Copy to Clipboard")) {
			array<char> buf;
			int index = 0;
			char c;
			buf.add('\t');
			buf.add('{');
			buf.add('\n');
			buf.add('\t');
			buf.add('\t');
			buf.add('\"');
			while ((c = editor_buf[index++])) {
				if (c == '\n') {
					buf.add('\"');
					buf.add(',');
					buf.add('\n');
					buf.add('\t');
					buf.add('\t');
					buf.add('\"');
				} else {
					buf.add(c);
				}
			}
			buf.pop();
			buf.pop();
			buf.add('}');
			buf.add(',');
			buf.add('\n');
			buf.add(0);
			clipboard_set(buf.data());
			copied = true;
		}
		if (copied) {
			static coroutine_t s_co;
			coroutine_t* co = &s_co;

			COROUTINE_START(co);
			float delay = 0.5f;
			ImGui::SetNextWindowBgAlpha(1.0f - co->elapsed / delay);
			ImGui::BeginTooltip();
			ImGui::Text("Copied!");
			ImGui::EndTooltip();
			COROUTINE_WAIT(co, delay, dt);
			copied = false;
			COROUTINE_END(co);
		}
		ImGui::End();
	}
}

void DrawGirlReflection()
{
	sg_depth_stencil_state stencil;
	CUTE_MEMSET(&stencil, 0, sizeof(stencil));
	stencil.stencil_front.fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.depth_fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.pass_op = SG_STENCILOP_REPLACE;
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_ALWAYS;
	stencil.stencil_back = stencil.stencil_front;
	stencil.stencil_enabled = true;
	stencil.stencil_read_mask = 0xFF;
	stencil.stencil_write_mask = 0xFF;
	stencil.stencil_ref = 0x1;

	sg_blend_state blend = { 0 };
	blend.color_write_mask = SG_COLORMASK_NONE;

	// Draw masks onto the stencil buffer.
	spritebatch_set_depth_stencil_state(sb, stencil);
	spritebatch_set_blend_state(sb, blend);
	for (int i = 0; i < ice_block_masks.size(); ++i) {
		spritebatch_push(sb, ice_block_masks[i]);
	}
	ice_block_masks.clear();
	spritebatch_flush(sb);

	// Cutout the player from the masks.
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_ZERO;
	stencil.stencil_back = stencil.stencil_front;
	spritebatch_set_depth_stencil_state(sb, stencil);
	spritebatch_push(sb, hero.reflection);
	spritebatch_flush(sb);

	// Render girl over the masks.
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_KEEP;
	stencil.stencil_back = stencil.stencil_front;
	spritebatch_set_depth_stencil_state(sb, stencil);
	spritebatch_set_blend_defaults(sb);

	hero.reflection.transform.p += v2(0, 6);
	hero.reflection.alpha = 0.35f;
	spritebatch_push(sb, hero.reflection);
	spritebatch_flush(sb);

	spritebatch_set_depth_stencil_defaults(sb);
}

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_D3D11_CONTEXT;
	app_t* app = app_make("Block Man", 0, 0, 960, 720, options);
	file_system_mount(file_system_get_base_dir(), "", 1);
	app_init_upscaling(app, UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 320, 240);
	ImGui::SetCurrentContext(app_init_imgui(app));

	sb = spritebatch_easy_make(app, "data");

	size_t sz;
	void* data;
	file_system_read_entire_file_to_memory("data/girl.aseprite", &data, &sz, NULL);
	ase_t* ase = cute_aseprite_load_from_memory(data, (int)sz, NULL);
	cp_image_t img;
	img.w = ase->w;
	img.h = ase->h;
	img.pix = (cp_pixel_t*)ase->frames[0].pixels;
	cp_save_png("test.png", &img);

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
	girl_spin.delay = 1.0f / (float)GirlSpin.count();
	girl_spin.frames = &GirlSpin;

	Animation girl_ladder;
	girl_ladder.name = "girl_ladder";
	girl_ladder.delay = 0.085f;
	girl_ladder.frames = &GirlLadder;

	hero.add_anim(idle);
	hero.add_anim(hold_side);
	hero.add_anim(hold_up);
	hero.add_anim(hold_down);
	hero.add_anim(girl_side);
	hero.add_anim(girl_up);
	hero.add_anim(girl_spin);
	hero.add_anim(girl_ladder);
	hero.switch_anim("idle");

	ice_block.delay = 0.35f;
	ice_block.frames = &IceBlockFrames;
	ice_block.name = "ice_block";

	ice_block_sheen.delay = 0.0075f;
	ice_block_sheen.frames = &IceBlockSheen;
	ice_block_sheen.name = "ice_block_sheen";

	LoadLevel(levels[level_index]);

	matrix_t mvp = matrix_ortho_2d(320, 240, 0, -100);
	const font_t* font = font_get_default(app);
	float w = (float)font_text_width(font, "0000");
	float h = (float)font_text_height(font, "0000");

	while (app_is_running(app)) {
		float dt = calc_dt();
		app_update(app, dt);

		if (key_was_pressed(app, KEY_Q)) {
			// Cheat. For testing.
			level_index = (level_index + 1) % levels.count();
			LoadLevel(levels[level_index]);
		}

		DrawBackgroundBricks();

		UpdateGame(app, dt);
		UpdateAnimation(ice_block, dt);
		UpdateAnimation(ice_block_sheen, dt);

		spritebatch_flush(sb);

		DrawGirlReflection();

		char buffer[4];
		itoa(hero.moves, buffer, 10);
		font_push_verts(app, font, buffer, -w / 2, h / 2, 0);
		font_draw(app, font, mvp);

		DoImguiStuff(app, dt);

		app_present(app);
	}

	app_destroy(app);

	return 0;
}
