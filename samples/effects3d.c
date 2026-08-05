/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Shape effects in 3d: outlines and glows that fall straight out of the signed distance every
// stroke already evaluates. No blur pass, no render target, no bloom -- the glow is exact at any
// distance, and the ribbon grows to fit it automatically.
//
// The scene is a neon sign: glowing tube letters, an outlined selection box that reads clearly
// against the busy grid behind it, and a pulsing rotation gizmo. Compare the glowing strokes to
// the plain ones on the ground grid -- same primitives, one push apart.
//
// Space toggles the effects off so you can see what they're doing.

#include <cute.h>
#include <stdio.h>

// A stroke path in the xy plane, drawn as a polyline. Each entry is a run of points.
typedef struct Stroke { int count; CF_V2 pts[8]; } Stroke;

// Block letters spelling "CF", in a 0..1 box per letter.
static const Stroke s_letters[] = {
	// C
	{ 4, { { 0.9f, 0.85f }, { 0.15f, 0.85f }, { 0.15f, 0.15f }, { 0.9f, 0.15f } } },
	// F
	{ 3, { { 0.15f, 0.15f }, { 0.15f, 0.85f }, { 0.85f, 0.85f } } },
	{ 2, { { 0.15f, 0.5f }, { 0.7f, 0.5f } } },
};
static const int s_letter_of[] = { 0, 1, 1 }; // Which letter each stroke belongs to.

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("3D Shape Effects", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.02f, 0.02f, 0.04f));

	bool effects_on = true;
	float t = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;
		if (cf_key_just_pressed(CF_KEY_SPACE)) effects_on = !effects_on;

		int w, h;
		cf_app_get_size(&w, &h);
		float ang = t * 0.15f;
		CF_V3 eye = cf_v3(cosf(ang) * 7.0f, 2.4f, sinf(ang) * 7.0f);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 4.0f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 0.9f, 0), cf_v3(0, 1, 0)));

		// Plain strokes for contrast: a dim ground grid with no effects at all.
		cf_draw3d_push_color(cf_make_color_rgb_f(0.16f, 0.17f, 0.22f));
		for (int i = -8; i <= 8; ++i) {
			cf_draw3d_line(cf_v3((float)i * 0.5f, 0, -4), cf_v3((float)i * 0.5f, 0, 4), 0.01f);
			cf_draw3d_line(cf_v3(-4, 0, (float)i * 0.5f), cf_v3(4, 0, (float)i * 0.5f), 0.01f);
		}
		cf_draw3d_pop_color();

		// The neon sign. The stroke itself is near-white; the glow supplies the color, which
		// is how real neon reads. Additive blending makes overlapping glows build up.
		float pulse = 0.75f + 0.25f * sinf(t * 2.2f);
		CF_RenderState neon = cf_render_state_3d_defaults();
		neon.blend.enabled = true;
		neon.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		neon.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
		neon.blend.rgb_op = CF_BLEND_OP_MAX;
		neon.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
		neon.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
		neon.blend.alpha_op = CF_BLEND_OP_MAX;
		cf_draw3d_push_render_state(neon);
		if (effects_on) {
			cf_draw3d_push_glow(cf_make_color_rgba_f(1.0f, 0.15f, 0.55f, pulse), 0.28f);
		}
		cf_draw3d_push_color(cf_make_color_rgb_f(1.0f, 0.85f, 0.95f));
		for (int s = 0; s < (int)(sizeof(s_letters) / sizeof(*s_letters)); ++s) {
			const Stroke* stroke = s_letters + s;
			CF_V3 pts[8];
			float x_offset = s_letter_of[s] * 1.15f - 1.15f;
			for (int i = 0; i < stroke->count; ++i) {
				pts[i] = cf_v3(x_offset + stroke->pts[i].x, 0.9f + stroke->pts[i].y * 1.1f, 0);
			}
			cf_draw3d_polyline(pts, stroke->count, 0.05f, false);
		}
		cf_draw3d_pop_color();
		if (effects_on) cf_draw3d_pop_glow();
		cf_draw3d_pop_render_state();

		// A selection box, outlined and self-occluding at once. Strokes don't write depth by
		// default -- an anti-aliased fringe that owned depth would punch halos into later draws
		// -- but that also stops a wireframe from occluding itself, so the box's back edges draw
		// over its front ones. Opting in fixes exactly that. cf_draw3d_box_wire draws all twelve
		// outlines before any core, so no corner's outline lands on a neighbor's core.
		cf_draw3d_push_stroke_depth_write(true);
		if (effects_on) {
			cf_draw3d_push_outline(cf_make_color_rgba_f(0.05f, 0.35f, 0.15f, 0.95f), 0.03f);
		}
		cf_draw3d_push_color(cf_make_color_rgb_f(0.5f, 1.0f, 0.7f));
		cf_draw3d_box_wire(cf_v3(2.4f, 0.65f, 0), cf_v3(0.6f, 0.6f, 0.6f), 0.012f);
		cf_draw3d_pop_color();
		if (effects_on) cf_draw3d_pop_outline();
		cf_draw3d_pop_stroke_depth_write();

		// Where an outline shines instead: a flat selection ring on the ground, its thin core
		// wrapped in a wider contrasting band. One closed shape, so there are no joints to fight
		// over, and no depth writes needed. The outline draws under the core, so you get a white
		// halo with the orange stroke riding on top -- pick a color that contrasts with the
		// *background*, not with the stroke.
		if (effects_on) {
			cf_draw3d_push_outline(cf_make_color_rgba_f(1.0f, 1.0f, 1.0f, 0.95f), 0.05f);
		}
		cf_draw3d_push_color(cf_make_color_rgb_f(1.0f, 0.5f, 0.05f));
		cf_draw3d_circle(cf_v3(2.4f, 0.02f, 0), cf_v3(0, 1, 0), 0.95f, 0.02f);
		cf_draw3d_pop_color();
		if (effects_on) cf_draw3d_pop_outline();

		// A rotation gizmo, also glowing. Glowing things want a lightening blend so they add to
		// whatever they overlap: a dark outline here would darken the neon behind it, and even
		// plain alpha blending lets a low-alpha glow edge dim what it covers. CF_BLEND_OP_MAX
		// takes the brighter of source and destination, so overlapping glows never darken and
		// never blow out the way pure additive does.
		CF_RenderState lighten = cf_render_state_3d_defaults();
		lighten.blend.enabled = true;
		lighten.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
		lighten.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
		lighten.blend.rgb_op = CF_BLEND_OP_MAX;
		lighten.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
		lighten.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
		lighten.blend.alpha_op = CF_BLEND_OP_MAX;
		cf_draw3d_push_render_state(lighten);
		if (effects_on) {
			cf_draw3d_push_glow(cf_make_color_rgba_f(0.3f, 0.7f, 1.0f, 0.8f), 0.18f);
		}
		cf_draw3d_push_color(cf_make_color_rgb_f(0.75f, 0.9f, 1.0f));
		CF_V3 hub = cf_v3(-2.4f, 0.9f, 0);
		cf_draw3d_circle(hub, cf_v3(0, 1, 0), 0.55f, 0.02f);
		cf_draw3d_arc(hub, cf_v3(1, 0, 0), 0.55f, t, 4.0f, 0.02f);
		cf_draw3d_arc(hub, cf_v3(0, 0, 1), 0.55f, -t * 1.3f, 4.0f, 0.02f);
		cf_draw3d_pop_color();
		if (effects_on) cf_draw3d_pop_glow();
		cf_draw3d_pop_render_state();

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		cf_push_font_size(20);
		cf_draw_push_color(cf_color_white());
		cf_draw_text(effects_on ? "effects on -- space to compare" : "effects off", cf_v2(-(float)w * 0.5f + 20, (float)h * 0.5f - 20), -1);
		cf_draw_pop_color();
		cf_pop_font_size();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	return 0;
}
