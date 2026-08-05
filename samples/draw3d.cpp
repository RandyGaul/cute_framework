/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// A city at dusk through cute_draw3d.h: ten thousand buildings in one baked draw list, a
// shadow-mapped sun, distance fog, procedural window lights, and a camera that flies the
// streets -- with a 2d HUD riding the same command stream.
//
// The point of this sample is the draw list. The whole city -- 10,000 buildings plus the
// ground -- records ONCE at init, with no shader pushed: the shader stays a free variable,
// bound per pass at replay (push the shadow shader, replay; push the lit shader, replay),
// and the bake is a single instanced draw either way. Every frame the camera moves and the
// replay simply happens under the new view; no per-building CPU work, ever. Per-building
// variety (size, tint, window pattern) rides the per-instance mesh attributes, which never
// split the draw.
//
// Everything atmospheric is user shader code, not framework policy: the lambert + shadow
// lighting, the exponential fog, and the windows (a world-space grid hashed per building)
// live in the fragment shader below. Swap in your own and CF neither knows nor cares.
//
// The shadow map itself is the access-layer machinery: a canvas whose depth target carries a
// comparison sampler, sampled through sampler2DShadow for hardware PCF. See the pixel_3d
// sample for the other way (color-encoded depth with a hand-rolled kernel).

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Cute;

#define CITY_N 100          // Buildings per side; CITY_N^2 total.
#define CITY_SPACING 3.4f
#define CITY_EXTENT (CITY_N * CITY_SPACING * 0.5f)
#define SHADOW_SIZE 2048

//--------------------------------------------------------------------------------------------------
// Shaders.

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

static const char* s_lit_vs = R"(
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (location = 12) in vec4 in_nmat0;
layout (location = 13) in vec4 in_nmat1;
layout (location = 14) in vec4 in_nmat2;
layout (location = 15) in vec4 in_mesh_attributes;
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec3 v_world;
layout (location = 2) out vec4 v_attrs; // rgb tint, w per-building seed (0 = no windows).
layout (location = 3) out float v_depth;
layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
};
void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	v_normal = normalize(vec3(dot(in_nmat0.xyz, in_normal),
	                          dot(in_nmat1.xyz, in_normal),
	                          dot(in_nmat2.xyz, in_normal)));
	v_world = world;
	v_attrs = in_mesh_attributes;
	gl_Position = u_view_projection * vec4(world, 1.0);
	// A per-frame camera uniform set outside the recording would stay live (ambient
	// uniforms bind at cf_draw_list time), but none is needed: the camera stacks already
	// deliver everything camera-dependent, and view depth for fog is just w.
	v_depth = gl_Position.w;
}
)";

// Dusk lighting: a low warm sun with hardware-PCF shadows, cool sky ambient, exponential
// distance fog, and procedural window lights. Buildings are axis-aligned, so the window grid
// lives in world space: facades pick their 2d coordinates by normal, and each window cell
// hashes against the building's seed to decide whether anyone is home.
static const char* s_lit_fs = R"(
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec3 v_world;
layout (location = 2) in vec4 v_attrs;
layout (location = 3) in float v_depth;
layout (location = 0) out vec4 result;
layout (set = 2, binding = 0) uniform sampler2DShadow u_shadow;
layout (set = 3, binding = 0) uniform uniform_block {
	mat4 u_light_vp;
	vec4 u_light_dir; // xyz: direction the sun travels.
	vec4 u_fog_color;
};

float hash2(vec2 cell, float seed)
{
	return fract(sin(dot(vec3(cell, seed), vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

void main()
{
	vec3 n = normalize(v_normal);

	// Sun + shadow map (canvas depth target with a comparison sampler).
	float ndl = max(dot(n, -u_light_dir.xyz), 0.0);
	vec4 lp = u_light_vp * vec4(v_world, 1.0);
	vec3 ndc = lp.xyz / lp.w;
	// Canvas textures sample with v = 0 at clip-space +y on every backend, hence the y flip.
	vec2 suv = vec2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	float lit = 1.0;
	if (suv.x >= 0.0 && suv.x <= 1.0 && suv.y >= 0.0 && suv.y <= 1.0) {
		lit = texture(u_shadow, vec3(suv, ndc.z - 0.0018));
	}

	vec3 sun = vec3(1.0, 0.55, 0.32) * (1.05 * ndl * lit);
	vec3 ambient = vec3(0.16, 0.19, 0.28);
	vec3 color = v_attrs.rgb * (ambient + sun);

	// Window lights, on the facades of buildings only (seed 0 marks the ground).
	if (v_attrs.w > 0.0 && abs(n.y) < 0.5) {
		vec2 facade = abs(n.x) > 0.5 ? v_world.zy : v_world.xy;
		vec2 grid = facade * vec2(1.4, 0.9);
		vec2 cell = floor(grid);
		vec2 f = fract(grid);
		float window = step(0.3, f.x) * step(f.x, 0.8) * step(0.25, f.y) * step(f.y, 0.7);
		float home = step(0.62, hash2(cell, v_attrs.w));
		float warmth = 0.6 + 0.4 * hash2(cell + 31.0, v_attrs.w);
		color += vec3(1.0, 0.75, 0.45) * (window * home * warmth * 1.1);
	}

	// Exponential fog toward the dusk sky, which is what sells the city's scale.
	float fog = 1.0 - exp(-v_depth * 0.0062);
	color = mix(color, u_fog_color.rgb, fog);

	result = vec4(color, 1.0);
}
)";

//--------------------------------------------------------------------------------------------------
// A unit cube (half-extent 1), 36 vertices, CCW-wound for the default 3d render state.

struct Vertex
{
	CF_V3 pos;
	CF_V3 n;
};

static CF_Mesh s_make_cube()
{
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
		for (int i = 0; i < 6; ++i) {
			verts[f * 6 + i].pos = faces[f].c[idx[i]];
			verts[f * 6 + i].n = faces[f].n;
		}
	}
	CF_VertexAttribute attrs[2] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(Vertex, n);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 36);
	return mesh;
}

//--------------------------------------------------------------------------------------------------

// Records the whole city, shaderless: with nothing pushed the shader records as a free
// variable, and each pass binds its own at replay. Every submission here is the same cube
// under the same state -- the bake folds all of it into ONE instanced draw, and the
// per-building variety rides the instance lanes.
static void s_record_city(CF_Mesh cube)
{
	// The ground. Seed 0 tells the shader to skip window lights.
	cf_draw3d_push();
	cf_draw3d_translate(cf_v3(0, -1.0f, 0));
	cf_draw3d_scale(cf_v3(CITY_EXTENT + 30.0f, 1.0f, CITY_EXTENT + 30.0f));
	cf_draw3d_push_mesh_attributes(cf_v4(0.16f, 0.16f, 0.18f, 0));
	cf_draw3d_mesh(cube);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();

	// The buildings: a downtown of towers falling off into low sprawl, with plazas skipped.
	CF_Rnd rnd = cf_rnd_seed(7);
	for (int i = 0; i < CITY_N; ++i) {
		for (int j = 0; j < CITY_N; ++j) {
			if (cf_rnd_range_float(&rnd, 0, 1) < 0.08f) continue; // Plaza.
			// Footprint plus jitter always fits the cell: max half-extent 1.25 + 0.35 of
			// jitter leaves a gap between neighbors, so facades never interpenetrate and
			// z-fight (two coincident walls both drawing window lights shimmer badly).
			float x = (i - CITY_N * 0.5f) * CITY_SPACING + cf_rnd_range_float(&rnd, -0.35f, 0.35f);
			float z = (j - CITY_N * 0.5f) * CITY_SPACING + cf_rnd_range_float(&rnd, -0.35f, 0.35f);
			float downtown = cf_max(0.0f, 1.0f - CF_SQRTF(x * x + z * z) / CITY_EXTENT);
			float h = cf_rnd_range_float(&rnd, 1.5f, 4.0f) + downtown * downtown * cf_rnd_range_float(&rnd, 0, 34.0f);
			float w = cf_rnd_range_float(&rnd, 0.7f, 1.25f);
			float d = cf_rnd_range_float(&rnd, 0.7f, 1.25f);
			float shade = cf_rnd_range_float(&rnd, 0.35f, 0.7f);
			CF_V4 tint = cf_v4(
				shade * cf_rnd_range_float(&rnd, 0.85f, 1.0f),
				shade * cf_rnd_range_float(&rnd, 0.85f, 1.0f),
				shade * cf_rnd_range_float(&rnd, 0.95f, 1.15f),
				cf_rnd_range_float(&rnd, 0.05f, 1.0f)); // w: window seed.
			cf_draw3d_push();
			cf_draw3d_translate(cf_v3(x, h, z));
			cf_draw3d_scale(cf_v3(w, h, d));
			cf_draw3d_push_mesh_attributes(tint);
			cf_draw3d_mesh(cube);
			cf_draw3d_pop_mesh_attributes();
			cf_draw3d_pop();
		}
	}
}

// A lazy figure-eight over the city. The height rides the local skyline -- the same downtown
// falloff the building generator uses -- so the camera swoops high over the towers and dives
// low across the sprawl without ever clipping a rooftop. Look-at chases a point further along
// the same path, so the camera banks into its turns.
static CF_V3 s_fly_path(float t)
{
	float a = t * 0.11f;
	float r = CITY_EXTENT * (0.45f + 0.3f * CF_SINF(t * 0.031f));
	float x = CF_SINF(a) * r;
	float z = CF_SINF(a * 2.0f) * r * 0.55f;
	float downtown = cf_max(0.0f, 1.0f - CF_SQRTF(x * x + z * z) / CITY_EXTENT);
	float skyline = 2.0f * (4.0f + downtown * downtown * 34.0f); // Tallest possible roof here.
	float y = skyline + 5.0f + 4.0f * CF_SINF(t * 0.13f);
	return cf_v3(x, y, z);
}

// Screenshot/exit harness, same contract as the fireflies and model3d samples:
// --shot <t> saves a numbered png of the app canvas, --exit-at <t> quits cleanly.
static int s_shot_count;
static void s_screenshot()
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
		snprintf(path, sizeof(path), "/draw3d_shot_%02d.png", s_shot_count++);
		cf_image_save_png(path, &img);
		printf("saved %s\n", path);
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
	CF_Result result = cf_make_app("cute_draw3d -- city", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_fs_set_write_directory(cf_fs_get_base_directory());

	CF_Color fog_color = cf_make_color_rgb_f(0.15f, 0.13f, 0.19f); // Dusk haze.
	cf_canvas_set_clear_color(cf_app_get_canvas(), fog_color);

	CF_Mesh cube = s_make_cube();
	CF_Shader shadow_shd = cf_make_shader_from_source(s_shadow_vs, s_shadow_fs);
	CF_Shader lit_shd = cf_make_shader_from_source(s_lit_vs, s_lit_fs);

	// The sun's shadow map: a canvas depth target with a comparison sampler (hardware PCF).
	CF_CanvasParams shadow_params = cf_canvas_defaults(SHADOW_SIZE, SHADOW_SIZE);
	shadow_params.depth_stencil_enable = true;
	shadow_params.depth_stencil_target.compare_enable = true;
	shadow_params.depth_stencil_target.compare_function = CF_COMPARE_FUNCTION_LESS_THAN_OR_EQUAL;
	shadow_params.depth_stencil_target.usage |= CF_TEXTURE_USAGE_SAMPLER_BIT;
	CF_Canvas shadow_canvas = cf_make_canvas(shadow_params);

	// A fixed low dusk sun. Static lights mean static uniforms: set once here, captured by
	// the recordings below, and reused by every frame's replay.
	CF_V3 sun_pos = cf_v3(CITY_EXTENT * 1.2f, CITY_EXTENT * 0.55f, CITY_EXTENT * 0.8f);
	CF_M4x4 light_view = cf_look_at(sun_pos, cf_v3(0, 0, 0), cf_v3(0, 1, 0));
	float ortho_r = CITY_EXTENT * 1.45f;
	CF_M4x4 light_proj = cf_ortho(-ortho_r, ortho_r, -ortho_r, ortho_r, 1.0f, CITY_EXTENT * 5.0f);
	CF_M4x4 light_vp = cf_mul(light_proj, light_view);
	CF_V3 light_dir = cf_norm_v3(cf_sub_v3(cf_v3(0, 0, 0), sun_pos));
	CF_V4 light_dir4 = cf_v4_from_v3(light_dir, 0);
	CF_V4 fog4 = cf_v4(fog_color.r, fog_color.g, fog_color.b, 1);

	cf_draw3d_set_uniform_m4("u_light_vp", light_vp);
	cf_draw3d_set_uniform("u_light_dir", &light_dir4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw3d_set_uniform("u_fog_color", &fog4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw3d_set_texture("u_shadow", cf_canvas_get_depth_stencil_target(shadow_canvas));

	// Record the city ONCE, with no shader pushed: the shader slot stays a free variable
	// (like the camera), and each pass below binds its own at replay -- one recording,
	// ~10,000 submissions, ONE instanced draw per pass. A shader pushed inside the
	// recording would freeze instead; ambient state binds at replay, exactly like the
	// transform stack.
	CF_DrawList city = cf_make_draw_list();
	cf_draw_list_begin(city);
	s_record_city(cube);
	cf_draw_list_end();

	// The sun never moves, so its shadow map could even render once and be kept -- but
	// re-rendering per frame keeps the sample honest about the cost of a dynamic light,
	// and it is still just one instanced draw.
	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// Pass 1: the city's depth from the sun -- the same recording under the shadow shader.
		cf_draw3d_push_projection(light_proj);
		cf_draw3d_push_view(light_view);
		cf_draw3d_push_shader(shadow_shd);
		cf_draw_list(city);
		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
		cf_render_to(shadow_canvas, true);

		// Pass 2: the lit city under the flying camera, straight onto the screen.
		int w, h;
		cf_app_get_size(&w, &h);
		CF_V3 eye = s_fly_path(t);
		CF_V3 ahead = s_fly_path(t + 6.0f);
		CF_V3 look = cf_v3(ahead.x, ahead.y - 6.0f, ahead.z);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.2f, (float)w / (float)h, 1.0f, 900.0f));
		cf_draw3d_push_view(cf_look_at(eye, look, cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(lit_shd);
		cf_draw_list(city);
		cf_draw3d_pop_shader();
		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		// The HUD rides the same command stream.
		char hud[256];
		snprintf(hud, sizeof(hud),
			"%d buildings + ground, baked into one draw list --\n"
			"one instanced draw for the shadow pass, one for the lit pass.\n"
			"%.0f fps", CITY_N * CITY_N, cf_app_get_smoothed_framerate());
		cf_draw_push_color(cf_make_color_rgb_f(0.85f, 0.87f, 0.95f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);

		for (int i = 0; i < shot_n; ++i) {
			if (shot_times[i] >= 0 && t >= shot_times[i]) {
				s_screenshot();
				shot_times[i] = -1.0f;
			}
		}
		if (exit_at > 0 && t >= exit_at) break;
	}

	cf_destroy_draw_list(city);
	cf_destroy_canvas(shadow_canvas);
	cf_destroy_shader(shadow_shd);
	cf_destroy_shader(lit_shd);
	cf_destroy_mesh(cube);
	cf_destroy_app();
	return 0;
}
