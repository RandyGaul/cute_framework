/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// High-quality 3d line drawing: icosahedron cages with every vertex-to-vertex chord,
// black strokes on a white page. Each stroke is a signed-distance ribbon: locally
// anti-aliased with no MSAA, and hairline chords -- thinner than a pixel for most of
// the orbit -- clamp to half-pixel width and fade alpha instead of shimmering. A row
// of cages recedes into the distance, where every stroke is sub-pixel: they should
// melt into uniform grey, never speckle. The dashed ring shows the dash stack.

#include <cute.h>
#include <float.h>

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("3D Lines", 0, 0, 0, 960, 540, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);

	// Icosahedron vertices on the unit sphere: cyclic permutations of (0, ±1, ±phi).
	const float phi = 1.6180339887f;
	CF_V3 v[12];
	int n = 0;
	for (int s0 = -1; s0 <= 1; s0 += 2) {
		for (int s1 = -1; s1 <= 1; s1 += 2) {
			v[n++] = cf_norm_v3(cf_v3(0, (float)s0, (float)s1 * phi));
			v[n++] = cf_norm_v3(cf_v3((float)s0, (float)s1 * phi, 0));
			v[n++] = cf_norm_v3(cf_v3((float)s0 * phi, 0, (float)s1));
		}
	}

	// The 30 outline edges are exactly the closest vertex pairs; everything else is a chord.
	float edge_len = FLT_MAX;
	for (int i = 0; i < 12; ++i) {
		for (int j = i + 1; j < 12; ++j) {
			edge_len = cf_min(edge_len, cf_len_v3(cf_sub_v3(v[i], v[j])));
		}
	}

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		cf_clear_color(1, 1, 1, 1);

		int w, h;
		cf_app_get_size(&w, &h);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 5.0f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(cf_v3(0, 0.6f, 4.2f), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));

		// The hero cage, then a row receding into the distance -- every stroke on the far
		// ones lands deep sub-pixel, the acid test for thin-stroke fading.
		cf_draw3d_push_color(cf_color_black());
		for (int c = 0; c < 5; ++c) {
			cf_draw3d_push();
			cf_draw3d_translate(cf_v3(c * 1.4f, 0, c * -7.0f));
			cf_draw3d_rotate(cf_quat_from_axis_angle(cf_norm_v3(cf_v3(0.3f, 1.0f, 0.15f)), t * 0.3f + c));
			for (int i = 0; i < 12; ++i) {
				for (int j = i + 1; j < 12; ++j) {
					float len = cf_len_v3(cf_sub_v3(v[i], v[j]));
					if (len < edge_len * 1.05f) {
						// Outline edge: full-weight ink.
						cf_draw3d_line(v[i], v[j], 0.016f);
					} else {
						// Interior chord: hairlines of varying weight, most under a pixel wide.
						float k = (float)(((i * 31 + j * 17) % 7)) / 6.0f;
						cf_draw3d_line(v[i], v[j], 0.001f + k * 0.007f);
					}
				}
			}
			cf_draw3d_pop();
		}

		// A dashed orbit ring around the hero cage, phase-animated for marching ants.
		cf_draw3d_push_dash(0.12f, 0.08f, t * 0.15f);
		cf_draw3d_circle(cf_v3(0, 0, 0), cf_v3(0.25f, 1, 0), 1.35f, 0.012f);
		cf_draw3d_pop_dash();
		cf_draw3d_pop_color();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
