/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Cel shading with inverted hull outlines through cute_draw3d.h: one draw list of toon-lit
// props, replayed twice per frame from the same camera.
//
//     Pass 1: the recording under the cel shader and the default render state.
//     Pass 2: the SAME recording under the hull shader and a front-face-culling render state.
//
// The hull shader inflates every vertex along its normal and paints it ink. Culling the FRONT
// faces of that enlarged copy leaves only the inside of its far half, which sits behind the
// original everywhere except past the silhouette -- so the depth test hides it across the
// body and a band of ink survives around the rim. No edge detection, no post-process.
//
// The point of this sample is that neither the shader nor the render state was pushed inside
// the recording, so both stay free variables of the draw list: each pass binds its own at
// `cf_draw_list` time, and the bake (one instanced draw per mesh) is shared.
//
// The opposite case lives in the same list: a few props are glass. Their translucent render
// state is pushed INSIDE the recording, so it is part of the recording -- frozen. Blending
// belongs to the glass, not to whichever pass happens to be replaying it, and the rest of
// the list stays ambient around it. The flip side is that a frozen draw cannot follow the
// hull pass's cull flip either, so the glass opts out of outlines through its mesh attributes.
//
// The props are the built-in solids (`cf_draw3d_sphere` and friends). A plain shader on the
// stack fully replaces their built-in pipeline under the ordinary mesh contract, with two
// things to know: the solids' color (`cf_draw3d_push_color`) arrives in `in_mesh_attributes`
// and the pushed mesh attributes in `in_uv_rect`; and the shader must already be pushed when
// recording begins -- with nothing pushed a solid resolves to its built-in shader and records
// that frozen. Pushed OUTSIDE begin/end it is merely ambient, so replays may swap it.
//
// Hard edges need one extra ingredient. A cube's per-face normals would tear the hull apart at
// the corners (each face inflates on its own), so cubes inflate along the direction from their
// center instead, flagged per prop through the mesh attributes. That works here because the
// unit cube is convex and centered on its origin; authored meshes usually carry a second,
// smoothed normal attribute for the hull to use.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Cute;

#define PROP_COUNT 72
#define FOV (CF_PI / 3.4f)

//--------------------------------------------------------------------------------------------------
// Shaders.

static const char* s_cel_vs = R"(
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (location = 11) in vec4 in_uv_rect;         // Built-in solids: the pushed mesh attributes.
layout (location = 12) in vec4 in_nmat0;
layout (location = 13) in vec4 in_nmat1;
layout (location = 14) in vec4 in_nmat2;
layout (location = 15) in vec4 in_mesh_attributes; // Built-in solids: cf_draw3d_push_color.
layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec3 v_world;
layout (location = 2) out vec4 v_attrs; // rgb tint, w specular strength.
layout (location = 3) out float v_alpha;
layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
};
void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	v_normal = vec3(dot(in_nmat0.xyz, in_normal), dot(in_nmat1.xyz, in_normal), dot(in_nmat2.xyz, in_normal));
	v_world = world;
	v_attrs = vec4(in_mesh_attributes.rgb, in_uv_rect.x);
	v_alpha = in_mesh_attributes.a; // Only matters under a blending render state (the glass).
	gl_Position = u_view_projection * vec4(world, 1.0);
}
)";

// Three flat bands of diffuse plus a hard specular dot -- lighting quantized into ink-and-paint.
static const char* s_cel_fs = R"(
layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec3 v_world;
layout (location = 2) in vec4 v_attrs;
layout (location = 3) in float v_alpha;
layout (location = 0) out vec4 result;
layout (set = 3, binding = 0) uniform uniform_block {
	vec4 u_light_dir; // xyz: direction the light travels.
	vec4 u_eye;       // xyz: camera position.
};
void main()
{
	vec3 n = normalize(v_normal);
	vec3 l = -u_light_dir.xyz;
	vec3 v = normalize(u_eye.xyz - v_world);
	float ndl = dot(n, l);
	float band = mix(0.34, 0.64, smoothstep(0.0, 0.02, ndl));
	band = mix(band, 1.0, smoothstep(0.5, 0.52, ndl));
	float spec = smoothstep(0.955, 0.965, dot(n, normalize(l + v))) * v_attrs.w;
	vec3 shadow_tint = vec3(0.82, 0.86, 1.0); // Cool the shaded bands a touch.
	vec3 color = v_attrs.rgb * band * mix(shadow_tint, vec3(1.0), band) + vec3(spec * 0.6);
	result = vec4(color, max(v_alpha, spec));
}
)";

// The hull: the same geometry pushed outward -- along its normal, or from its center for the
// hard-edged props (in_uv_rect.y). Props flagged in_uv_rect.z opt out: their vertices collapse
// outside the clip volume and nothing rasterizes. Scaling the push by clip-space w keeps the outline a
// constant width on screen (u_outline.x is the world-space size of the desired pixel width at
// unit view depth).
static const char* s_hull_vs = R"(
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (location = 11) in vec4 in_uv_rect;
layout (location = 12) in vec4 in_nmat0;
layout (location = 13) in vec4 in_nmat1;
layout (location = 14) in vec4 in_nmat2;
layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
	vec4 u_outline;
};
void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	vec3 dir = in_uv_rect.y > 0.5 ? normalize(in_pos) : in_normal;
	vec3 n = normalize(vec3(dot(in_nmat0.xyz, dir), dot(in_nmat1.xyz, dir), dot(in_nmat2.xyz, dir)));
	float w = (u_view_projection * vec4(world, 1.0)).w;
	world += n * (u_outline.x * w);
	gl_Position = u_view_projection * vec4(world, 1.0);
	if (in_uv_rect.z > 0.5) gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
}
)";

static const char* s_hull_fs = R"(
layout (location = 0) out vec4 result;
void main() { result = vec4(0.07, 0.05, 0.10, 1.0); }
)";

//--------------------------------------------------------------------------------------------------

// Records the props under whatever shader the caller has pushed outside the recording. Most
// props push no render state either, so both stay free variables of the list. Every ninth prop
// is glass and pushes its own render state, which records frozen.
static void s_record_props()
{
	// Glass: alpha blended, depth tested but not written. Non-writing mesh commands also sort
	// back-to-front after the opaque ones at flush, so the glass composites over everything.
	CF_RenderState glass_rs = cf_render_state_3d_defaults();
	glass_rs.depth_write_enabled = false;
	glass_rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_SRC_ALPHA;
	glass_rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	glass_rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	glass_rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	CF_Rnd rnd = cf_rnd_seed(11);
	const CF_Color palette[] = {
		cf_make_color_rgb_f(0.96f, 0.42f, 0.36f), cf_make_color_rgb_f(0.99f, 0.76f, 0.30f), cf_make_color_rgb_f(0.45f, 0.78f, 0.52f),
		cf_make_color_rgb_f(0.36f, 0.66f, 0.93f), cf_make_color_rgb_f(0.72f, 0.52f, 0.92f), cf_make_color_rgb_f(0.97f, 0.62f, 0.78f),
	};
	for (int i = 0; i < PROP_COUNT; ++i) {
		// Rings of props around the origin; a golden-angle spiral keeps them from lining up.
		float angle = i * 2.39996f;
		float radius = 2.2f + 1.05f * CF_SQRTF((float)i) * 1.5f;
		float scale = cf_rnd_range_float(&rnd, 0.55f, 1.05f);
		CF_V3 axis = cf_norm_v3(cf_v3(cf_rnd_range_float(&rnd, -1, 1), cf_rnd_range_float(&rnd, 0.2f, 1), cf_rnd_range_float(&rnd, -1, 1)));
		int kind = i % 4;
		bool hard_edged = kind == 3;
		bool glass = i % 9 == 4;
		cf_draw3d_push();
		cf_draw3d_translate(cf_v3(CF_COSF(angle) * radius, scale * 1.35f + cf_rnd_range_float(&rnd, 0, 0.8f), CF_SINF(angle) * radius));
		cf_draw3d_rotate(cf_quat_from_axis_angle(axis, cf_rnd_range_float(&rnd, 0, 2.0f * CF_PI)));
		CF_Color color = palette[cf_rnd_range_int(&rnd, 0, (int)(sizeof(palette) / sizeof(palette[0])) - 1)];
		if (glass) color.a = 0.4f;
		cf_draw3d_push_color(color);
		// x: specular strength -- a flat face catches the highlight all at once and just
		// flashes white, so only the curved props get one. y: inflate the hull from the center.
		// z: no outline.
		cf_draw3d_push_mesh_attributes(cf_v4(hard_edged ? 0.0f : 1.0f, hard_edged ? 1.0f : 0.0f, glass ? 1.0f : 0.0f, 0));
		// Pushed inside the recording, so frozen: the glass blends in every replay no matter
		// which render state the pass pushes. Everything else records none and stays ambient.
		if (glass) cf_draw3d_push_render_state(glass_rs);
		switch (kind) {
		case 0: cf_draw3d_sphere(cf_v3(0, 0, 0), scale); break;
		case 1: cf_draw3d_torus(cf_v3(0, 0, 0), cf_v3(0, 1, 0), scale * 0.8f, scale * 0.32f); break;
		case 2: cf_draw3d_capsule(cf_v3(0, -scale * 0.6f, 0), cf_v3(0, scale * 0.6f, 0), scale * 0.5f); break;
		case 3: cf_draw3d_cube(cf_v3(0, 0, 0), cf_v3(scale * 0.8f, scale * 0.8f, scale * 0.8f)); break;
		}
		if (glass) cf_draw3d_pop_render_state();
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop_color();
		cf_draw3d_pop();
	}
}

// Screenshot/exit harness, same contract as the draw3d and model3d samples:
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
		snprintf(path, sizeof(path), "/cel_shading_shot_%02d.png", s_shot_count++);
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
	bool gles = false;
	for (int i = 1; i < argc; ++i) {
		if (!CF_STRCMP(argv[i], "--shot") && i + 1 < argc) { if (shot_n < 16) shot_times[shot_n++] = (float)atof(argv[++i]); }
		else if (!CF_STRCMP(argv[i], "--exit-at") && i + 1 < argc) exit_at = (float)atof(argv[++i]);
		else if (!CF_STRCMP(argv[i], "--gles")) gles = true;
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	if (gles) options |= CF_APP_OPTIONS_GFX_OPENGL_BIT;
	CF_Result result = cf_make_app("cute_draw3d -- cel shading", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_fs_set_write_directory(cf_fs_get_base_directory());
	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.95f, 0.91f, 0.84f));

	CF_Shader cel_shd = cf_make_shader_from_source(s_cel_vs, s_cel_fs);
	CF_Shader hull_shd = cf_make_shader_from_source(s_hull_vs, s_hull_fs);

	// The hull pass's render state: the 3d defaults with the cull mode flipped.
	CF_RenderState hull_rs = cf_render_state_3d_defaults();
	hull_rs.cull_mode = CF_CULL_MODE_FRONT;

	// Ambient uniforms are captured by NAME at record time, so every name the replays will
	// drive per frame must exist before recording. The values set here are placeholders.
	CF_V4 light_dir = cf_v4_from_v3(cf_norm_v3(cf_v3(-0.55f, -0.75f, -0.35f)), 0);
	CF_V4 zero = cf_v4(0, 0, 0, 0);
	cf_draw3d_set_uniform("u_light_dir", &light_dir, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw3d_set_uniform("u_eye", &zero, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_draw3d_set_uniform("u_outline", &zero, CF_UNIFORM_TYPE_FLOAT4, 1);

	// One recording serves both passes. The cel shader is pushed OUTSIDE begin/end: that keeps
	// the built-in solids on the plain mesh contract while leaving the shader ambient, so the
	// hull pass can swap it.
	cf_draw3d_push_shader(cel_shd);
	CF_DrawList props = cf_make_draw_list();
	cf_draw_list_begin(props);
	s_record_props();
	cf_draw_list_end();
	cf_draw3d_pop_shader();

	bool outlines = true;
	float outline_px = 3.0f;
	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		if (cf_key_just_pressed(CF_KEY_O)) outlines = !outlines;
		if (cf_key_just_pressed(CF_KEY_UP)) outline_px = cf_min(outline_px + 1.0f, 12.0f);
		if (cf_key_just_pressed(CF_KEY_DOWN)) outline_px = cf_max(outline_px - 1.0f, 1.0f);

		int w, h;
		cf_app_get_size(&w, &h);
		float orbit = t * 0.22f;
		CF_V3 eye_pos = cf_v3(CF_COSF(orbit) * 19.0f, 8.5f + 2.0f * CF_SINF(t * 0.31f), CF_SINF(orbit) * 19.0f);
		CF_V4 eye = cf_v4_from_v3(eye_pos, 1);
		// World-space size of `outline_px` pixels at unit view depth; the hull shader scales
		// it by each vertex's depth to hold the width constant on screen.
		CF_V4 outline = cf_v4(outline_px * 2.0f * CF_TANF(FOV * 0.5f) / (float)h, 0, 0, 0);
		cf_draw3d_set_uniform("u_eye", &eye, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_draw3d_set_uniform("u_outline", &outline, CF_UNIFORM_TYPE_FLOAT4, 1);

		cf_draw3d_push_projection(cf_perspective(FOV, (float)w / (float)h, 0.5f, 200.0f));
		cf_draw3d_push_view(cf_look_at(eye_pos, cf_v3(0, 1.5f, 0), cf_v3(0, 1, 0)));

		// Pass 1: toon-lit bodies. The floor is plain immediate-mode drawing alongside the
		// replay, and gets no outline because the hull pass never draws it.
		cf_draw3d_push_shader(cel_shd);
		cf_draw3d_push_color(cf_make_color_rgb_f(0.80f, 0.74f, 0.66f));
		cf_draw3d_push_mesh_attributes(cf_v4(0, 0, 0, 0));
		cf_draw3d_cube(cf_v3(0, -0.5f, 0), cf_v3(40.0f, 0.5f, 40.0f));
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop_color();
		cf_draw_list(props);
		cf_draw3d_pop_shader();

		// Pass 2: the same recording as an inverted hull. The opaque props recorded no render
		// state of their own, so they bind the one pushed here -- front faces culled. The glass
		// keeps its frozen state and sits this pass out (see the hull vertex shader).
		if (outlines) {
			cf_draw3d_push_shader(hull_shd);
			cf_draw3d_push_render_state(hull_rs);
			cf_draw_list(props);
			cf_draw3d_pop_render_state();
			cf_draw3d_pop_shader();
		}

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();

		char hud[512];
		snprintf(hud, sizeof(hud),
			"%d props, one draw list, replayed for the cel pass and the hull pass --\n"
			"each replay binds the shader and render state pushed around it.\n"
			"O outlines %s   UP/DOWN width %.0fpx   %.0f fps",
			PROP_COUNT, outlines ? "on" : "off", outline_px, cf_app_get_framerate());
		cf_draw_push_color(cf_make_color_rgb_f(0.15f, 0.12f, 0.2f));
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

	cf_destroy_draw_list(props);
	cf_destroy_shader(cel_shd);
	cf_destroy_shader(hull_shd);
	cf_destroy_app();
	return 0;
}
