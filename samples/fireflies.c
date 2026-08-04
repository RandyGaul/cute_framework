/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Fireflies: a first-person forest at dusk, built from blocks.
//
// Wander a Zelda-ish woodland vale rendered Minecraft-style -- every tree, hill, and stone
// is a box -- until you find a glass jar resting on a stump. With the jar in hand, catch
// the fireflies drifting through the glades; the jar doubles as your lantern, glowing
// brighter with every catch, which matters in the deep thickets where the fireflies live.
// Carry your light to the dormant shrine in the great clearing and set them free: each
// release lights another lantern, and when all the lanterns wake, dawn comes early.
//
// Rendering showcase for the 3d API working as one system:
//   - Thousands of blocks flow through cf_draw3d_mesh and coalesce into a handful of
//     instanced draws (color + emissive ride in_mesh_attributes, sway rides the free
//     second lane).
//   - Two-cascade shadow maps rendered into the layers of one depth 2d-array texture
//     (CF_CanvasParams attach_target + attach_layer) and sampled with hardware PCF
//     through sampler2DArrayShadow.
//   - HDR bloom through render-to-mip canvases (CF_CanvasParams attach_mip): a bright
//     pass, then a downsample chain ping-ponging between the mip levels of two textures.
//   - Chunked frustum culling and all picking via cute_math3d (CF_Frustum, CF_Aabb3,
//     cf_ray3_to_sphere).
//
// Controls: WASD move, mouse look, E interact (pick up the jar / release at the shrine),
// left click to catch a firefly near the crosshair.
//
// Automated testing: every control flows through one Input struct, so `fireflies --auto`
// plays a scripted tour of the whole loop with no human input, `--shot <seconds>`
// (repeatable) saves numbered screenshots, and `--exit-at <seconds>` quits cleanly. The
// sample can smoke-test itself in CI, or under a pair of watchful robot eyes.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------
// Tuning.

#define WORLD_N 96          // Columns per side.
#define CHUNK_N 8           // Columns per chunk side.
#define CHUNKS (WORLD_N / CHUNK_N)
#define MAX_BLOCKS_PER_CHUNK 512
#define FIREFLY_COUNT 44
#define FIREFLY_LIGHTS 12   // Nearest lights that reach the shader (jar takes one slot).
#define SHRINE_LANTERNS 5   // Fireflies needed to wake the shrine.
#define EYE_HEIGHT 1.65f
#define SHADOW_RES 1024 // EVSM blurs, so 1024 out-resolves what raw 2048 PCF managed.
#define BLOOM_LEVELS 5

//--------------------------------------------------------------------------------------------------
// Deterministic little PRNG so every run grows the same forest.

static uint64_t s_rng = 0xF17EF11E5ull;
static uint32_t rnd(void)
{
	uint64_t x = s_rng;
	x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
	s_rng = x;
	return (uint32_t)((x * 0x2545F4914F6CDD1Dull) >> 32);
}
static float rndf(void) { return (float)(rnd() & 0xFFFFFF) / (float)0xFFFFFF; }
static float rndf_range(float lo, float hi) { return lo + (hi - lo) * rndf(); }
static int clampi(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }

//--------------------------------------------------------------------------------------------------
// Value noise for the terrain: smooth rolling hills out of a tiny hash.

static float s_hash2(int x, int z)
{
	uint32_t h = (uint32_t)(x * 374761393 + z * 668265263) ^ 0x9E3779B9u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}

static float s_value_noise(float x, float z)
{
	int x0 = (int)floorf(x), z0 = (int)floorf(z);
	float fx = x - x0, fz = z - z0;
	fx = fx * fx * (3.0f - 2.0f * fx);
	fz = fz * fz * (3.0f - 2.0f * fz);
	float a = s_hash2(x0, z0), b = s_hash2(x0 + 1, z0);
	float c = s_hash2(x0, z0 + 1), d = s_hash2(x0 + 1, z0 + 1);
	return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fz;
}

static float s_terrain_height(float x, float z)
{
	float h = 0;
	h += s_value_noise(x * 0.030f, z * 0.030f) * 6.0f;
	h += s_value_noise(x * 0.085f, z * 0.085f) * 2.0f;
	float cx = x - WORLD_N * 0.5f, cz = z - WORLD_N * 0.5f;
	float r = sqrtf(cx * cx + cz * cz);
	// A broad flat clearing in the middle for the shrine.
	float clearing = 1.0f - cf_clamp(1.0f - r / 14.0f, 0, 1);
	h *= 0.35f + 0.65f * clearing;
	// The vale: the land rises into hills toward the edges, closing the horizon.
	float edge = cf_clamp((r - 34.0f) / 14.0f, 0, 1);
	h += edge * edge * 14.0f;
	return h;
}

//--------------------------------------------------------------------------------------------------
// The world: static blocks baked into chunks at startup, culled per frame by chunk AABB.

typedef struct Block
{
	CF_V3 center;
	CF_V3 scale;
	CF_Color color;
	float emissive;  // Rides in_mesh_attributes.w.
	float sway;      // Rides the second lane; leaves shimmer slightly.
} Block;

typedef struct Chunk
{
	Block blocks[MAX_BLOCKS_PER_CHUNK];
	int count;
	CF_Aabb3 bounds;
} Chunk;

typedef struct World
{
	Chunk chunks[CHUNKS * CHUNKS];
	int column_h[WORLD_N][WORLD_N];
} World;

static World g_world;

static void s_add_block(CF_V3 center, CF_V3 scale, CF_Color color, float emissive, float sway)
{
	int cx = clampi((int)(center.x / CHUNK_N), 0, CHUNKS - 1);
	int cz = clampi((int)(center.z / CHUNK_N), 0, CHUNKS - 1);
	Chunk* chunk = &g_world.chunks[cz * CHUNKS + cx];
	if (chunk->count >= MAX_BLOCKS_PER_CHUNK) return;
	Block* b = &chunk->blocks[chunk->count++];
	b->center = center;
	b->scale = scale;
	b->color = color;
	b->emissive = emissive;
	b->sway = sway;
	CF_Aabb3 bb = cf_make_aabb3_center(center, cf_mul_v3_f(scale, 0.5f));
	if (chunk->count == 1) chunk->bounds = bb;
	else chunk->bounds = cf_combine_aabb3(chunk->bounds, bb);
}

static float s_ground(float x, float z)
{
	int ix = clampi((int)x, 0, WORLD_N - 1);
	int iz = clampi((int)z, 0, WORLD_N - 1);
	return (float)g_world.column_h[iz][ix] + 1.0f;
}

// Trunk positions for a soft radial push-out, so the camera never walks through a tree.
#define MAX_TRUNKS 512
static CF_V2 g_trunks[MAX_TRUNKS];
static int g_trunk_count;

static void s_make_tree(float x, float z, float ground)
{
	int trunk_h = 5 + (int)(rndf() * 4.0f);
	float bark_g = rndf_range(0.85f, 1.1f);
	CF_Color bark = cf_make_color_rgb_f(0.33f * bark_g, 0.23f * bark_g, 0.15f * bark_g);
	s_add_block(cf_v3(x, ground + trunk_h * 0.5f, z), cf_v3(0.5f, (float)trunk_h, 0.5f), bark, 0, 0);
	if (g_trunk_count < MAX_TRUNKS) g_trunks[g_trunk_count++] = cf_v2(x, z);

	// Canopy: a chunky cluster of leaf boxes seated on the trunk's shoulders, with sky
	// gaps between neighboring trees. Deep forest greens with hue drift per puff.
	int puffs = 4 + (int)(rndf() * 3.0f);
	float canopy_y = ground + trunk_h - 0.8f;
	for (int i = 0; i < puffs; ++i) {
		float s = rndf_range(2.2f, 3.6f);
		CF_V3 off = cf_v3(rndf_range(-1.4f, 1.4f), rndf_range(0.0f, 2.0f), rndf_range(-1.4f, 1.4f));
		float g = rndf_range(0.65f, 1.05f);
		float hue = rndf();
		CF_Color leaf = cf_make_color_rgb_f(
			(0.06f + hue * 0.05f) * g,
			(0.23f + hue * 0.06f) * g,
			(0.11f + (1.0f - hue) * 0.07f) * g);
		s_add_block(cf_add_v3(cf_v3(x, canopy_y + s * 0.35f, z), off), cf_v3(s, s * 0.75f, s), leaf, 0, 1.0f);
	}
}

// The stump the jar rests on, in a glade east of spawn.
static CF_V3 g_jar_pos;
static CF_V3 g_shrine_pos;

static void s_build_world(void)
{
	for (int z = 0; z < WORLD_N; ++z) {
		for (int x = 0; x < WORLD_N; ++x) {
			g_world.column_h[z][x] = (int)s_terrain_height((float)x, (float)z);
		}
	}
	for (int z = 0; z < WORLD_N; ++z) {
		for (int x = 0; x < WORLD_N; ++x) {
			int h = g_world.column_h[z][x];
			int lo = h;
			if (x > 0) lo = cf_min(lo, g_world.column_h[z][x - 1]);
			if (x < WORLD_N - 1) lo = cf_min(lo, g_world.column_h[z][x + 1]);
			if (z > 0) lo = cf_min(lo, g_world.column_h[z - 1][x]);
			if (z < WORLD_N - 1) lo = cf_min(lo, g_world.column_h[z + 1][x]);
			float top = h + 1.0f;
			float bottom = (float)(lo - 1);
			float g = 0.85f + 0.15f * s_hash2(x * 7 + 3, z * 13 + 1);
			float warm = s_value_noise(x * 0.11f, z * 0.11f);
			CF_Color grass = cf_make_color_rgb_f(
				(0.11f + 0.05f * warm) * g,
				(0.26f + 0.06f * warm) * g,
				(0.14f + 0.02f * (1.0f - warm)) * g);
			s_add_block(
				cf_v3(x + 0.5f, (top + bottom) * 0.5f, z + 0.5f),
				cf_v3(1.0f, top - bottom, 1.0f),
				grass, 0, 0);
		}
	}

	// Trees: scattered by noise, never in the clearing, breathing room between them.
	for (int z = 3; z < WORLD_N - 3; z += 2) {
		for (int x = 3; x < WORLD_N - 3; x += 2) {
			float cx = x - WORLD_N * 0.5f, cz = z - WORLD_N * 0.5f;
			if (sqrtf(cx * cx + cz * cz) < 13.0f) continue;
			float density = s_value_noise(x * 0.07f + 40.0f, z * 0.07f + 40.0f);
			if (density > 0.58f && rndf() > 0.52f) {
				float fx = x + rndf_range(-0.6f, 0.6f);
				float fz = z + rndf_range(-0.6f, 0.6f);
				s_make_tree(fx, fz, s_ground(fx, fz));
			}
		}
	}

	// Stones.
	for (int i = 0; i < 50; ++i) {
		float x = rndf_range(4.0f, WORLD_N - 4.0f);
		float z = rndf_range(4.0f, WORLD_N - 4.0f);
		float s = rndf_range(0.4f, 1.1f);
		float g = rndf_range(0.40f, 0.55f);
		s_add_block(cf_v3(x, s_ground(x, z) + s * 0.3f, z), cf_v3(s, s * 0.7f, s),
			cf_make_color_rgb_f(g, g, g * 1.08f), 0, 0);
	}

	// The jar's stump, in a glade a short wander from spawn.
	{
		float x = WORLD_N * 0.5f + 16.0f, z = WORLD_N * 0.5f - 14.0f;
		float ground = s_ground(x, z);
		s_add_block(cf_v3(x, ground + 0.35f, z), cf_v3(0.9f, 0.7f, 0.9f),
			cf_make_color_rgb_f(0.30f, 0.23f, 0.17f), 0, 0);
		g_jar_pos = cf_v3(x, ground + 0.95f, z);
	}

	// The shrine: a ring of stone pillars around a central altar in the clearing.
	{
		float x = WORLD_N * 0.5f, z = WORLD_N * 0.5f;
		float ground = s_ground(x, z);
		g_shrine_pos = cf_v3(x, ground + 1.2f, z);
		CF_Color stone = cf_make_color_rgb_f(0.38f, 0.40f, 0.46f);
		s_add_block(cf_v3(x, ground + 0.5f, z), cf_v3(2.2f, 1.0f, 2.2f), stone, 0, 0);   // Altar base.
		s_add_block(cf_v3(x, ground + 1.15f, z), cf_v3(1.4f, 0.35f, 1.4f), stone, 0, 0); // Altar top.
		for (int i = 0; i < SHRINE_LANTERNS; ++i) {
			float ang = (float)i / SHRINE_LANTERNS * CF_TAU;
			float px = x + cosf(ang) * 5.5f;
			float pz = z + sinf(ang) * 5.5f;
			float pg = s_ground(px, pz);
			s_add_block(cf_v3(px, pg + 1.4f, pz), cf_v3(0.8f, 2.8f, 0.8f), stone, 0, 0);
			// The lantern cube sits on top; it lights up as fireflies are delivered
			// (drawn dynamically, not baked -- see s_draw_dynamic).
		}
	}
}

//--------------------------------------------------------------------------------------------------
// Fireflies.

typedef struct Firefly
{
	CF_V3 home;
	float phase[3];
	float speed;
	CF_V3 pos;
	bool caught;    // In the jar.
	bool delivered; // Released at the shrine.
	float release_t;
} Firefly;

static Firefly g_fireflies[FIREFLY_COUNT];

static void s_spawn_fireflies(void)
{
	CF_V3 glades[4];
	int n_glades = 4;
	for (int i = 0; i < n_glades; ++i) {
		float ang = (float)i / n_glades * CF_TAU + 0.5f + rndf() * 0.6f;
		float r = rndf_range(18.0f, 30.0f);
		float x = WORLD_N * 0.5f + cosf(ang) * r;
		float z = WORLD_N * 0.5f + sinf(ang) * r;
		glades[i] = cf_v3(x, 0, z);
	}
	for (int i = 0; i < FIREFLY_COUNT; ++i) {
		CF_V3 g = glades[i % n_glades];
		float x = cf_clamp(g.x + rndf_range(-6.0f, 6.0f), 4.0f, WORLD_N - 4.0f);
		float z = cf_clamp(g.z + rndf_range(-6.0f, 6.0f), 4.0f, WORLD_N - 4.0f);
		g_fireflies[i].home = cf_v3(x, s_ground(x, z) + rndf_range(1.0f, 2.6f), z);
		g_fireflies[i].phase[0] = rndf() * CF_TAU;
		g_fireflies[i].phase[1] = rndf() * CF_TAU;
		g_fireflies[i].phase[2] = rndf() * CF_TAU;
		g_fireflies[i].speed = rndf_range(0.5f, 1.0f);
		g_fireflies[i].pos = g_fireflies[i].home;
		g_fireflies[i].caught = false;
		g_fireflies[i].delivered = false;
	}
}

static void s_update_fireflies(float t)
{
	for (int i = 0; i < FIREFLY_COUNT; ++i) {
		Firefly* f = &g_fireflies[i];
		if (f->caught) continue;
		if (f->delivered) {
			// Released at the shrine: spiral up and away, fading into the dawn.
			float age = t - f->release_t;
			float ang = f->phase[0] + age * 1.4f;
			float r = 1.0f + age * 0.55f;
			f->pos = cf_v3(
				g_shrine_pos.x + cosf(ang) * r,
				g_shrine_pos.y + 0.6f + age * 1.7f,
				g_shrine_pos.z + sinf(ang) * r);
			continue;
		}
		float s = f->speed;
		f->pos = cf_add_v3(f->home, cf_v3(
			sinf(t * 0.31f * s + f->phase[0]) * 2.2f + sinf(t * 0.83f * s + f->phase[2]) * 0.5f,
			sinf(t * 0.47f * s + f->phase[1]) * 0.7f,
			cosf(t * 0.27f * s + f->phase[2]) * 2.2f + cosf(t * 0.71f * s + f->phase[0]) * 0.5f));
	}
}

//--------------------------------------------------------------------------------------------------
// Game state.

typedef enum GameStage { STAGE_FIND_JAR, STAGE_COLLECT, STAGE_AWAKENED } GameStage;

typedef struct Game
{
	GameStage stage;
	int in_jar;       // Fireflies currently carried.
	int delivered;    // Fireflies released at the shrine.
	float dawn;       // 0 = dusk, 1 = the shrine's early dawn. Eases in after awakening.
	float awaken_t;
	const char* prompt;
} Game;

static Game g_game;

//--------------------------------------------------------------------------------------------------
// Input abstraction: everything the game reads lives here, filled either from the real
// keyboard/mouse or by the autopilot -- which is what makes the sample self-testable.

typedef struct Input
{
	float move_x, move_z;
	float look_dx, look_dy;
	bool interact;
	bool catch_click;
} Input;

typedef struct Autopilot
{
	bool enabled;
	float t;
	int stage;
	float stage_t;
} Autopilot;

static Autopilot g_auto;

// Steers toward a target: returns desired yaw and drives forward when roughly facing it.
static void s_auto_seek(Input* in, CF_V3 player, float yaw, CF_V3 target, float dt)
{
	CF_V3 to = cf_sub_v3(target, player);
	float want = atan2f(to.x, to.z);
	float diff = want - yaw;
	while (diff > CF_PI) diff -= CF_TAU;
	while (diff < -CF_PI) diff += CF_TAU;
	in->look_dx = cf_clamp(-diff * 3.0f, -2.4f, 2.4f) / 0.02f * dt * -1.0f;
	// The look_dx sign convention: yaw -= look_dx * 0.02, so negative diff steers correctly.
	in->look_dx = -diff * 2.0f / 0.02f * dt;
	if (cf_abs(diff) < 0.6f) in->move_z = 1.0f;
}

static void s_gather_input(Input* in, CF_V3 player, float yaw, float dt)
{
	memset(in, 0, sizeof(*in));
	if (g_auto.enabled) {
		g_auto.t += dt;
		g_auto.stage_t += dt;
		switch (g_auto.stage) {
		case 0: // Admire the glade with a slow turn.
			in->look_dx = 55.0f * dt;
			if (g_auto.stage_t > 4.5f) { g_auto.stage = 1; g_auto.stage_t = 0; }
			break;
		case 1: // Walk to the jar and take it.
			s_auto_seek(in, player, yaw, g_jar_pos, dt);
			if (cf_distance(player, g_jar_pos) < 2.4f) {
				in->interact = true;
				if (g_game.stage != STAGE_FIND_JAR) { g_auto.stage = 2; g_auto.stage_t = 0; }
			}
			if (g_auto.stage_t > 30.0f) { g_auto.stage = 2; g_auto.stage_t = 0; } // Safety.
			break;
		case 2: { // Hunt fireflies until the jar holds enough for the shrine.
			int best = -1;
			float best_d = 1e9f;
			for (int i = 0; i < FIREFLY_COUNT; ++i) {
				if (g_fireflies[i].caught || g_fireflies[i].delivered) continue;
				float d = cf_distance(player, g_fireflies[i].pos);
				if (d < best_d) { best_d = d; best = i; }
			}
			if (best >= 0) {
				s_auto_seek(in, player, yaw, g_fireflies[best].pos, dt);
				if (best_d < 2.6f) in->catch_click = true;
			}
			if (g_game.in_jar >= SHRINE_LANTERNS || g_auto.stage_t > 60.0f) { g_auto.stage = 3; g_auto.stage_t = 0; }
			break;
		}
		case 3: // Carry the light to the shrine.
			s_auto_seek(in, player, yaw, g_shrine_pos, dt);
			if (cf_distance(player, g_shrine_pos) < 4.0f) in->interact = true;
			if (g_game.stage == STAGE_AWAKENED) { g_auto.stage = 4; g_auto.stage_t = 0; }
			break;
		case 4: // Step back and watch the dawn.
			if (g_auto.stage_t < 2.5f) in->move_z = -0.6f;
			in->look_dx = sinf(g_auto.stage_t * 0.4f) * 8.0f * dt;
			break;
		}
		return;
	}
	if (cf_key_down(CF_KEY_W)) in->move_z += 1.0f;
	if (cf_key_down(CF_KEY_S)) in->move_z -= 1.0f;
	if (cf_key_down(CF_KEY_D)) in->move_x += 1.0f;
	if (cf_key_down(CF_KEY_A)) in->move_x -= 1.0f;
	in->look_dx = cf_mouse_motion_x() * 0.08f;
	in->look_dy = cf_mouse_motion_y() * 0.08f;
	in->interact = cf_key_just_pressed(CF_KEY_E);
	in->catch_click = cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT);
}

//--------------------------------------------------------------------------------------------------
// Shaders.

// Shadow-moment pass for the EVSM cascades. Instead of raw depth + a comparison sampler
// (which cannot be pre-filtered, hence shadow acne and bias-fighting), each cascade renders
// exponentially-warped depth and its square into a color target. Those moments CAN be
// blurred like any image, and the receiver tests against a Chebyshev bound -- no depth
// bias anywhere, no acne by construction, and softness comes from an honest gaussian.
#define EVSM_C "40.0"

static const char* s_moment_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 0) out vec2 v_zw;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"	mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"	vec4 hp = vec4(in_pos, 1.0);\n"
"	vec3 world = vec3(dot(in_model0, hp), dot(in_model1, hp), dot(in_model2, hp));\n"
"	vec4 clip = u_view_projection * vec4(world, 1.0);\n"
"	v_zw = clip.zw;\n"
"	gl_Position = clip;\n"
"}\n";

static const char* s_moment_fs =
"layout (location = 0) in vec2 v_zw;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"	float z = v_zw.x / v_zw.y;\n"        // [0, 1] under CF's clip conventions (ortho: w = 1).
"	float e = exp(" EVSM_C " * z);\n"
"	result = vec4(e, e * e, 0.0, 1.0);\n"
"}\n";

// Separable gaussian over one cascade layer of the moment map.
static const char* s_shadow_blur_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2DArray u_src;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"	vec4 u_dir_layer;\n" // xy = texel step direction, z = layer.\n
"};\n"
"void main() {\n"
"	vec2 s = u_dir_layer.xy;\n"
"	float layer = u_dir_layer.z;\n"
"	vec2 m = texture(u_src, vec3(v_uv, layer)).rg * 0.294;\n"
"	m += texture(u_src, vec3(v_uv + s * 1.18, layer)).rg * 0.235;\n"
"	m += texture(u_src, vec3(v_uv - s * 1.18, layer)).rg * 0.235;\n"
"	m += texture(u_src, vec3(v_uv + s * 2.76, layer)).rg * 0.118;\n"
"	m += texture(u_src, vec3(v_uv - s * 2.76, layer)).rg * 0.118;\n"
"	result = vec4(m, 0.0, 1.0);\n"
"}\n";

// The one lit block shader that carries the whole world. HDR out; tonemap happens in post.
static const char* s_block_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 11) in vec4 in_uv_rect;\n"         // Second user lane: (sway, 0, 1, 1).
"layout (location = 15) in vec4 in_mesh_attributes;\n" // rgb color + w emissive.
"layout (location = 0) out vec3 v_world;\n"
"layout (location = 1) out vec3 v_normal;\n"
"layout (location = 2) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"	mat4 u_view_projection;\n"
"	vec4 u_time;\n"
"};\n"
"void main() {\n"
"	vec3 p = in_pos;\n"
"	vec4 hp = vec4(p, 1.0);\n"
"	vec3 world = vec3(dot(in_model0, hp), dot(in_model1, hp), dot(in_model2, hp));\n"
"	float sway = in_uv_rect.x;\n"
"	world.x += sway * sin(u_time.x * 1.1 + world.z * 0.35 + world.y) * 0.06 * (p.y + 0.5);\n"
"	world.z += sway * cos(u_time.x * 0.9 + world.x * 0.35) * 0.06 * (p.y + 0.5);\n"
"	vec3 n = normalize(vec3(dot(in_model0.xyz, in_normal), dot(in_model1.xyz, in_normal), dot(in_model2.xyz, in_normal)));\n"
"	v_world = world;\n"
"	v_normal = n;\n"
"	v_color = in_mesh_attributes;\n"
"	gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_block_fs =
"layout (location = 0) in vec3 v_world;\n"
"layout (location = 1) in vec3 v_normal;\n"
"layout (location = 2) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2DArray u_shadow;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"	vec4 u_eye;\n"
"	vec4 u_sun_dir;\n"
"	vec4 u_sun_color;\n"
"	vec4 u_sky_ambient;\n"
"	vec4 u_ground_ambient;\n"
"	vec4 u_fog_color;\n"      // w = density.
"	vec4 u_lights[12];\n"     // xyz world pos, w intensity.
"	vec4 u_light_color;\n"
"	mat4 u_shadow_vp0;\n"
"	mat4 u_shadow_vp1;\n"
"	vec4 u_cascade;\n"        // x = cascade 0 far distance.
"};\n"
"float linstep(float lo, float hi, float x) { return clamp((x - lo) / (hi - lo), 0.0, 1.0); }\n"
"float shadow_sample(vec3 world, float ndl) {\n"
"	float dist = length(world - u_eye.xyz);\n"
"	float layer = dist < u_cascade.x ? 0.0 : 1.0;\n"
"	vec4 clip = (dist < u_cascade.x ? u_shadow_vp0 : u_shadow_vp1) * vec4(world, 1.0);\n"
"	vec3 ndc = clip.xyz / clip.w;\n"
"	vec2 uv = vec2(ndc.x, -ndc.y) * 0.5 + 0.5;\n"
"	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;\n"
"	// EVSM: Chebyshev upper bound over pre-blurred exponentially-warped moments.\n"
"	vec2 m = texture(u_shadow, vec3(uv, layer)).rg;\n"
"	float e = exp(" EVSM_C " * ndc.z);\n"
"	if (e <= m.x) return 1.0;\n"
"	float variance = max(m.y - m.x * m.x, 0.00005 * e * e);\n"
"	float d = e - m.x;\n"
"	float p = variance / (variance + d * d);\n"
"	// Light-bleed reduction: crush the low tail, keep the top smooth.\n"
"	return linstep(0.25, 1.0, p);\n"
"}\n"
"void main() {\n"
"	vec3 n = normalize(v_normal);\n"
"	vec3 albedo = v_color.rgb;\n"
"	float ndl = max(dot(n, -u_sun_dir.xyz), 0.0);\n"
"	float shadow = ndl > 0.0 ? shadow_sample(v_world, ndl) : 0.0;\n"
"	vec3 ambient = mix(u_ground_ambient.rgb, u_sky_ambient.rgb, n.y * 0.5 + 0.5);\n"
"	vec3 lit = albedo * (u_sun_color.rgb * ndl * shadow + ambient);\n"
"	for (int i = 0; i < 12; ++i) {\n"
"		vec3 to_l = u_lights[i].xyz - v_world;\n"
"		float d2 = dot(to_l, to_l);\n"
"		float atten = u_lights[i].w / (1.0 + d2 * 1.1);\n"
"		float wrap = max(dot(n, normalize(to_l)) * 0.5 + 0.5, 0.0);\n"
"		lit += albedo * u_light_color.rgb * atten * wrap;\n"
"	}\n"
"	lit += v_color.rgb * v_color.a;\n" // Emissive: fireflies, jar, lanterns. HDR-bright.\n
"	float dist = length(v_world - u_eye.xyz);\n"
"	float fog = 1.0 - exp(-u_fog_color.w * dist);\n"
"	fog = min(fog + 0.16 * exp(-max(v_world.y - 1.0, 0.0) * 0.30) * min(dist * 0.02, 1.0), 1.0);\n"
"	vec3 col = mix(lit, u_fog_color.rgb, fog);\n"
"	result = vec4(col, 1.0);\n"
"}\n";

// The sky: vertical gradient with a warm horizon band and a soft moon. HDR out.
static const char* s_sky_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 0) out vec3 v_dir;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"	mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"	vec4 hp = vec4(in_pos, 1.0);\n"
"	vec3 world = vec3(dot(in_model0, hp), dot(in_model1, hp), dot(in_model2, hp));\n"
"	v_dir = in_pos;\n"
"	vec4 clip = u_view_projection * vec4(world, 1.0);\n"
"	gl_Position = clip.xyww;\n"
"}\n";

static const char* s_sky_fs =
"layout (location = 0) in vec3 v_dir;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"	vec4 u_zenith;\n"
"	vec4 u_horizon;\n"
"	vec4 u_glow;\n"
"	vec4 u_moon_dir;\n"
"};\n"
"void main() {\n"
"	vec3 d = normalize(v_dir);\n"
"	float h = clamp(d.y, -0.1, 1.0);\n"
"	vec3 col = mix(u_horizon.rgb, u_zenith.rgb, pow(max(h, 0.0), 0.55));\n"
"	col += u_glow.rgb * pow(1.0 - abs(h), 6.0);\n"
"	float m = max(dot(d, u_moon_dir.xyz), 0.0);\n"
"	col += vec3(1.15, 1.2, 1.3) * smoothstep(0.9991, 0.9996, m) * 2.2;\n"
"	col += vec3(0.5, 0.55, 0.7) * pow(m, 160.0) * 0.35;\n"
"	result = vec4(col, 1.0);\n"
"}\n";

// Post: fullscreen triangle helpers.
static const char* s_fullscreen_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 0) out vec2 v_uv;\n"
"void main() {\n"
"	v_uv = in_pos * 0.5 + 0.5;\n"
"	v_uv.y = 1.0 - v_uv.y;\n"
"	gl_Position = vec4(in_pos, 0, 1);\n"
"}\n";

// Bright pass: keep what glows, soft knee below the threshold.
static const char* s_bright_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_scene;\n"
"void main() {\n"
"	vec3 c = texture(u_scene, v_uv).rgb;\n"
"	float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
"	float knee = smoothstep(0.85, 1.6, lum);\n"
"	result = vec4(c * knee, 1.0);\n"
"}\n";

// Downsample: 4-tap box with bilinear taps -- one pass per mip, ping-ponging textures.
static const char* s_down_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_src;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"	vec4 u_src_lod_texel;\n" // x = source lod, yz = source texel size at that lod.\n
"};\n"
"void main() {\n"
"	float lod = u_src_lod_texel.x;\n"
"	vec2 t = u_src_lod_texel.yz;\n"
"	vec3 c = textureLod(u_src, v_uv + vec2(-t.x, -t.y), lod).rgb\n"
"	       + textureLod(u_src, v_uv + vec2( t.x, -t.y), lod).rgb\n"
"	       + textureLod(u_src, v_uv + vec2(-t.x,  t.y), lod).rgb\n"
"	       + textureLod(u_src, v_uv + vec2( t.x,  t.y), lod).rgb;\n"
"	result = vec4(c * 0.25, 1.0);\n"
"}\n";

// Composite: scene + weighted bloom mips, filmic tonemap, subtle vignette.
static const char* s_tonemap_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_scene;\n"
"layout (set = 2, binding = 1) uniform sampler2D u_bloom_a;\n"
"layout (set = 2, binding = 2) uniform sampler2D u_bloom_b;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"	vec4 u_bloom_amount;\n"
"};\n"
"vec3 aces(vec3 x) {\n"
"	return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
"}\n"
"void main() {\n"
"	vec3 c = texture(u_scene, v_uv).rgb;\n"
"	vec3 bloom = textureLod(u_bloom_a, v_uv, 0.0).rgb * 0.30\n"
"	           + textureLod(u_bloom_b, v_uv, 1.0).rgb * 0.26\n"
"	           + textureLod(u_bloom_a, v_uv, 2.0).rgb * 0.22\n"
"	           + textureLod(u_bloom_b, v_uv, 3.0).rgb * 0.16\n"
"	           + textureLod(u_bloom_a, v_uv, 4.0).rgb * 0.12;\n"
"	c += bloom * u_bloom_amount.x;\n"
"	c = aces(c * 1.35);\n"
"	vec2 vig = v_uv - 0.5;\n"
"	c *= 1.0 - dot(vig, vig) * 0.45;\n"
"	c = pow(c, vec3(1.0 / 2.2));\n"
"	result = vec4(c, 1.0);\n"
"}\n";

//--------------------------------------------------------------------------------------------------
// Meshes.

typedef struct BlockVert { CF_V3 p; CF_V3 n; } BlockVert;

static CF_Mesh s_make_cube_mesh(void)
{
	BlockVert verts[36];
	int n = 0;
	for (int f = 0; f < 6; ++f) {
		int axis = f >> 1;
		float sign = (f & 1) ? -1.0f : 1.0f;
		CF_V3 nrm = cf_v3(axis == 0 ? sign : 0, axis == 1 ? sign : 0, axis == 2 ? sign : 0);
		CF_V3 u = cf_v3(nrm.y, nrm.z, nrm.x);
		CF_V3 w = cf_cross(nrm, u);
		CF_V3 c = cf_mul_v3_f(nrm, 0.5f);
		CF_V3 q[4];
		q[0] = cf_add_v3(c, cf_add_v3(cf_mul_v3_f(u, -0.5f), cf_mul_v3_f(w, -0.5f)));
		q[1] = cf_add_v3(c, cf_add_v3(cf_mul_v3_f(u,  0.5f), cf_mul_v3_f(w, -0.5f)));
		q[2] = cf_add_v3(c, cf_add_v3(cf_mul_v3_f(u,  0.5f), cf_mul_v3_f(w,  0.5f)));
		q[3] = cf_add_v3(c, cf_add_v3(cf_mul_v3_f(u, -0.5f), cf_mul_v3_f(w,  0.5f)));
		int idx[6] = { 0, 1, 2, 0, 2, 3 };
		for (int i = 0; i < 6; ++i) {
			verts[n].p = q[idx[i]];
			verts[n].n = nrm;
			n++;
		}
	}
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";    attrs[0].format = CF_VERTEX_FORMAT_FLOAT3; attrs[0].offset = CF_OFFSET_OF(BlockVert, p);
	attrs[1].name = "in_normal"; attrs[1].format = CF_VERTEX_FORMAT_FLOAT3; attrs[1].offset = CF_OFFSET_OF(BlockVert, n);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(BlockVert));
	cf_mesh_update_vertex_data(mesh, verts, 36);
	return mesh;
}

static CF_Mesh s_make_fullscreen_mesh(void)
{
	float verts[12] = { -1, -1, 3, -1, -1, 3, 0, 0, 0, 0, 0, 0 }; // One big triangle.
	CF_VertexAttribute attrs[1] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(float) * 6, attrs, 1, sizeof(float) * 2);
	cf_mesh_update_vertex_data(mesh, verts, 3);
	return mesh;
}

//--------------------------------------------------------------------------------------------------
// Block submission.

static void s_submit_block_ex(CF_Mesh cube, CF_V3 center, CF_V3 scale, CF_Color color, float emissive, float sway)
{
	cf_draw3d_push();
	cf_draw3d_translate(center);
	cf_draw3d_scale(scale);
	cf_draw3d_push_mesh_attributes(cf_v4(color.r, color.g, color.b, emissive));
	cf_draw3d_push_mesh_attributes2(cf_v4(sway, 0, 1, 1));
	cf_draw3d_mesh(cube);
	cf_draw3d_pop_mesh_attributes2();
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();
}

static void s_submit_block(CF_Mesh cube, const Block* b)
{
	s_submit_block_ex(cube, b->center, b->scale, b->color, b->emissive, b->sway);
}

// Everything that moves or glows dynamically: fireflies, the jar, the shrine lanterns.
static void s_draw_dynamic(CF_Mesh cube, CF_V3 player, CF_V3 fwd, float t)
{
	// Fireflies (loose and released ones).
	for (int i = 0; i < FIREFLY_COUNT; ++i) {
		Firefly* f = &g_fireflies[i];
		if (f->caught) continue;
		float fade = 1.0f;
		if (f->delivered) {
			float age = t - f->release_t;
			fade = cf_clamp(1.0f - (age - 6.0f) / 3.0f, 0, 1);
			if (fade <= 0) continue;
		}
		float pulse = 7.0f + sinf(t * 2.2f + f->phase[0] * 3.0f) * 3.5f;
		s_submit_block_ex(cube, f->pos, cf_v3(0.06f, 0.06f, 0.06f),
			cf_make_color_rgb_f(1.0f, 0.68f, 0.24f), cf_max(pulse, 1.2f) * fade, 0);
	}

	// The jar: a pale glass block on its stump until taken, then held low in view,
	// glowing with its catch.
	if (g_game.stage == STAGE_FIND_JAR) {
		float bob = sinf(t * 1.7f) * 0.04f;
		s_submit_block_ex(cube, cf_add_v3(g_jar_pos, cf_v3(0, 0.18f + bob, 0)), cf_v3(0.34f, 0.42f, 0.34f),
			cf_make_color_rgb_f(0.75f, 0.85f, 0.92f), 0.55f + sinf(t * 2.4f) * 0.2f, 0);
	} else {
		CF_V3 right = cf_norm(cf_cross(cf_v3(fwd.x, 0, fwd.z), cf_v3(0, 1, 0)));
		CF_V3 hold = cf_add_v3(player, cf_add_v3(cf_mul_v3_f(fwd, 0.85f),
			cf_add_v3(cf_mul_v3_f(right, 0.30f), cf_v3(0, -0.30f + sinf(t * 2.1f) * 0.008f, 0))));
		float glow = 1.1f + 1.5f * (float)g_game.in_jar;
		s_submit_block_ex(cube, hold, cf_v3(0.055f, 0.07f, 0.055f),
			cf_make_color_rgb_f(1.0f, 0.72f, 0.30f), glow, 0);
	}

	// Shrine lanterns: lit ones burn warm; unlit ones are dark glass.
	for (int i = 0; i < SHRINE_LANTERNS; ++i) {
		float ang = (float)i / SHRINE_LANTERNS * CF_TAU;
		float px = g_shrine_pos.x + cosf(ang) * 5.5f;
		float pz = g_shrine_pos.z + sinf(ang) * 5.5f;
		float pg = s_ground(px, pz);
		bool lit = i < g_game.delivered;
		float em = lit ? 3.0f + sinf(t * 1.9f + i) * 0.7f : 0.05f;
		CF_Color c = lit ? cf_make_color_rgb_f(1.0f, 0.72f, 0.30f) : cf_make_color_rgb_f(0.5f, 0.62f, 0.72f);
		s_submit_block_ex(cube, cf_v3(px, pg + 3.1f, pz), cf_v3(0.55f, 0.55f, 0.55f), c, em, 0);
	}

	// The altar heart: dark until awakened, then a rising sunstone.
	{
		float rise = g_game.dawn * 1.6f;
		float em = 0.1f + g_game.dawn * 7.0f + (g_game.stage == STAGE_AWAKENED ? sinf(t * 1.3f) * 0.8f : 0);
		s_submit_block_ex(cube, cf_add_v3(g_shrine_pos, cf_v3(0, 0.45f + rise, 0)),
			cf_v3(0.6f, 0.6f, 0.6f), cf_make_color_rgb_f(1.0f, 0.78f, 0.40f), em, 0);
	}
}

//--------------------------------------------------------------------------------------------------
// Screenshots.

static int s_shot_count;
static void s_screenshot(void)
{
	CF_Canvas canvas = cf_app_get_canvas();
	int w = 0, h = 0;
	cf_canvas_get_size(canvas, &w, &h);
	CF_Pixel* px = (CF_Pixel*)cf_alloc((size_t)w * h * sizeof(CF_Pixel));
	CF_Readback rb = cf_canvas_readback(canvas);
	if (rb.id) {
		while (!cf_readback_ready(rb)) {}
		cf_readback_data(rb, px, w * h * (int)sizeof(CF_Pixel));
		cf_destroy_readback(rb);
		CF_Image img;
		img.w = w;
		img.h = h;
		img.pix = px;
		char path[256];
		snprintf(path, sizeof(path), "/fireflies_shot_%02d.png", s_shot_count++);
		cf_image_save_png(path, &img);
		printf("saved %s (%dx%d)\n", path, w, h);
	}
	cf_free(px);
}

//--------------------------------------------------------------------------------------------------

typedef struct Post
{
	int w, h;
	CF_Canvas scene;               // HDR scene target.
	CF_Texture bloom_a, bloom_b;   // Half-res mipped HDR textures, ping-ponged.
	CF_Canvas bloom_canvas[BLOOM_LEVELS]; // Level k renders into (k odd ? b : a) at mip k.
	CF_Mesh fullscreen;
	CF_Shader bright, down, tonemap;
	CF_Material mat_bright, mat_down, mat_tonemap;
} Post;

static void s_post_destroy_targets(Post* p)
{
	if (p->scene.id) cf_destroy_canvas(p->scene);
	for (int i = 0; i < BLOOM_LEVELS; ++i) if (p->bloom_canvas[i].id) cf_destroy_canvas(p->bloom_canvas[i]);
	if (p->bloom_a.id) cf_destroy_texture(p->bloom_a);
	if (p->bloom_b.id) cf_destroy_texture(p->bloom_b);
	p->scene.id = 0;
	p->bloom_a.id = 0;
	p->bloom_b.id = 0;
	for (int i = 0; i < BLOOM_LEVELS; ++i) p->bloom_canvas[i].id = 0;
}

static void s_post_make_targets(Post* p, int w, int h)
{
	s_post_destroy_targets(p);
	p->w = w;
	p->h = h;

	CF_CanvasParams scene_params = cf_canvas_defaults(w, h);
	scene_params.target.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	scene_params.target.filter = CF_FILTER_LINEAR;
	scene_params.depth_stencil_enable = true;
	p->scene = cf_make_canvas(scene_params);

	// Two mipped half-res HDR textures; the downsample chain ping-pongs between them so a
	// pass never samples the texture it writes (mip k of one samples mip k-1 of the other).
	CF_TextureParams tp = cf_texture_defaults(w / 2 < 1 ? 1 : w / 2, h / 2 < 1 ? 1 : h / 2);
	tp.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	tp.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
	tp.filter = CF_FILTER_LINEAR;
	tp.mip_filter = CF_MIP_FILTER_NEAREST;
	tp.allocate_mipmaps = true;
	tp.mip_count = BLOOM_LEVELS;
	p->bloom_a = cf_make_texture(tp);
	p->bloom_b = cf_make_texture(tp);
	for (int k = 0; k < BLOOM_LEVELS; ++k) {
		CF_CanvasParams bp = cf_canvas_defaults(0, 0);
		bp.attach_target = (k & 1) ? p->bloom_b : p->bloom_a;
		bp.attach_mip = k;
		p->bloom_canvas[k] = cf_make_canvas(bp);
	}
}

// Runs the whole post chain: bright pass into bloom mip 0, downsample through the ping-pong
// mips, then tonemap + composite into the app canvas.
static void s_post_run(Post* p)
{
	// Bright pass.
	cf_apply_canvas(p->bloom_canvas[0], true);
	cf_apply_mesh(p->fullscreen);
	cf_material_set_texture_fs(p->mat_bright, "u_scene", cf_canvas_get_target(p->scene));
	cf_apply_shader(p->bright, p->mat_bright);
	cf_draw_elements();

	// Downsample chain.
	for (int k = 1; k < BLOOM_LEVELS; ++k) {
		CF_Texture src = (k & 1) ? p->bloom_a : p->bloom_b; // Previous level lives in the other texture.
		int src_w = cf_max(p->w / 2 >> (k - 1), 1);
		int src_h = cf_max(p->h / 2 >> (k - 1), 1);
		CF_V4 lod_texel = cf_v4((float)(k - 1), 0.75f / (float)src_w, 0.75f / (float)src_h, 0);
		cf_apply_canvas(p->bloom_canvas[k], true);
		cf_apply_mesh(p->fullscreen);
		cf_material_set_texture_fs(p->mat_down, "u_src", src);
		cf_material_set_uniform_fs(p->mat_down, "u_src_lod_texel", &lod_texel, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_apply_shader(p->down, p->mat_down);
		cf_draw_elements();
	}

	// Composite + tonemap into the app canvas.
	CF_V4 amount = cf_v4(0.85f, 0, 0, 0);
	cf_apply_canvas(cf_app_get_canvas(), true);
	cf_apply_mesh(p->fullscreen);
	cf_material_set_texture_fs(p->mat_tonemap, "u_scene", cf_canvas_get_target(p->scene));
	cf_material_set_texture_fs(p->mat_tonemap, "u_bloom_a", p->bloom_a);
	cf_material_set_texture_fs(p->mat_tonemap, "u_bloom_b", p->bloom_b);
	cf_material_set_uniform_fs(p->mat_tonemap, "u_bloom_amount", &amount, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_apply_shader(p->tonemap, p->mat_tonemap);
	cf_draw_elements();
}

//--------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	float shot_times[16];
	int shot_n = 0;
	float exit_at = -1.0f;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--auto")) g_auto.enabled = true;
		else if (!strcmp(argv[i], "--shot") && i + 1 < argc) { if (shot_n < 16) shot_times[shot_n++] = (float)atof(argv[++i]); }
		else if (!strcmp(argv[i], "--exit-at") && i + 1 < argc) exit_at = (float)atof(argv[++i]);
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Fireflies", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	if (!g_auto.enabled) {
		// First-person look: capture the mouse and read raw deltas.
		cf_mouse_set_relative_mode(true);
	}

	s_build_world();
	s_spawn_fireflies();
	g_game.stage = STAGE_FIND_JAR;

	CF_Mesh cube = s_make_cube_mesh();
	CF_Shader block_shd = cf_make_shader_from_source(s_block_vs, s_block_fs);
	CF_Shader sky_shd = cf_make_shader_from_source(s_sky_vs, s_sky_fs);
	CF_Shader moment_shd = cf_make_shader_from_source(s_moment_vs, s_moment_fs);
	CF_Shader shadow_blur_shd = cf_make_shader_from_source(s_fullscreen_vs, s_shadow_blur_fs);
	CF_Material shadow_blur_mat = cf_make_material();

	// EVSM cascades: exponentially-warped depth moments in a color 2d-array (one layer per
	// cascade), plus a scratch array for the separable blur. Each canvas targets one layer.
	CF_Texture shadow_tex, shadow_scratch;
	CF_Canvas shadow_canvas[2], shadow_blur_h[2], shadow_blur_v[2];
	{
		CF_TextureParams sp = cf_texture_defaults(SHADOW_RES, SHADOW_RES);
		sp.pixel_format = CF_PIXEL_FORMAT_R32G32_FLOAT;
		sp.texture_type = CF_TEXTURE_TYPE_2D_ARRAY;
		sp.layer_count = 2;
		sp.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT;
		sp.filter = CF_FILTER_LINEAR;
		shadow_tex = cf_make_texture(sp);
		shadow_scratch = cf_make_texture(sp);
		float e = expf(40.0f); // Matches EVSM_C: an empty texel reads as "farthest possible".
		for (int i = 0; i < 2; ++i) {
			// Sized defaults so depth_stencil_target params are filled; the attach branch
			// ignores the color target entirely and sizes depth to the attach mip.
			CF_CanvasParams cp = cf_canvas_defaults(SHADOW_RES, SHADOW_RES);
			cp.attach_target = shadow_tex;
			cp.attach_layer = i;
			cp.depth_stencil_enable = true; // The moment pass still depth-tests its own boxes.
			shadow_canvas[i] = cf_make_canvas(cp);
			cf_canvas_set_clear_color(shadow_canvas[i], cf_make_color_rgba_f(e, e * e, 0, 1.0f));

			CF_CanvasParams hp2 = cf_canvas_defaults(0, 0);
			hp2.attach_target = shadow_scratch;
			hp2.attach_layer = i;
			shadow_blur_h[i] = cf_make_canvas(hp2);

			CF_CanvasParams vp2 = cf_canvas_defaults(0, 0);
			vp2.attach_target = shadow_tex;
			vp2.attach_layer = i;
			shadow_blur_v[i] = cf_make_canvas(vp2);
		}
	}

	Post post = { 0 };
	post.fullscreen = s_make_fullscreen_mesh();
	post.bright = cf_make_shader_from_source(s_fullscreen_vs, s_bright_fs);
	post.down = cf_make_shader_from_source(s_fullscreen_vs, s_down_fs);
	post.tonemap = cf_make_shader_from_source(s_fullscreen_vs, s_tonemap_fs);
	post.mat_bright = cf_make_material();
	post.mat_down = cf_make_material();
	post.mat_tonemap = cf_make_material();
	{
		int w, h;
		cf_app_get_size(&w, &h);
		s_post_make_targets(&post, w, h);
	}

	CF_V3 player = cf_v3(WORLD_N * 0.5f + 24.0f, 0, WORLD_N * 0.5f + 6.0f);
	float yaw = 3.9f, pitch = 0.0f;
	player.y = s_ground(player.x, player.z) + EYE_HEIGHT;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		float dt = CF_DELTA_TIME;
		t += dt;

		Input in;
		s_gather_input(&in, player, yaw, dt);

		yaw -= in.look_dx * 0.02f;
		pitch = cf_clamp(pitch - in.look_dy * 0.02f, -1.4f, 1.4f);
		CF_V3 fwd = cf_v3(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch));
		CF_V3 flat_fwd = cf_norm(cf_v3(fwd.x, 0, fwd.z));
		CF_V3 right = cf_cross(flat_fwd, cf_v3(0, 1, 0));

		float speed = 5.0f;
		CF_V3 wish = cf_add_v3(cf_mul_v3_f(flat_fwd, in.move_z), cf_mul_v3_f(right, in.move_x));
		if (cf_len(wish) > 0.01f) wish = cf_norm(wish);
		player.x = cf_clamp(player.x + wish.x * speed * dt, 3.0f, WORLD_N - 3.0f);
		player.z = cf_clamp(player.z + wish.z * speed * dt, 3.0f, WORLD_N - 3.0f);
		// Soft radial push away from tree trunks: enough to never clip through one, gentle
		// enough to slide around it and keep walking.
		for (int i = 0; i < g_trunk_count; ++i) {
			CF_V2 d = cf_sub_v2(cf_v2(player.x, player.z), g_trunks[i]);
			float d2 = cf_dot(d, d);
			const float r = 0.75f;
			if (d2 < r * r && d2 > 1.0e-6f) {
				float dist = sqrtf(d2);
				CF_V2 push = cf_mul_v2_f(d, (r - dist) / dist);
				player.x += push.x;
				player.z += push.y;
			}
		}
		float target_y = s_ground(player.x, player.z) + EYE_HEIGHT;
		player.y += (target_y - player.y) * cf_min(dt * 10.0f, 1.0f);

		s_update_fireflies(t);

		// Gameplay.
		g_game.prompt = NULL;
		if (g_game.stage == STAGE_FIND_JAR) {
			if (cf_distance(player, g_jar_pos) < 2.6f) {
				g_game.prompt = "E -- take the jar";
				if (in.interact) g_game.stage = STAGE_COLLECT;
			}
		} else {
			// Catch: nearest loose firefly close to the crosshair ray.
			if (in.catch_click && g_game.stage != STAGE_FIND_JAR) {
				CF_Ray3 ray;
				ray.p = player;
				ray.d = fwd;
				ray.t = 3.4f;
				for (int i = 0; i < FIREFLY_COUNT; ++i) {
					Firefly* f = &g_fireflies[i];
					if (f->caught || f->delivered) continue;
					if (cf_ray3_to_sphere(ray, cf_make_sphere(f->pos, 0.55f)).hit) {
						f->caught = true;
						g_game.in_jar++;
						break;
					}
				}
			}
			// Deliver at the shrine.
			if (g_game.stage == STAGE_COLLECT && cf_distance(player, g_shrine_pos) < 4.5f) {
				if (g_game.in_jar > 0) {
					g_game.prompt = "E -- set them free";
					if (in.interact) {
						for (int i = 0; i < FIREFLY_COUNT && g_game.in_jar > 0; ++i) {
							Firefly* f = &g_fireflies[i];
							if (!f->caught) continue;
							f->caught = false;
							f->delivered = true;
							f->release_t = t;
							g_game.in_jar--;
							g_game.delivered++;
						}
						if (g_game.delivered >= SHRINE_LANTERNS) {
							g_game.stage = STAGE_AWAKENED;
							g_game.awaken_t = t;
						}
					}
				} else if (g_game.delivered < SHRINE_LANTERNS) {
					g_game.prompt = "the shrine sleeps -- bring fireflies";
				}
			}
		}
		if (g_game.stage == STAGE_AWAKENED) {
			g_game.dawn = cf_min(g_game.dawn + dt / 6.0f, 1.0f);
		}

		// Handle window resizes for the post targets.
		int w, h;
		cf_app_get_size(&w, &h);
		if (w != post.w || h != post.h) s_post_make_targets(&post, w, h);

		CF_M4x4 proj = cf_perspective(CF_PI / 3.2f, (float)w / (float)h, 0.1f, 260.0f);
		CF_M4x4 view = cf_look_at(player, cf_add_v3(player, fwd), cf_v3(0, 1, 0));
		CF_Frustum frustum = cf_frustum_from_m4(cf_mul_m4(proj, view));

		// The dusk-to-dawn palette. `dawn` eases the whole world warm after the shrine wakes.
		float dawn = g_game.dawn;
		CF_V3 sun_dir = cf_norm(cf_v3(-0.38f, cf_clamp(-0.46f + dawn * 0.14f, -0.6f, -0.2f), -0.55f));
		CF_Color sun_color = cf_make_color_rgb_f(
			0.62f + dawn * 1.05f, 0.68f + dawn * 0.52f, 0.95f - dawn * 0.18f);
		CF_Color sky_amb = cf_make_color_rgb_f(0.13f + dawn * 0.20f, 0.18f + dawn * 0.15f, 0.28f + dawn * 0.04f);
		CF_Color gnd_amb = cf_make_color_rgb_f(0.045f + dawn * 0.10f, 0.06f + dawn * 0.065f, 0.085f + dawn * 0.02f);
		CF_V4 fog = cf_v4(
			0.09f + dawn * 0.33f, 0.14f + dawn * 0.13f, 0.22f - dawn * 0.02f,
			0.017f - dawn * 0.007f);

		// Shadow cascades: a tight one around the player, a wide one for the vale. Snap the
		// centers to shadow-texel steps so the world doesn't shimmer as the camera glides.
		CF_M4x4 shadow_vp[2];
		float cascade_r[2] = { 14.0f, 52.0f };
		for (int i = 0; i < 2; ++i) {
			float r = cascade_r[i];
			CF_V3 center = cf_add_v3(player, cf_mul_v3_f(flat_fwd, r * 0.55f));
			float texel = (r * 2.0f) / SHADOW_RES;
			center.x = floorf(center.x / texel) * texel;
			center.y = floorf(center.y / texel) * texel;
			center.z = floorf(center.z / texel) * texel;
			CF_M4x4 lview = cf_look_at(cf_sub_v3(center, cf_mul_v3_f(sun_dir, 70.0f)), center, cf_v3(0, 1, 0));
			CF_M4x4 lproj = cf_ortho(-r, r, -r, r, 1.0f, 160.0f);
			shadow_vp[i] = cf_mul_m4(lproj, lview);
		}

		// Pass 1 + 2: render the world's shadow moments into each cascade. Same submissions,
		// moment shader pushed, light matrix as the camera -- multi-pass exactly as the
		// draw3d header prescribes. No depth bias anywhere: EVSM doesn't need one.
		CF_RenderState shadow_rs = cf_render_state_3d_defaults();
		shadow_rs.cull_mode = CF_CULL_MODE_NONE; // Leaf boxes are thin; keep their backs.
		for (int cascade = 0; cascade < 2; ++cascade) {
			cf_draw3d_push_projection(shadow_vp[cascade]);
			cf_draw3d_push_view(cf_m4_identity());
			cf_draw3d_push_shader(moment_shd);
			cf_draw3d_push_render_state(shadow_rs);
			CF_V3 cc = cf_add_v3(player, cf_mul_v3_f(flat_fwd, cascade_r[cascade] * 0.55f));
			CF_Sphere cull = cf_make_sphere(cc, cascade_r[cascade] * 1.75f);
			for (int i = 0; i < CHUNKS * CHUNKS; ++i) {
				Chunk* c = &g_world.chunks[i];
				if (!c->count) continue;
				if (!cf_sphere_to_aabb3(cull, c->bounds)) continue;
				for (int k = 0; k < c->count; ++k) s_submit_block(cube, &c->blocks[k]);
			}
			cf_draw3d_pop_render_state();
			cf_draw3d_pop_shader();
			cf_draw3d_pop_view();
			cf_draw3d_pop_projection();
			cf_render_to(shadow_canvas[cascade], true);

			// Separable gaussian over the cascade's moments: horizontal into the scratch
			// layer, vertical back into the moment map. This blur is what a comparison
			// sampler could never allow -- and why the acne is gone.
			for (int dir = 0; dir < 2; ++dir) {
				CF_V4 dir_layer = dir == 0
					? cf_v4(1.0f / SHADOW_RES, 0, (float)cascade, 0)
					: cf_v4(0, 1.0f / SHADOW_RES, (float)cascade, 0);
				cf_apply_canvas(dir == 0 ? shadow_blur_h[cascade] : shadow_blur_v[cascade], true);
				cf_apply_mesh(post.fullscreen);
				cf_material_set_texture_fs(shadow_blur_mat, "u_src", dir == 0 ? shadow_tex : shadow_scratch);
				cf_material_set_uniform_fs(shadow_blur_mat, "u_dir_layer", &dir_layer, CF_UNIFORM_TYPE_FLOAT4, 1);
				cf_apply_shader(shadow_blur_shd, shadow_blur_mat);
				cf_draw_elements();
			}
		}

		// Pass 3: the scene, in HDR.
		cf_draw3d_push_projection(proj);
		cf_draw3d_push_view(view);

		cf_draw3d_set_uniform_v3("u_eye", player);
		cf_draw3d_set_uniform_v3("u_sun_dir", sun_dir);
		cf_draw3d_set_uniform_color("u_sun_color", sun_color);
		cf_draw3d_set_uniform_color("u_sky_ambient", sky_amb);
		cf_draw3d_set_uniform_color("u_ground_ambient", gnd_amb);
		cf_draw3d_set_uniform("u_fog_color", &fog, CF_UNIFORM_TYPE_FLOAT4, 1);
		CF_V4 time4 = cf_v4(t, 0, 0, 0);
		cf_draw3d_set_uniform("u_time", &time4, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_draw3d_set_uniform_color("u_light_color", cf_make_color_rgb_f(1.0f, 0.72f, 0.32f));
		cf_draw3d_set_uniform_m4("u_shadow_vp0", shadow_vp[0]);
		cf_draw3d_set_uniform_m4("u_shadow_vp1", shadow_vp[1]);
		CF_V4 cascade4 = cf_v4(cascade_r[0] * 1.05f, 0, 0, 0);
		cf_draw3d_set_uniform("u_cascade", &cascade4, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_draw3d_set_texture("u_shadow", shadow_tex);

		// Firefly + jar lights.
		CF_V4 lights[FIREFLY_LIGHTS];
		int nl = 0;
		if (g_game.stage != STAGE_FIND_JAR && g_game.in_jar > 0) {
			CF_V3 lp = cf_add_v3(player, cf_mul_v3_f(fwd, 0.7f));
			lights[nl++] = cf_v4(lp.x, lp.y - 0.3f, lp.z, 0.9f + 0.55f * g_game.in_jar);
		}
		for (int i = 0; i < FIREFLY_COUNT && nl < FIREFLY_LIGHTS; ++i) {
			Firefly* f = &g_fireflies[i];
			if (f->caught) continue;
			CF_V3 d = cf_sub_v3(f->pos, player);
			if (cf_dot(d, d) < 34.0f * 34.0f) {
				lights[nl++] = cf_v4(f->pos.x, f->pos.y, f->pos.z, 1.5f);
			}
		}
		for (int i = nl; i < FIREFLY_LIGHTS; ++i) lights[i] = cf_v4(0, -1000.0f, 0, 0);
		cf_draw3d_set_uniform("u_lights", lights, CF_UNIFORM_TYPE_FLOAT4, FIREFLY_LIGHTS);

		// Sky beneath the world.
		cf_draw_push_layer(-1);
		cf_draw3d_push_shader(sky_shd);
		CF_RenderState sky_rs = cf_render_state_3d_defaults();
		sky_rs.depth_compare = CF_COMPARE_FUNCTION_ALWAYS;
		sky_rs.depth_write_enabled = false;
		sky_rs.cull_mode = CF_CULL_MODE_NONE;
		cf_draw3d_push_render_state(sky_rs);
		CF_V3 moon = cf_neg(sun_dir);
		CF_V4 moon4 = cf_v4(moon.x, moon.y, moon.z, 0);
		cf_draw3d_set_uniform_color("u_zenith", cf_make_color_rgb_f(0.035f + dawn * 0.15f, 0.07f + dawn * 0.22f, 0.16f + dawn * 0.30f));
		cf_draw3d_set_uniform_color("u_horizon", cf_make_color_rgb_f(0.14f + dawn * 0.72f, 0.22f + dawn * 0.30f, 0.34f - dawn * 0.06f));
		cf_draw3d_set_uniform_color("u_glow", cf_make_color_rgb_f(0.55f + dawn * 0.95f, 0.28f + dawn * 0.33f, 0.10f + dawn * 0.06f));
		cf_draw3d_set_uniform("u_moon_dir", &moon4, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_draw3d_push();
		cf_draw3d_translate(player);
		cf_draw3d_scale(cf_v3(400.0f, 400.0f, 400.0f));
		cf_draw3d_mesh(cube);
		cf_draw3d_pop();
		cf_draw3d_pop_render_state();
		cf_draw3d_pop_shader();
		cf_draw_pop_layer();

		// The world.
		cf_draw3d_push_shader(block_shd);
		int visible_chunks = 0;
		for (int i = 0; i < CHUNKS * CHUNKS; ++i) {
			Chunk* c = &g_world.chunks[i];
			if (!c->count) continue;
			if (!cf_frustum_test_aabb3(&frustum, c->bounds)) continue;
			visible_chunks++;
			for (int k = 0; k < c->count; ++k) s_submit_block(cube, &c->blocks[k]);
		}
		s_draw_dynamic(cube, player, fwd, t);
		cf_draw3d_pop_shader();

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
		cf_render_to(post.scene, true);

		// Pass 4: bloom + tonemap into the app canvas.
		s_post_run(&post);

		// HUD over the top, through the ordinary 2d layer.
		{
			cf_push_font_size(18);
			if (g_game.prompt) {
				cf_draw_push_color(cf_make_color_rgba_f(1.0f, 0.94f, 0.8f, 0.85f));
				float tw = cf_text_width(g_game.prompt, -1);
				cf_draw_text(g_game.prompt, cf_v2(-tw * 0.5f, -(float)h * 0.18f), -1);
				cf_draw_pop_color();
			}
			if (g_game.stage != STAGE_FIND_JAR) {
				char buf[64];
				snprintf(buf, sizeof(buf), "fireflies %d   lanterns %d / %d", g_game.in_jar, g_game.delivered, SHRINE_LANTERNS);
				cf_draw_push_color(cf_make_color_rgba_f(1, 1, 1, 0.4f));
				cf_draw_text(buf, cf_v2(-(float)w * 0.5f + 16, -(float)h * 0.5f + 34), -1);
				cf_draw_pop_color();
			}
			if (g_game.stage == STAGE_AWAKENED && t - g_game.awaken_t < 6.0f) {
				const char* line = "the shrine remembers the sun";
				float a = cf_clamp((t - g_game.awaken_t) / 1.5f, 0, 1) * cf_clamp((6.0f - (t - g_game.awaken_t)) / 1.5f, 0, 1);
				cf_draw_push_color(cf_make_color_rgba_f(1.0f, 0.9f, 0.7f, a));
				float tw = cf_text_width(line, -1);
				cf_draw_text(line, cf_v2(-tw * 0.5f, (float)h * 0.22f), -1);
				cf_draw_pop_color();
			}
			// Crosshair dot.
			cf_draw_push_color(cf_make_color_rgba_f(1, 1, 1, 0.35f));
			cf_draw_circle_fill2(cf_v2(0, 0), 2.0f);
			cf_draw_pop_color();
			cf_pop_font_size();
		}
		cf_render_to(cf_app_get_canvas(), false);
		cf_app_draw_onto_screen(false);

		for (int i = 0; i < shot_n; ++i) {
			if (shot_times[i] >= 0 && t >= shot_times[i]) {
				s_screenshot();
				shot_times[i] = -1.0f;
			}
		}
		if (exit_at > 0 && t >= exit_at) break;
	}

	s_post_destroy_targets(&post);
	cf_destroy_material(post.mat_bright);
	cf_destroy_material(post.mat_down);
	cf_destroy_material(post.mat_tonemap);
	cf_destroy_shader(post.bright);
	cf_destroy_shader(post.down);
	cf_destroy_shader(post.tonemap);
	cf_destroy_mesh(post.fullscreen);
	for (int i = 0; i < 2; ++i) {
		cf_destroy_canvas(shadow_canvas[i]);
		cf_destroy_canvas(shadow_blur_h[i]);
		cf_destroy_canvas(shadow_blur_v[i]);
	}
	cf_destroy_texture(shadow_tex);
	cf_destroy_texture(shadow_scratch);
	cf_destroy_material(shadow_blur_mat);
	cf_destroy_shader(shadow_blur_shd);
	cf_destroy_shader(block_shd);
	cf_destroy_shader(sky_shd);
	cf_destroy_shader(moment_shd);
	cf_destroy_mesh(cube);
	cf_destroy_app();
	return 0;
}
