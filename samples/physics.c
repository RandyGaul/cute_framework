/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Box2D through CF: a pyramid of boxes on a ground segment, stepped on CF's fixed
// timestep and rendered with cf_physics_draw -- the whole wiring described in
// cute_physics.h in one place.
//
//   - The world steps inside the CF_OnUpdateFn callback via cf_physics_step, so physics
//     ticks exactly once per fixed update no matter the render framerate.
//   - Rendering scales the 2d camera by PIXELS_PER_METER, then cf_physics_draw renders
//     the world's shapes as anti-aliased CF draw calls in physics units.
//   - Everything Box2D is called directly: b2CreateWorld, b2CreateBody, b2World_Explode.
//     CF adds the seam, not a wrapper.
//
// Left click drops a box or ball at the mouse, right click sets off an explosion,
// space resets the scene.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

#define PIXELS_PER_METER 32.0f

b2WorldId g_world;
int g_body_count;

static void s_make_box(CF_V2 pos, float hw, float hh, bool dynamic)
{
	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = dynamic ? b2_dynamicBody : b2_staticBody;
	body_def.position = cf_v2_to_b2(pos);
	b2BodyId body = b2CreateBody(g_world, &body_def);
	b2ShapeDef shape_def = b2DefaultShapeDef();
	b2Polygon box = b2MakeBox(hw, hh);
	b2CreatePolygonShape(body, &shape_def, &box);
	g_body_count++;
}

static void s_make_ball(CF_V2 pos, float r)
{
	b2BodyDef body_def = b2DefaultBodyDef();
	body_def.type = b2_dynamicBody;
	body_def.position = cf_v2_to_b2(pos);
	b2BodyId body = b2CreateBody(g_world, &body_def);
	b2ShapeDef shape_def = b2DefaultShapeDef();
	shape_def.material.restitution = 0.6f;
	b2Circle circle = { { 0, 0 }, r };
	b2CreateCircleShape(body, &shape_def, &circle);
	g_body_count++;
}

static void s_build_scene(void)
{
	if (B2_IS_NON_NULL(g_world)) b2DestroyWorld(g_world);
	g_body_count = 0;

	b2WorldDef world_def = b2DefaultWorldDef();
	world_def.gravity.x = 0;
	world_def.gravity.y = -10.0f;
	g_world = b2CreateWorld(&world_def);

	// Ground: one static segment shape spanning the vale.
	b2BodyDef ground_def = b2DefaultBodyDef();
	ground_def.position.x = 0;
	ground_def.position.y = -8.0f;
	b2BodyId ground = b2CreateBody(g_world, &ground_def);
	b2ShapeDef ground_shape = b2DefaultShapeDef();
	b2Segment segment = { { -18.0f, 0 }, { 18.0f, 0 } };
	b2CreateSegmentShape(ground, &ground_shape, &segment);

	// A pyramid of boxes.
	int rows = 10;
	float half = 0.5f;
	for (int row = 0; row < rows; ++row) {
		int count = rows - row;
		float y = -8.0f + half + (2.0f * half) * row;
		float x0 = -(count - 1) * half;
		for (int i = 0; i < count; ++i) {
			s_make_box(cf_v2(x0 + i * 2.0f * half, y), half * 0.95f, half * 0.95f, true);
		}
	}

	// A capsule leaning on the pile for shape variety.
	b2BodyDef cap_def = b2DefaultBodyDef();
	cap_def.type = b2_dynamicBody;
	cap_def.position.x = -8.0f;
	cap_def.position.y = 2.0f;
	b2BodyId cap_body = b2CreateBody(g_world, &cap_def);
	b2ShapeDef cap_shape = b2DefaultShapeDef();
	b2Capsule capsule = { { 0, -1.0f }, { 0, 1.0f }, 0.4f };
	b2CreateCapsuleShape(cap_body, &cap_shape, &capsule);
	g_body_count++;
}

// Physics ticks on CF's fixed clock: this runs exactly once per fixed update.
static void s_fixed_update(void* udata)
{
	(void)udata;
	cf_physics_step(g_world, 4);
}

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
		snprintf(path, sizeof(path), "/physics_shot_%02d.png", s_shot_count++);
		cf_image_save_png(path, &img);
		printf("saved %s (%dx%d)\n", path, w, h);
	}
	cf_free(px);
}

int main(int argc, char* argv[])
{
	float shot_times[16];
	int shot_n = 0;
	float exit_at = -1.0f;
	for (int i = 1; i < argc; ++i) {
		if (!CF_STRCMP(argv[i], "--shot") && i + 1 < argc) { if (shot_n < 16) shot_times[shot_n++] = (float)atof(argv[++i]); }
		else if (!CF_STRCMP(argv[i], "--exit-at") && i + 1 < argc) exit_at = (float)atof(argv[++i]);
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("CF + Box2D", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_set_fixed_timestep(60);

	s_build_scene();

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(s_fixed_update);
		t += CF_DELTA_TIME;

		// The mouse in physics meters: 2d-draw world coordinates over the meter scale.
		CF_V2 mouse = cf_screen_to_world(cf_v2(cf_mouse_x(), cf_mouse_y()));
		CF_V2 mouse_m = cf_div_v2_f(mouse, PIXELS_PER_METER);

		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			if (g_body_count & 1) s_make_ball(mouse_m, 0.4f);
			else s_make_box(mouse_m, 0.5f, 0.5f, true);
		}
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT)) {
			b2ExplosionDef boom = b2DefaultExplosionDef();
			boom.position = cf_v2_to_b2(mouse_m);
			boom.radius = 3.0f;
			boom.impulsePerLength = 4.0f;
			b2World_Explode(g_world, &boom);
		}
		if (cf_key_just_pressed(CF_KEY_SPACE)) s_build_scene();

		// Draw the world in physics units under a meter-scaled camera.
		cf_draw_push();
		cf_draw_scale(PIXELS_PER_METER, PIXELS_PER_METER);
		cf_physics_draw(g_world, 0.04f);
		cf_draw_pop();

		// HUD.
		int w, h;
		cf_app_get_size(&w, &h);
		char hud[128];
		snprintf(hud, sizeof(hud), "%d bodies -- LMB drop, RMB explode, space reset", g_body_count);
		cf_push_font_size(18);
		cf_draw_push_color(cf_make_color_rgba_f(1, 1, 1, 0.75f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 16, (float)h * 0.5f - 20), -1);
		cf_draw_pop_color();
		cf_pop_font_size();

		cf_app_draw_onto_screen(true);

		for (int i = 0; i < shot_n; ++i) {
			if (shot_times[i] >= 0 && t >= shot_times[i]) {
				s_screenshot();
				shot_times[i] = -1.0f;
			}
		}
		if (exit_at > 0 && t >= exit_at) break;
	}

	b2DestroyWorld(g_world);
	cf_destroy_app();
	return 0;
}
