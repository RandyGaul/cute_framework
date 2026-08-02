/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Translucency in 3d, the honest version: alpha blending is ORDER-DEPENDENT, the depth
// buffer cannot fix that, and no framework can hide it -- so this sample shows the standard
// discipline and lets you toggle it off to watch it break (press SPACE).
//
//   1. Opaque geometry draws first, depth writes ON. Order irrelevant; depth sorts it.
//   2. Translucent geometry draws after, depth TEST on but depth WRITES OFF, alpha-blended,
//      submitted back-to-front along the view direction, re-sorted every frame because
//      "back" is wherever the camera isn't.
//
// Two draw3d details do the heavy lifting:
//
//   - Submission order is paint order. Opaque-then-translucent needs nothing special --
//     just submit in that order (layers could do it too, but order already does).
//   - Sorting SURVIVES coalescing. All twenty panes are one instanced draw, and instances
//     within a draw blend in submission order -- so sorting the submissions is sorting the
//     blend order, with no draw-call cost at all.
//
// With sorting off, panes blend in storage order: whenever a near pane happens to draw
// before a far one, the far one blends OVER it -- watch the colors and edges pop as the
// camera orbits. That shimmer is the bug this sample teaches you to never ship.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

#define PANES 20

static const char* s_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 12) in vec4 in_nmat0;\n"
"layout (location = 13) in vec4 in_nmat1;\n"
"layout (location = 14) in vec4 in_nmat2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec4 v_tint;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),\n"
"                              dot(in_nmat1.xyz, in_normal),\n"
"                              dot(in_nmat2.xyz, in_normal)));\n"
"    v_tint = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

// Simple lambert; alpha comes straight from the per-instance tint. Note the rgb is
// premultiplied by alpha in the shader so the blend state below can use ONE /
// ONE_MINUS_SRC_ALPHA -- the same premultiplied convention the 2d renderer uses.
static const char* s_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec4 v_tint;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    float ndl = max(dot(n, normalize(vec3(0.4, 0.8, 0.45))), 0.0);\n"
"    float two_side = max(dot(-n, normalize(vec3(0.4, 0.8, 0.45))), 0.0);\n"
"    vec3 lit = v_tint.rgb * (0.35 + 0.65 * max(ndl, two_side * 0.6));\n"
"    result = vec4(lit * v_tint.a, v_tint.a);\n"
"}\n";

//--------------------------------------------------------------------------------------------------

typedef struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
} Vertex;

static CF_Mesh s_make_cube(void)
{
	struct Face { CF_V3 c[4]; CF_V3 n; };
	struct Face faces[6] = {
		{ { { -1,-1, 1 }, {  1,-1, 1 }, {  1, 1, 1 }, { -1, 1, 1 } }, {  0, 0, 1 } },
		{ { {  1,-1,-1 }, { -1,-1,-1 }, { -1, 1,-1 }, {  1, 1,-1 } }, {  0, 0,-1 } },
		{ { {  1,-1, 1 }, {  1,-1,-1 }, {  1, 1,-1 }, {  1, 1, 1 } }, {  1, 0, 0 } },
		{ { { -1,-1,-1 }, { -1,-1, 1 }, { -1, 1, 1 }, { -1, 1,-1 } }, { -1, 0, 0 } },
		{ { { -1, 1, 1 }, {  1, 1, 1 }, {  1, 1,-1 }, { -1, 1,-1 } }, {  0, 1, 0 } },
		{ { { -1,-1,-1 }, {  1,-1,-1 }, {  1,-1, 1 }, { -1,-1, 1 } }, {  0,-1, 0 } },
	};
	Vertex verts[36];
	for (int f = 0; f < 6; ++f) {
		int idx[6] = { 0, 1, 2, 0, 2, 3 };
		for (int i = 0; i < 6; ++i) {
			verts[f * 6 + i].pos = faces[f].c[idx[i]];
			verts[f * 6 + i].n = faces[f].n;
		}
	}
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(Vertex, n);
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 2, (int)sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 36);
	return mesh;
}

typedef struct Pane
{
	CF_V3 at;
	float spin;
	CF_V4 tint;
	float dist; // View distance this frame, the sort key.
} Pane;

static int s_pane_cmp(const void* a, const void* b)
{
	const Pane* pa = (const Pane*)a;
	const Pane* pb = (const Pane*)b;
	return pa->dist < pb->dist ? 1 : (pa->dist > pb->dist ? -1 : 0); // Far first.
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_draw3d -- transparency", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.09f, 0.10f, 0.13f));

	CF_Mesh cube = s_make_cube();
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);

	// Translucent state: depth TEST on (opaque geometry still occludes panes), depth WRITES
	// off (panes must not occlude each other -- blending is doing that job), premultiplied
	// alpha blend, no culling so both faces of each pane show.
	CF_RenderState glass_rs = cf_render_state_3d_defaults();
	glass_rs.cull_mode = CF_CULL_MODE_NONE;
	glass_rs.depth_write_enabled = false;
	glass_rs.blend.enabled = true;
	glass_rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	glass_rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	glass_rs.blend.rgb_op = CF_BLEND_OP_ADD;
	glass_rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	glass_rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	glass_rs.blend.alpha_op = CF_BLEND_OP_ADD;

	// A ring of tinted panes around two opaque pillars and a floor slab.
	Pane panes[PANES];
	CF_Rnd rnd = cf_rnd_seed(9);
	for (int i = 0; i < PANES; ++i) {
		float a = (float)i / PANES * 2.0f * CF_PI;
		float r = 2.6f + 1.5f * (i & 1);
		panes[i].at = cf_v3(CF_COSF(a) * r, 1.3f + 0.5f * CF_SINF(a * 3.0f), CF_SINF(a) * r);
		panes[i].spin = a;
		float hue = (float)i / PANES;
		panes[i].tint = cf_v4(
			0.5f + 0.5f * CF_SINF(hue * 6.28f),
			0.5f + 0.5f * CF_SINF(hue * 6.28f + 2.1f),
			0.5f + 0.5f * CF_SINF(hue * 6.28f + 4.2f),
			cf_rnd_range_float(&rnd, 0.35f, 0.55f));
	}

	bool sorted = true;
	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;
		if (cf_key_just_pressed(CF_KEY_SPACE)) sorted = !sorted;

		int w, h;
		cf_app_get_size(&w, &h);
		float ca = t * 0.22f;
		CF_V3 eye = cf_v3(CF_SINF(ca) * 9.0f, 3.4f, CF_COSF(ca) * 9.0f);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.4f, (float)w / (float)h, 0.1f, 60.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 1.2f, 0), cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(shader);

		// 1) Opaque first: floor and pillars, default render state, any order.
		struct { CF_V3 at, scale; CF_V4 tint; } opaques[3] = {
			{ { 0, -0.25f, 0 }, { 7.0f, 0.25f, 7.0f }, { 0.28f, 0.28f, 0.32f, 1 } },
			{ { -1.1f, 1.1f, 0 }, { 0.45f, 1.35f, 0.45f }, { 0.75f, 0.62f, 0.4f, 1 } },
			{ { 1.2f, 0.85f, 0.4f }, { 0.45f, 1.1f, 0.45f }, { 0.5f, 0.65f, 0.75f, 1 } },
		};
		for (int i = 0; i < 3; ++i) {
			cf_draw3d_push();
			cf_draw3d_translate(opaques[i].at);
			cf_draw3d_scale(opaques[i].scale);
			cf_draw3d_push_mesh_attributes(opaques[i].tint);
			cf_draw3d_mesh(cube);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}

		// 2) Translucent second: sort back-to-front along the view, then submit in that
		// order. The panes still coalesce into ONE instanced draw -- instances blend in
		// submission order, so the sort costs nothing on the GPU side.
		for (int i = 0; i < PANES; ++i) {
			CF_V3 d = cf_sub_v3(panes[i].at, eye);
			panes[i].dist = cf_dot_v3(d, d);
		}
		if (sorted) qsort(panes, PANES, sizeof(Pane), s_pane_cmp);

		cf_draw3d_push_render_state(glass_rs);
		for (int i = 0; i < PANES; ++i) {
			cf_draw3d_push();
			cf_draw3d_translate(panes[i].at);
			cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0, 1, 0), panes[i].spin + t * 0.3f));
			cf_draw3d_scale(cf_v3(0.9f, 1.1f, 0.05f));
			cf_draw3d_push_mesh_attributes(panes[i].tint);
			cf_draw3d_mesh(cube);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}
		cf_draw3d_pop_render_state();

		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// The HUD rides the same command stream.
		char hud[256];
		snprintf(hud, sizeof(hud),
			"%d translucent panes, one instanced draw -- blended in submission order.\n"
			"Back-to-front sorting: %s   (SPACE to toggle and watch it break)",
			PANES, sorted ? "ON" : "OFF");
		cf_draw_push_color(cf_make_color_rgb_f(0.85f, 0.87f, 0.92f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_shader(shader);
	cf_destroy_mesh(cube);
	cf_destroy_app();
	return 0;
}
