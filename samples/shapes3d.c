/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// 3d shape drawing through cute_draw3d.h: lines, circles, arcs, and solid primitives with no
// shader, mesh, or material setup -- the 3d analog of cute_draw.h's immediate-mode shapes.
//
// Everything here is anti-aliased signed-distance rendering. Strokes expand to camera-facing
// ribbons with a fragment-shader distance field, so edges stay smooth at any zoom without
// MSAA, and strokes thinner than a pixel fade out instead of shimmering -- watch the grid
// recede toward the horizon, or the fan of hairlines on the left. Thickness is in world
// units: lines get thinner with distance like real geometry, not like screen overlays.
//
// Solids (cube, sphere, cone, torus) draw with a built-in hemisphere light and the color
// stack. Strokes batch: the whole grid is one instanced draw.

#include <cute.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("3D Shapes", 0, 0, 0, 960, 540, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// Slow orbit camera.
		float ang = t * 0.25f;
		CF_V3 eye = cf_v3(cosf(ang) * 7.0f, 3.2f, sinf(ang) * 7.0f);
		int w, h;
		cf_app_get_size(&w, &h);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 4.0f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 0.5f, 0), cf_v3(0, 1, 0)));

		// Ground grid -- one batched instanced draw. Distant lines drop below a pixel wide
		// and fade instead of aliasing.
		cf_draw3d_push_color(cf_make_color_rgb_f(0.25f, 0.28f, 0.33f));
		for (int i = -10; i <= 10; ++i) {
			cf_draw3d_line(cf_v3((float)i, 0, -10), cf_v3((float)i, 0, 10), 0.02f);
			cf_draw3d_line(cf_v3(-10, 0, (float)i), cf_v3(10, 0, (float)i), 0.02f);
		}
		cf_draw3d_pop_color();

		// Origin axes gizmo (red/green/blue = x/y/z).
		cf_draw3d_axes(1.0f, 0.045f);

		// Solids on a ring of pedestals, each with its bounding wire box.
		struct { CF_Color color; } spots[4] = {
			{ { 0.95f, 0.45f, 0.35f, 1 } },
			{ { 0.40f, 0.75f, 0.95f, 1 } },
			{ { 0.55f, 0.90f, 0.45f, 1 } },
			{ { 0.95f, 0.80f, 0.35f, 1 } },
		};
		for (int i = 0; i < 4; ++i) {
			float a = CF_PI * 0.5f * (float)i;
			CF_V3 at = cf_v3(cosf(a) * 3.0f, 0.6f, sinf(a) * 3.0f);
			float bob = sinf(t * 1.5f + (float)i) * 0.1f;
			at.y += bob;
			cf_draw3d_push_color(spots[i].color);
			switch (i) {
			case 0: cf_draw3d_cube(at, cf_v3(0.45f, 0.45f, 0.45f)); break;
			case 1: cf_draw3d_sphere(at, 0.5f); break;
			case 2: cf_draw3d_cone(cf_v3(at.x, at.y - 0.45f, at.z), cf_v3(at.x, at.y + 0.55f, at.z), 0.45f); break;
			case 3: cf_draw3d_torus(at, cf_v3(0, 1, 0), 0.42f, 0.16f); break;
			}
			cf_draw3d_pop_color();
			cf_draw3d_push_color(cf_make_color_rgb_f(0.9f, 0.9f, 0.9f));
			cf_draw3d_box_wire(at, cf_v3(0.6f, 0.6f, 0.6f), 0.012f);
			cf_draw3d_pop_color();
			// Soft "shadow" disc on the ground.
			cf_draw3d_push_color(cf_make_color_rgba_f(0, 0, 0, 0.35f));
			cf_draw3d_circle_fill(cf_v3(at.x, 0.01f, at.z), cf_v3(0, 1, 0), 0.55f + bob * 0.3f);
			cf_draw3d_pop_color();
		}

		// A rotation-gizmo of three arcs around the center, spinning at different rates.
		CF_V3 hub = cf_v3(0, 1.4f, 0);
		cf_draw3d_push_color(cf_make_color_rgb_f(0.95f, 0.35f, 0.40f));
		cf_draw3d_arc(hub, cf_v3(1, 0, 0), 0.8f, t, 4.2f, 0.035f);
		cf_draw3d_pop_color();
		cf_draw3d_push_color(cf_make_color_rgb_f(0.40f, 0.90f, 0.45f));
		cf_draw3d_arc(hub, cf_v3(0, 1, 0), 0.8f, -t * 1.3f, 4.2f, 0.035f);
		cf_draw3d_pop_color();
		cf_draw3d_push_color(cf_make_color_rgb_f(0.45f, 0.55f, 0.95f));
		cf_draw3d_arc(hub, cf_v3(0, 0, 1), 0.8f, t * 0.7f, 4.2f, 0.035f);
		cf_draw3d_pop_color();
		cf_draw3d_push_color(cf_make_color_rgb_f(0.9f, 0.9f, 0.9f));
		cf_draw3d_sphere(hub, 0.15f);
		cf_draw3d_pop_color();

		// An animated polyline ribbon -- a sine wave whirling around the scene edge.
		CF_V3 wave[48];
		for (int i = 0; i < 48; ++i) {
			float a = 2.0f * CF_PI * (float)i / 48.0f;
			wave[i] = cf_v3(cosf(a) * 4.6f, 0.8f + sinf(a * 6.0f + t * 2.0f) * 0.25f, sinf(a) * 4.6f);
		}
		cf_draw3d_push_color(cf_make_color_rgb_f(0.85f, 0.55f, 0.95f));
		cf_draw3d_polyline(wave, 48, 0.05f, true);
		cf_draw3d_pop_color();

		// Per-end thickness plus arrows: a fan of tapered hairlines, then an arrow tracking
		// the tallest wave crest.
		cf_draw3d_push_color(cf_make_color_rgb_f(0.95f, 0.9f, 0.8f));
		for (int i = 0; i < 7; ++i) {
			float lift = 0.35f + (float)i * 0.28f;
			cf_draw3d_line2(cf_v3(-4.5f, 0, -2.5f + (float)i * 0.8f), cf_v3(-2.2f, lift, -2.5f + (float)i * 0.8f), 0.06f, 0.004f);
		}
		cf_draw3d_pop_color();
		cf_draw3d_push_color(cf_make_color_rgb_f(0.35f, 0.9f, 0.9f));
		cf_draw3d_arrow(cf_v3(0, 2.6f, 0), cf_v3(cosf(t) * 1.5f, 2.2f, sinf(t) * 1.5f), 0.03f, 0.09f);
		cf_draw3d_pop_color();

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		cf_app_draw_onto_screen(true);

	}

	cf_destroy_app();
	return 0;
}
