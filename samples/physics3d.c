/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Box3D through CF: a tower of boxes, spheres, and a capsule on a ground slab, stepped on
// CF's fixed timestep and rendered with cf_physics_draw3 -- the 3d wiring described in
// cute_physics.h in one place.
//
//   - The world is created from cf_physics_world_def3, which wires the debug-shape bake
//     callbacks so cf_physics_draw3 can render hulls.
//   - Physics ticks inside the CF_OnUpdateFn callback via cf_physics_step3.
//   - Rendering is draw3d's shader-free built-ins: spheres and capsules draw as
//     hemisphere-lit solids, hulls as anti-aliased wireframes, under an orbit camera.
//
// Left click drops a shape along the camera ray, right click sets off an explosion at the
// tower, space resets. Drag to orbit, mouse wheel to zoom.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

b3WorldId g_world;
int g_body_count;

static void s_make_box(CF_V3 pos, CF_V3 half_extents, bool dynamic)
{
	b3BodyDef body_def = b3DefaultBodyDef();
	body_def.type = dynamic ? b3_dynamicBody : b3_staticBody;
	body_def.position = cf_v3_to_b3(pos);
	b3BodyId body = b3CreateBody(g_world, &body_def);
	b3ShapeDef shape_def = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull(half_extents.x, half_extents.y, half_extents.z);
	b3CreateHullShape(body, &shape_def, &box.base);
	g_body_count++;
}

static void s_make_ball(CF_V3 pos, float r)
{
	b3BodyDef body_def = b3DefaultBodyDef();
	body_def.type = b3_dynamicBody;
	body_def.position = cf_v3_to_b3(pos);
	b3BodyId body = b3CreateBody(g_world, &body_def);
	b3ShapeDef shape_def = b3DefaultShapeDef();
	shape_def.baseMaterial.restitution = 0.5f;
	b3Sphere sphere = { { 0, 0, 0 }, r };
	b3CreateSphereShape(body, &shape_def, &sphere);
	g_body_count++;
}

static void s_build_scene(void)
{
	if (B3_IS_NON_NULL(g_world)) b3DestroyWorld(g_world);
	g_body_count = 0;

	// cf_physics_world_def3 wires CF's debug-shape callbacks; everything else is ordinary.
	b3WorldDef world_def = cf_physics_world_def3();
	world_def.gravity.y = -10.0f;
	g_world = b3CreateWorld(&world_def);

	// Ground slab.
	s_make_box(cf_v3(0, -0.5f, 0), cf_v3(12.0f, 0.5f, 12.0f), false);

	// A tower of boxes with a slight twist per level.
	for (int level = 0; level < 10; ++level) {
		float y = 0.5f + level * 1.01f;
		s_make_box(cf_v3(0, y, 0), cf_v3(0.5f, 0.5f, 0.5f), true);
		s_make_box(cf_v3(1.05f, y, 0), cf_v3(0.5f, 0.5f, 0.5f), true);
		s_make_box(cf_v3(-1.05f, y, 0), cf_v3(0.5f, 0.5f, 0.5f), true);
	}

	// A few spheres resting nearby, and one capsule leaning on the tower.
	s_make_ball(cf_v3(3.0f, 0.4f, 2.0f), 0.4f);
	s_make_ball(cf_v3(-2.5f, 0.5f, -2.0f), 0.5f);

	b3BodyDef cap_def = b3DefaultBodyDef();
	cap_def.type = b3_dynamicBody;
	cap_def.position = cf_v3_to_b3(cf_v3(2.2f, 2.0f, 0.5f));
	b3BodyId cap_body = b3CreateBody(g_world, &cap_def);
	b3ShapeDef cap_shape = b3DefaultShapeDef();
	b3Capsule capsule = { { 0, -0.8f, 0 }, { 0, 0.8f, 0 }, 0.35f };
	b3CreateCapsuleShape(cap_body, &cap_shape, &capsule);
	g_body_count++;
}

// Physics ticks on CF's fixed clock: this runs exactly once per fixed update.
static void s_fixed_update(void* udata)
{
	(void)udata;
	cf_physics_step3(g_world, 4);
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
		snprintf(path, sizeof(path), "/physics3d_shot_%02d.png", s_shot_count++);
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
	CF_Result result = cf_make_app("CF + Box3D", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_set_fixed_timestep(60);
	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.10f, 0.11f, 0.13f));

	s_build_scene();

	// Orbit camera.
	float azimuth = 0.8f, elevation = 0.45f, distance = 18.0f;
	bool dragging = false;
	float last_mx = 0, last_my = 0;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(s_fixed_update);
		t += CF_DELTA_TIME;

		float mx = cf_mouse_x(), my = cf_mouse_y();
		if (cf_mouse_down(CF_MOUSE_BUTTON_LEFT) && !cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			if (dragging) {
				azimuth -= (mx - last_mx) * 0.008f;
				elevation = cf_clamp(elevation + (my - last_my) * 0.008f, -1.4f, 1.4f);
			}
			dragging = true;
		} else {
			dragging = false;
		}
		last_mx = mx;
		last_my = my;
		distance = cf_clamp(distance - cf_mouse_wheel_motion() * 1.2f, 6.0f, 40.0f);
		if (cf_key_just_pressed(CF_KEY_SPACE)) s_build_scene();

		int w, h;
		cf_app_get_size(&w, &h);
		float ce = CF_COSF(elevation);
		CF_V3 eye = cf_v3(CF_SINF(azimuth) * ce * distance, 2.0f + CF_SINF(elevation) * distance, CF_COSF(azimuth) * ce * distance);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.6f, (float)w / (float)h, 0.1f, 200.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 2.0f, 0), cf_v3(0, 1, 0)));

		// Drop a shape along the camera ray, or blow the tower up.
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			CF_V3 origin, dir;
			cf_draw3d_unproject(cf_screen_to_world(cf_v2(mx, my)), &origin, &dir);
			CF_V3 spawn = cf_add_v3(origin, cf_mul_v3_f(dir, 10.0f));
			if (g_body_count & 1) s_make_ball(spawn, 0.4f);
			else s_make_box(spawn, cf_v3(0.5f, 0.5f, 0.5f), true);
		}
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT)) {
			b3ExplosionDef boom = b3DefaultExplosionDef();
			boom.position = cf_v3_to_b3(cf_v3(0, 1.0f, 0));
			boom.radius = 5.0f;
			boom.impulsePerArea = 6.0f;
			b3World_Explode(g_world, &boom);
		}

		// The whole world, one call: solids for spheres/capsules, wireframes for hulls.
		cf_physics_draw3(g_world, 0.02f);

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// HUD.
		char hud[128];
		snprintf(hud, sizeof(hud), "%d bodies -- LMB drop, RMB explode, space reset, drag to orbit", g_body_count);
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

	b3DestroyWorld(g_world);
	cf_destroy_app();
	return 0;
}
