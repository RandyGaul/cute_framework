/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// A multi-pass 3d scene through cute_draw3d.h: a shadow-mapped world under an orbiting
// camera, with a 2d HUD riding the same command stream.
//
// What this shows, and where:
//
//   - Custom lighting as user code: both shaders below are plain cf_make_shader_from_source
//     shaders following the contract in cute_draw3d.h. CF supplies transforms and submission;
//     every lighting decision (lambert, bias, ambient) is in this file, not the framework.
//   - Multi-pass: pass 1 renders depth from the light into a canvas whose depth target is a
//     comparison sampler (hardware PCF); pass 2 samples it through sampler2DShadow.
//   - Baked draw lists: the level (ground slab + cube grid) records once at init and replays
//     as one instanced draw per pass, under whatever camera is live that frame. The ground
//     slab is non-uniformly scaled -- baked lists carry exact normal matrices, so its
//     lighting stays correct.
//   - Automatic instancing: the spinning cubes submit immediately every frame and coalesce.
//   - Sprite-textured meshes: one cube samples a sprite through the texture atlas via the
//     in_uv_rect instance lane -- no atlas API in sight.
//   - Shared stream: the HUD text draws through the ordinary 2d API in the same frame.

#include <cute.h>
#include <stdio.h>

using namespace Cute;

//--------------------------------------------------------------------------------------------------
// Shaders.

// Depth-only pass from the light's point of view.
static const char* s_shadow_vs = R"(
layout (location = 0) in vec3 in_pos;
layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
};
void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	gl_Position = u_view_projection * vec4(world, 1.0);
}
)";

static const char* s_shadow_fs = R"(
layout (location = 0) out vec4 result;
void main() { result = vec4(1.0); }
)";

// Lambert + shadow map. The vertex stage is the full shader contract; everything after
// gl_Position is user-space lighting.
static const char* s_lit_vs = R"(
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (location = 11) in vec4 in_uv_rect;
layout (location = 12) in vec4 in_nmat0;
layout (location = 13) in vec4 in_nmat1;
layout (location = 14) in vec4 in_nmat2;
layout (location = 15) in vec4 in_mesh_attributes;
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec4 v_light;
layout (location = 2) out vec4 v_tint;
layout (location = 3) out vec2 v_uv;
layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
	mat4 u_light_vp;
};
void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	v_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),
	                          dot(in_nmat1.xyz, in_normal),
	                          dot(in_nmat2.xyz, in_normal)));
	v_light = u_light_vp * vec4(world, 1.0);
	v_tint = in_mesh_attributes;
	v_uv = mix(in_uv_rect.xy, in_uv_rect.zw, in_uv);
	gl_Position = u_view_projection * vec4(world, 1.0);
}
)";

// u_textured switches the sprite-textured cube onto the same shader (the atlas page arrives
// as u_image, the sub-rect already mixed into v_uv by the vertex stage).
static const char* s_lit_fs = R"(
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec4 v_light;
layout (location = 2) in vec4 v_tint;
layout (location = 3) in vec2 v_uv;
layout (location = 0) out vec4 result;
layout (set = 2, binding = 0) uniform sampler2D u_image;
layout (set = 2, binding = 1) uniform sampler2DShadow u_shadow;
layout (set = 3, binding = 0) uniform uniform_block {
	vec3 u_light_dir;
	float u_textured;
};
void main()
{
	vec3 n = normalize(v_normal);
	float ndl = max(dot(n, -u_light_dir), 0.0);
	vec3 ndc = v_light.xyz / v_light.w;
	// Canvas textures sample with v = 0 at clip-space +y on every backend, hence the y flip.
	vec2 suv = vec2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	float lit = 1.0;
	if (suv.x >= 0.0 && suv.x <= 1.0 && suv.y >= 0.0 && suv.y <= 1.0) {
		lit = texture(u_shadow, vec3(suv, ndc.z - 0.0015));
	}
	vec4 albedo = v_tint;
	if (u_textured > 0.5) albedo *= texture(u_image, v_uv);
	result = vec4(albedo.rgb * (0.25 + 0.75 * ndl * lit), 1.0);
}
)";

//--------------------------------------------------------------------------------------------------
// A unit cube (half-extent 1), 36 vertices, CCW-wound for the default 3d render state.

struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
	CF_V2 uv;
};

static CF_Mesh s_make_cube()
{
	// Four corners + normal per face; corners wind CCW viewed from outside.
	struct Face { CF_V3 c[4]; CF_V3 n; };
	Face faces[6] = {
		{ { { -1,-1, 1 }, {  1,-1, 1 }, {  1, 1, 1 }, { -1, 1, 1 } }, {  0, 0, 1 } }, // +z
		{ { {  1,-1,-1 }, { -1,-1,-1 }, { -1, 1,-1 }, {  1, 1,-1 } }, {  0, 0,-1 } }, // -z
		{ { {  1,-1, 1 }, {  1,-1,-1 }, {  1, 1,-1 }, {  1, 1, 1 } }, {  1, 0, 0 } }, // +x
		{ { { -1,-1,-1 }, { -1,-1, 1 }, { -1, 1, 1 }, { -1, 1,-1 } }, { -1, 0, 0 } }, // -x
		{ { { -1, 1, 1 }, {  1, 1, 1 }, {  1, 1,-1 }, { -1, 1,-1 } }, {  0, 1, 0 } }, // +y
		{ { { -1,-1,-1 }, {  1,-1,-1 }, {  1,-1, 1 }, { -1,-1, 1 } }, {  0,-1, 0 } }, // -y
	};
	Vertex verts[36];
	for (int f = 0; f < 6; ++f) {
		int idx[6] = { 0, 1, 2, 0, 2, 3 };
		CF_V2 uvs[4] = { cf_v2(0, 1), cf_v2(1, 1), cf_v2(1, 0), cf_v2(0, 0) };
		for (int i = 0; i < 6; ++i) {
			Vertex& v = verts[f * 6 + i];
			v.pos = faces[f].c[idx[i]];
			v.n = faces[f].n;
			v.uv = uvs[idx[i]];
		}
	}
	CF_VertexAttribute attrs[3] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(Vertex, n);
	attrs[2].name = "in_uv";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(Vertex, uv);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 3, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 36);
	return mesh;
}

//--------------------------------------------------------------------------------------------------

// The level: a ground slab plus a ring of pillars, recorded under whatever shader is pushed.
// Called twice at init -- once under the shadow shader, once under the lit shader -- since a
// recorded submission captures its shader (each pass replays its own list).
static void s_record_level(CF_Mesh cube)
{
	// Ground slab: non-uniform scale, which is exactly what baked lists handle with exact
	// normal matrices (the immediate path would skew this normal).
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, -1.0f, 0));
	cf_draw3d_scale(cf_v3(11.0f, 0.25f, 11.0f));
	cf_draw3d_push_mesh_attributes(cf_v4(0.42f, 0.54f, 0.42f, 1));
	cf_draw3d_mesh(cube);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();

	// Pillars of varying height on a grid, tinted per instance through mesh attributes --
	// different tints never split the instanced draw.
	CF_Rnd rnd = cf_rnd_seed(42);
	for (int i = -3; i <= 3; ++i) {
		for (int j = -3; j <= 3; ++j) {
			if (i > -2 && i < 2 && j > -2 && j < 2) continue; // Keep the center clear for the spinners.
			float h = cf_rnd_range_float(&rnd, 0.3f, 1.6f);
			cf_draw3d_push();
			cf_draw3d_translate(cf_v3(i * 2.6f, h - 0.75f, j * 2.6f));
			cf_draw3d_scale(cf_v3(0.5f, h, 0.5f));
			cf_draw3d_push_mesh_attributes(cf_v4(
				cf_rnd_range_float(&rnd, 0.4f, 0.9f),
				cf_rnd_range_float(&rnd, 0.4f, 0.9f),
				cf_rnd_range_float(&rnd, 0.4f, 0.9f), 1));
			cf_draw3d_mesh(cube);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}
	}
}

// The spinning centerpiece cubes, submitted immediately every frame (consecutive same-state
// submissions coalesce on their own). The textured one rides the same lit shader with
// u_textured flipped on and a sprite pushed.
static void s_submit_spinners(CF_Mesh cube, float t, const CF_Sprite* sprite, bool lit_pass)
{
	CF_V4 tints[3] = { cf_v4(0.95f, 0.45f, 0.35f, 1), cf_v4(0.4f, 0.6f, 0.95f, 1), cf_v4(0.95f, 0.85f, 0.4f, 1) };
	for (int i = 0; i < 3; ++i) {
		float a = t * (0.6f + 0.2f * i) + i * 2.1f;
		cf_draw3d_push();
		cf_draw3d_translate(cf_v3(CF_SINF(a) * 2.2f, 0.6f + 0.4f * CF_SINF(t + i * 2.0f), CF_COSF(a) * 2.2f));
		cf_draw3d_rotate(cf_quat_from_axis_angle(cf_norm_v3(cf_v3(0.3f, 1, 0.2f)), a * 1.7f));
		cf_draw3d_scale(cf_v3(0.45f));
		cf_draw3d_push_mesh_attributes(tints[i]);
		cf_draw3d_mesh(cube);
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop();
	}

	// The sprite-textured cube: its image lives wherever the atlas compiler put it, and the
	// shader receives the page as u_image plus the sub-rect on in_uv_rect.
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, 1.1f + 0.25f * CF_SINF(t * 0.8f), 0));
	cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0, 1, 0), t * 0.5f));
	cf_draw3d_scale(cf_v3(0.8f));
	cf_draw3d_push_mesh_attributes(cf_v4(1, 1, 1, 1));
	if (lit_pass) {
		cf_draw3d_set_uniform_float("u_textured", 1.0f);
		cf_draw3d_push_texture(sprite);
	}
	cf_draw3d_mesh(cube);
	if (lit_pass) {
		cf_draw3d_pop_texture();
		cf_draw3d_set_uniform_float("u_textured", 0.0f);
	}
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_draw3d", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.10f, 0.11f, 0.14f));

	CF_Mesh cube = s_make_cube();
	CF_Shader shadow_shd = cf_make_shader_from_source(s_shadow_vs, s_shadow_fs);
	CF_Shader lit_shd = cf_make_shader_from_source(s_lit_vs, s_lit_fs);

	// The shadow map: a canvas whose depth target is sampleable and carries a comparison
	// sampler -- texture() through sampler2DShadow becomes a hardware PCF depth test.
	CF_CanvasParams shadow_params = cf_canvas_defaults(2048, 2048);
	shadow_params.depth_stencil_enable = true;
	shadow_params.depth_stencil_target.compare_enable = true;
	shadow_params.depth_stencil_target.compare_function = CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL;
	shadow_params.depth_stencil_target.usage |= CF_TEXTURE_USAGE_SAMPLER_BIT;
	CF_Canvas shadow_canvas = cf_make_canvas(shadow_params);

	// A checkerboard sprite, fed to the atlas like any other sprite.
	CF_Pixel checker[64 * 64];
	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			bool on = ((x / 8) + (y / 8)) & 1;
			checker[y * 64 + x] = on ? cf_make_pixel_rgb(240, 240, 240) : cf_make_pixel_rgb(240, 110, 60);
		}
	}
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(checker, 64, 64);

	// A fixed directional light. Static across the run, so its matrices and the lit pass's
	// uniforms/textures are set once here -- recorded submissions capture them, and the
	// live-state versions stick around for the immediate spinners.
	CF_V3 light_pos = cf_v3(9, 13, 7);
	CF_M4x4 light_view = cf_look_at(light_pos, cf_v3(0, 0, 0), cf_v3(0, 1, 0));
	CF_M4x4 light_proj = cf_ortho(-14, 14, -14, 14, 1, 40);
	CF_M4x4 light_vp = cf_mul(light_proj, light_view);
	CF_V3 light_dir = cf_norm_v3(cf_sub_v3(cf_v3(0, 0, 0), light_pos));

	cf_draw3d_set_uniform_m4("u_light_vp", light_vp);
	cf_draw3d_set_uniform_v3("u_light_dir", light_dir);
	cf_draw3d_set_uniform_float("u_textured", 0.0f);
	cf_draw3d_set_texture("u_shadow", cf_canvas_get_depth_stencil_target(shadow_canvas));

	// Record the level once per pass (a recorded submission captures its shader).
	CF_DrawList level_shadow = cf_make_draw_list();
	cf_draw3d_push_shader(shadow_shd);
	cf_draw_list_begin(level_shadow);
	s_record_level(cube);
	cf_draw_list_end();
	cf_draw3d_pop_shader();

	CF_DrawList level_lit = cf_make_draw_list();
	cf_draw3d_push_shader(lit_shd);
	cf_draw_list_begin(level_lit);
	s_record_level(cube);
	cf_draw_list_end();
	cf_draw3d_pop_shader();

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// Pass 1: scene depth from the light.
		cf_draw3d_push_projection(light_proj);
		cf_draw3d_push_view(light_view);
		cf_draw3d_push_shader(shadow_shd);
		cf_draw_list(level_shadow);
		s_submit_spinners(cube, t, &sprite, false);
		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
		cf_render_to(shadow_canvas, true);

		// Pass 2: the lit scene under an orbiting camera, straight onto the screen.
		int w, h;
		cf_app_get_size(&w, &h);
		CF_V3 eye = cf_v3(CF_SINF(t * 0.3f) * 11.0f, 6.5f, CF_COSF(t * 0.3f) * 11.0f);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.0f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 0.5f, 0), cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(lit_shd);
		cf_draw_list(level_lit);
		s_submit_spinners(cube, t, &sprite, true);
		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// The HUD rides the same command stream.
		cf_draw_push_color(cf_color_white());
		cf_draw_text("cute_draw3d -- shadow pass into a comparison sampler, baked draw list,\ninstanced spinners, one cube sprite-textured through the atlas.", cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_draw_list(level_shadow);
	cf_destroy_draw_list(level_lit);
	cf_easy_sprite_unload(&sprite);
	cf_destroy_canvas(shadow_canvas);
	cf_destroy_shader(shadow_shd);
	cf_destroy_shader(lit_shd);
	cf_destroy_mesh(cube);
	cf_destroy_app();
	return 0;
}
