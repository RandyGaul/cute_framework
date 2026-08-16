/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Billboards through cute_draw3d.h: a night forest of sprite-textured, camera-facing quads.
//
// Two flavors of billboard live here, and neither needs a sorting pass:
//
//   - Trees: CYLINDRICAL billboards (they turn to face the camera around y only, so they
//     never tilt backwards) using alpha CUTOUT -- the shader discards transparent texels, so
//     depth writes stay on and the depth buffer orders them like opaque geometry.
//   - Fireflies: SPHERICAL billboards (fully camera-facing) using ADDITIVE blending, which
//     is order-independent by construction -- depth writes off, no sort, no artifacts.
//
// Sorted alpha blending -- the third flavor, the one that DOES need ordering -- has its own
// sample: see transparency3d.c.
//
// The texturing is the part worth noticing: every tree pushes its sprite through
// cf_draw3d_push_texture, the images live wherever CF's texture atlas decides, and the
// sub-rect rides the in_uv_rect instance lane -- so three different tree images across one
// hundred billboards still coalesce toward a single instanced draw as the atlas settles.
// There is no atlas API in this file, and no billboard API either: a billboard is just a
// quad whose transform is built from the camera axes before submission.

#include <cute.h>
#include <stdio.h>

#define TREES 120
#define FLIES 70

//--------------------------------------------------------------------------------------------------
// Shaders.

// Shared vertex stage: the draw3d contract plus uvs remapped by the atlas sub-rect.
static const char* s_bill_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec2 in_uv;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 11) in vec4 in_uv_rect;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec2 v_uv;\n"
"layout (location = 1) out vec4 v_tint;\n"
"layout (location = 2) out float v_depth;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_uv = mix(in_uv_rect.xy, in_uv_rect.zw, in_uv);\n"
"    v_tint = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"    v_depth = gl_Position.w;\n"
"}\n";

// Trees: alpha cutout. Discarded texels never write depth, so the depth buffer handles
// ordering and the silhouettes stay crisp.
static const char* s_cutout_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 1) in vec4 v_tint;\n"
"layout (location = 2) in float v_depth;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    vec4 c = texture(u_image, v_uv);\n"
"    if (c.a < 0.5) discard;\n"
"    float fog = 1.0 - exp(-v_depth * 0.045);\n"
"    vec3 color = mix(c.rgb * v_tint.rgb, vec3(0.05, 0.06, 0.12), fog);\n"
"    result = vec4(color, 1.0);\n"
"}\n";

// Fireflies: additive glow. a + b == b + a, so draw order cannot matter.
static const char* s_glow_fs =
"layout (location = 0) in vec2 v_uv;\n"
"layout (location = 1) in vec4 v_tint;\n"
"layout (location = 2) in float v_depth;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform sampler2D u_image;\n"
"void main() {\n"
"    vec4 c = texture(u_image, v_uv);\n"
"    result = vec4(c.rgb * v_tint.rgb * (c.a * v_tint.a), 1.0);\n"
"}\n";

static const char* s_ground_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (location = 1) out float v_depth;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_color = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"    v_depth = gl_Position.w;\n"
"}\n";

static const char* s_ground_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 1) in float v_depth;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    float fog = 1.0 - exp(-v_depth * 0.045);\n"
"    result = vec4(mix(v_color.rgb, vec3(0.05, 0.06, 0.12), fog), 1.0);\n"
"}\n";

//--------------------------------------------------------------------------------------------------
// Procedural sprites: three pine silhouettes and one radial glow, fed to the atlas like any
// other sprite. A real game would load .ase or .png files; the mechanics are identical.

static CF_Sprite s_make_pine(uint64_t seed, int base_r, int base_g, int base_b)
{
	enum { W = 32, H = 48 };
	CF_Pixel px[W * H];
	CF_MEMSET(px, 0, sizeof(px));
	CF_Rnd rnd = cf_rnd_seed(seed);
	// Stacked, jittered layers of foliage over a stubby trunk. Row 0 is the sprite's top.
	for (int y = 4; y < H - 6; ++y) {
		float f = (float)(y - 4) / (float)(H - 10);
		int half = (int)(1.0f + f * (W / 2 - 3) * (0.75f + 0.25f * CF_SINF((float)y * 1.7f)));
		half += cf_rnd_range_int(&rnd, -1, 1);
		if (half < 1) half = 1;
		for (int x = W / 2 - half; x <= W / 2 + half; ++x) {
			float shade = 0.75f + 0.25f * cf_rnd_range_float(&rnd, 0, 1);
			px[y * W + x] = cf_make_pixel_rgba(
				(uint8_t)(base_r * shade), (uint8_t)(base_g * shade), (uint8_t)(base_b * shade), 255);
		}
	}
	for (int y = H - 6; y < H; ++y) {
		for (int x = W / 2 - 2; x <= W / 2 + 2; ++x) {
			px[y * W + x] = cf_make_pixel_rgb(58, 42, 30);
		}
	}
	return cf_make_easy_sprite_from_pixels(px, W, H);
}

static CF_Sprite s_make_glow(void)
{
	enum { N = 32 };
	CF_Pixel px[N * N];
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			float dx = (x + 0.5f - N / 2) / (N / 2), dy = (y + 0.5f - N / 2) / (N / 2);
			float d = CF_SQRTF(dx * dx + dy * dy);
			float a = cf_clamp(1.0f - d, 0.0f, 1.0f);
			a = a * a;
			px[y * N + x] = cf_make_pixel_rgba(255, 255, 255, (uint8_t)(a * 255.0f));
		}
	}
	return cf_make_easy_sprite_from_pixels(px, N, N);
}

//--------------------------------------------------------------------------------------------------

// A unit quad in the xz... no -- the xy plane, centered at the base (y in [0, 1]), so a
// billboard "stands" on its position. UVs put (0,0) at the sprite's top-left, matching 2d.
typedef struct Vertex
{
	CF_V3 pos;
	CF_V2 uv;
} Vertex;

static CF_Mesh s_make_quad(void)
{
	Vertex verts[6] = {
		{ { -0.5f, 0, 0 }, { 0, 1 } }, { { 0.5f, 0, 0 }, { 1, 1 } }, { { 0.5f, 1, 0 }, { 1, 0 } },
		{ { -0.5f, 0, 0 }, { 0, 1 } }, { { 0.5f, 1, 0 }, { 1, 0 } }, { { -0.5f, 1, 0 }, { 0, 0 } },
	};
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, uv);
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 2, (int)sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

static CF_Mesh s_make_floor(void)
{
	CF_V3 verts[6] = {
		{ -1, 0, -1 }, { -1, 0, 1 }, { 1, 0, 1 },
		{ -1, 0, -1 }, { 1, 0, 1 }, { 1, 0, -1 },
	};
	CF_VertexAttribute attrs[1] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = 0;
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 1, (int)sizeof(CF_V3));
	cf_mesh_update_vertex_data(mesh, verts, 6);
	return mesh;
}

// A basis matrix from explicit axes (column-major), for pointing quads at cameras.
static CF_M4x4 s_basis(CF_V3 x, CF_V3 y, CF_V3 z)
{
	CF_M4x4 m = cf_m4_identity();
	m.elements[0] = x.x; m.elements[1] = x.y; m.elements[2]  = x.z;
	m.elements[4] = y.x; m.elements[5] = y.y; m.elements[6]  = y.z;
	m.elements[8] = z.x; m.elements[9] = z.y; m.elements[10] = z.z;
	return m;
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_draw3d -- billboards", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.05f, 0.06f, 0.12f));

	// Sprites at 3d distances swim without mips; one call gives every atlased sprite a mip
	// chain, and anisotropy keeps the cylindrical trees sharp when viewed edge-on.
	cf_draw3d_mips(4);
	cf_draw3d_anisotropy(4);

	CF_Mesh quad = s_make_quad();
	CF_Mesh ground = s_make_floor();
	CF_Shader cutout_shd = cf_make_shader_from_source(s_bill_vs, s_cutout_fs);
	CF_Shader glow_shd = cf_make_shader_from_source(s_bill_vs, s_glow_fs);
	CF_Shader ground_shd = cf_make_shader_from_source(s_ground_vs, s_ground_fs);

	CF_Sprite pines[3];
	pines[0] = s_make_pine(3, 40, 96, 60);
	pines[1] = s_make_pine(7, 30, 82, 72);
	pines[2] = s_make_pine(12, 52, 104, 46);
	CF_Sprite glow = s_make_glow();

	// Cutout: depth writes stay ON (discard handles the holes), so no sorting ever.
	CF_RenderState cutout_rs = cf_render_state_3d_defaults();
	cutout_rs.cull_mode = CF_CULL_MODE_NONE;

	// Additive: blend ONE/ONE, depth test on but writes OFF -- glows never occlude.
	CF_RenderState glow_rs = cf_render_state_3d_defaults();
	glow_rs.cull_mode = CF_CULL_MODE_NONE;
	glow_rs.depth_write_enabled = false;
	glow_rs.blend.enabled = true;
	glow_rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	glow_rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
	glow_rs.blend.rgb_op = CF_BLEND_OP_ADD;
	glow_rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	glow_rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	glow_rs.blend.alpha_op = CF_BLEND_OP_ADD;

	// The forest.
	typedef struct Tree { CF_V3 at; float h; int kind; CF_V4 tint; } Tree;
	Tree trees[TREES];
	CF_Rnd rnd = cf_rnd_seed(5);
	for (int i = 0; i < TREES; ++i) {
		float a = cf_rnd_range_float(&rnd, 0, 2.0f * CF_PI);
		float r = CF_SQRTF(cf_rnd_range_float(&rnd, 0.02f, 1.0f)) * 16.0f;
		trees[i].at = cf_v3(CF_COSF(a) * r, 0, CF_SINF(a) * r);
		trees[i].h = cf_rnd_range_float(&rnd, 1.6f, 3.4f);
		trees[i].kind = cf_rnd_range_int(&rnd, 0, 2);
		float m = cf_rnd_range_float(&rnd, 0.6f, 1.0f);
		trees[i].tint = cf_v4(m, m, m, 1);
	}
	typedef struct Fly { CF_V3 at; float phase; CF_V4 tint; } Fly;
	Fly flies[FLIES];
	for (int i = 0; i < FLIES; ++i) {
		float a = cf_rnd_range_float(&rnd, 0, 2.0f * CF_PI);
		float r = CF_SQRTF(cf_rnd_range_float(&rnd, 0.02f, 1.0f)) * 14.0f;
		flies[i].at = cf_v3(CF_COSF(a) * r, cf_rnd_range_float(&rnd, 0.4f, 2.2f), CF_SINF(a) * r);
		flies[i].phase = cf_rnd_range_float(&rnd, 0, 6.28f);
		flies[i].tint = cf_v4(1.0f, cf_rnd_range_float(&rnd, 0.75f, 0.95f), 0.35f, 1.0f);
	}

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		int w, h;
		cf_app_get_size(&w, &h);
		float ca = t * 0.1f;
		CF_V3 eye = cf_v3(CF_SINF(ca) * 10.0f, 2.2f + 0.4f * CF_SINF(t * 0.23f), CF_COSF(ca) * 10.0f);
		CF_M4x4 view = cf_look_at(eye, cf_v3(0, 1.2f, 0), cf_v3(0, 1, 0));
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.2f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(view);

		// Camera axes for spherical billboards, straight out of the view matrix's rows.
		CF_V3 cam_right = cf_v3(view.elements[0], view.elements[4], view.elements[8]);
		CF_V3 cam_up = cf_v3(view.elements[1], view.elements[5], view.elements[9]);

		// Ground.
		cf_draw3d_push_shader(ground_shd);
		cf_draw3d_push();
		cf_draw3d_scale(cf_v3(40.0f, 1.0f, 40.0f));
		cf_draw3d_push_mesh_attributes(cf_v4(0.08f, 0.12f, 0.09f, 1));
		cf_draw3d_mesh(ground);
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop();
		cf_draw3d_pop_shader();

		// Trees: cylindrical billboards. Aim each quad at the camera around y only, push its
		// sprite, submit. Different sprites do NOT split the instanced draw -- images resolve
		// through the atlas, and drawing them together is what teaches the atlas to pack
		// them onto one page.
		cf_draw3d_push_shader(cutout_shd);
		cf_draw3d_push_render_state(cutout_rs);
		for (int i = 0; i < TREES; ++i) {
			CF_V3 to_cam = cf_sub_v3(eye, trees[i].at);
			to_cam.y = 0;
			CF_V3 fwd = cf_norm_v3(to_cam);
			CF_V3 up = cf_v3(0, 1, 0);
			CF_V3 right = cf_cross_v3(up, fwd);
			cf_draw3d_push();
			cf_draw3d_translate(trees[i].at);
			cf_draw3d_transform(s_basis(right, up, fwd));
			cf_draw3d_scale(cf_v3(trees[i].h * 0.66f, trees[i].h, 1.0f));
			cf_draw3d_push_mesh_attributes(trees[i].tint);
			cf_draw3d_push_texture(&pines[trees[i].kind]);
			cf_draw3d_mesh(quad);
			cf_draw3d_pop_texture();
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}
		cf_draw3d_pop_render_state();
		cf_draw3d_pop_shader();

		// Fireflies: spherical billboards, additive, bobbing on their phase.
		cf_draw3d_push_shader(glow_shd);
		cf_draw3d_push_render_state(glow_rs);
		for (int i = 0; i < FLIES; ++i) {
			CF_V3 at = flies[i].at;
			at.y += 0.25f * CF_SINF(t * 1.3f + flies[i].phase);
			at.x += 0.15f * CF_SINF(t * 0.7f + flies[i].phase * 2.0f);
			float pulse = 0.55f + 0.45f * CF_SINF(t * 2.1f + flies[i].phase);
			CF_V4 tint = flies[i].tint;
			tint.w = pulse;
			cf_draw3d_push();
			cf_draw3d_translate(at);
			cf_draw3d_transform(s_basis(cam_right, cam_up, cf_norm_v3(cf_sub_v3(eye, at))));
			cf_draw3d_scale(cf_v3(0.3f));
			cf_draw3d_push_mesh_attributes(tint);
			cf_draw3d_push_texture(&glow);
			cf_draw3d_mesh(quad);
			cf_draw3d_pop_texture();
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
			"%d cutout trees (3 sprites, one atlas) + %d additive fireflies.\n"
			"Billboards are just quads aimed at the camera; sprites texture them through the atlas.",
			TREES, FLIES);
		cf_draw_push_color(cf_make_color_rgb_f(0.8f, 0.85f, 0.9f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_shader(cutout_shd);
	cf_destroy_shader(glow_shd);
	cf_destroy_shader(ground_shd);
	cf_easy_sprite_unload(&pines[0]);
	cf_easy_sprite_unload(&pines[1]);
	cf_easy_sprite_unload(&pines[2]);
	cf_easy_sprite_unload(&glow);
	cf_destroy_mesh(quad);
	cf_destroy_mesh(ground);
	cf_destroy_app();
	return 0;
}
