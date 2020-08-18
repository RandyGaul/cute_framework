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

spritebatch_t* sb;

struct Level
{
	int start_x;
	int start_y;
	int w, h;
	array<array<char>> data;
} level;
int level_index = 0;
bool loaded_level_into_editor = false;

sprite_t AddSprite(string_t path);

struct Animation
{
	string_t name;
	array<string_t>* frames;
	int frame = 0;
	float t = 0;
	float delay = 0.25f;

	sprite_t current_sprite() { return AddSprite(frames->operator[](frame)); }
};

Animation ice_block;
Animation ice_block_sheen;

struct Hero
{
	int x, y;
	bool initialized = false;
	int xdir = 0;
	int ydir = -1;
	bool holding = false;
	bool won = false;
    int moves = 0;

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

	Animation anim;
	dictionary<string_t, Animation> anims;
	void add_anim(Animation& a) { anims.insert(a.name, a); }
	void switch_anim(string_t name) { error_t err = anims.find(name, &anim); if (err.is_error()) CUTE_ASSERT(false); }
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

array<string_t> GirlLadder = {
	"data/girl_ladder1.png",
	"data/girl_ladder2.png",
	"data/girl_ladder3.png",
	"data/girl_ladder4.png",
	"data/girl_ladder5.png",
	"data/girl_ladder6.png",
	"data/girl_ladder7.png",
	"data/girl_ladder8.png",
	"data/girl_ladder9.png",
	"data/girl_ladder10.png",
	"data/girl_ladder11.png",
	"data/girl_ladder12.png",
	"data/girl_ladder13.png",
	"data/girl_ladder14.png",
	"data/girl_ladder15.png",
	"data/girl_ladder16.png",
	"data/girl_ladder17.png",
	"data/girl_ladder18.png",
	"data/girl_ladder19.png",
	"data/girl_ladder20.png",
	"data/girl_ladder21.png",
	"data/girl_ladder22.png",
	"data/girl_ladder23.png",
	"data/girl_ladder24.png",
	"data/girl_ladder25.png",
	"data/girl_ladder26.png",
	"data/girl_ladder27.png",
	"data/girl_ladder28.png",
	"data/girl_ladder29.png",
	"data/girl_ladder30.png",
	"data/girl_ladder31.png",
	"data/girl_ladder32.png",
	"data/girl_ladder33.png",
	"data/girl_ladder34.png",
	"data/girl_ladder35.png",
	"data/girl_ladder36.png",
	"data/girl_ladder37.png",
	"data/girl_ladder38.png",
	"data/girl_ladder39.png",
	"data/girl_ladder40.png",
	"data/girl_ladder41.png",
	"data/girl_ladder42.png",
	"data/girl_ladder43.png",
	"data/girl_ladder44.png",
};

array<string_t> IceBlockFrames = {
	"data/ice_block1.png",
	"data/ice_block2.png",
};

array<string_t> IceBlockSheen = {
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png", "data/ice_block_sheen1.png",
	"data/ice_block_sheen2.png",
	"data/ice_block_sheen3.png",
	"data/ice_block_sheen4.png",
	"data/ice_block_sheen5.png",
	"data/ice_block_sheen6.png",
	"data/ice_block_sheen7.png",
	"data/ice_block_sheen8.png",
	"data/ice_block_sheen9.png",
	"data/ice_block_sheen10.png",
	"data/ice_block_sheen11.png",
	"data/ice_block_sheen12.png",
	"data/ice_block_sheen13.png",
	"data/ice_block_sheen14.png",
	"data/ice_block_sheen15.png",
	"data/ice_block_sheen16.png",
	"data/ice_block_sheen17.png",
	"data/ice_block_sheen18.png",
	"data/ice_block_sheen19.png",
	"data/ice_block_sheen20.png",
	"data/ice_block_sheen21.png",
	"data/ice_block_sheen22.png",
	"data/ice_block_sheen23.png",
	"data/ice_block_sheen24.png",
	"data/ice_block_sheen25.png",
	"data/ice_block_sheen26.png",
	"data/ice_block_sheen27.png",
	"data/ice_block_sheen28.png",
	"data/ice_block_sheen29.png",
	"data/ice_block_sheen30.png",
	"data/ice_block_sheen31.png",
	"data/ice_block_sheen32.png",
	"data/ice_block_sheen33.png",
	"data/ice_block_sheen34.png",
	"data/ice_block_sheen35.png",
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

int sort_bits(sprite_t sprite)
{
	int x, y;
	world2tile(sprite.h, sprite.transform.p, &x, &y);
	return level.w * y + x;
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
		sprite_batch_push(sb, sprite);

		sprite = ice_block_sheen.current_sprite();
		sprite.transform.p = p_delta;
		sprite.sort_bits = sort_bits(sprite);
		sprite_batch_push(sb, sprite);
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
				sprite_batch_push(sb, sprite);
				break;

			case 'x':
				sprite = ice_block.current_sprite();
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.sort_bits = i * level.w + j;
				sprite_batch_push(sb, sprite);
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
				sprite_batch_push(sb, sprite);

				sprite = ice_block_sheen.current_sprite();
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.transform.p.y += hero.held_block.floating_offset;
				sprite.sort_bits = i * level.w + j;
				sprite_batch_push(sb, sprite);
			}	break;

			case 'e':
				sprite = AddSprite("data/ladder.png");
				sprite.transform.p = tile2world(sprite.h, j, i);
				sprite.sort_bits = i * level.w + j;
				sprite_batch_push(sb, sprite);
				break;

			default:
				break;

			case 'p':
				UpdateAnimation(hero.anim, dt);
				sprite = hero.anim.current_sprite();
				if (hero.xdir == 1 && hero.ydir == 0) sprite.scale_x *= -1;
				sprite.transform.p = tile2world(sprite.h, j, i) + v2(0, 2);
				sprite.sort_bits = i * level.w + j;
				sprite_batch_push(sb, sprite);
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
			sprite_batch_push(sb, sprite);
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
			sprite_batch_push(sb, sprite);
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
			sprite_batch_push(sb, sprite);
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
		sprite_batch_push(sb, sprite);
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
			sprite_batch_push(sb, sprite);
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

int main(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED | CUTE_APP_OPTIONS_D3D11_CONTEXT;
	app_t* app = app_make("Block Man", 0, 0, 960, 720, options);
	file_system_mount(file_system_get_base_dir(), "", 1);
	app_init_upscaling(app, UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 320, 240);
	ImGui::SetCurrentContext(app_init_imgui(app));

	sb = sprite_batch_easy_make(app, "data");

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

		UpdateGame(app, dt);
		UpdateAnimation(ice_block, dt);
		UpdateAnimation(ice_block_sheen, dt);

		sprite_batch_flush(sb);

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
