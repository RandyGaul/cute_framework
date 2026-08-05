/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// Omnidirectional (point-light) shadows: one cube texture, six per-face canvases via
// CF_CanvasParams.attach_target, and a light that casts shadows in every direction at once.
//
// Each frame renders the scene six times -- once per cube face, 90 degree frustum each --
// storing the fragment's DISTANCE to the light in a float color face. The lit pass then
// compares each pixel's own distance against the stored one along its direction from the
// light: farther than what the light saw means something else is in between, i.e. shadow.
// Distance-in-color is the portable flavor of this technique (depth-format cube faces also
// attach -- see CF_CanvasParams.attach_target -- but color faces work on every backend).
//
// The scene draws through cute_draw3d.h with two shaders and zero extra machinery: the same
// submission code runs under the face cameras and the main camera.

#include <cute.h>
#include <stdio.h>

#define PILLARS 8
#define LIGHT_RADIUS 14.0f
#define FACE_SIZE 512

static const char* s_dist_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 0) out vec3 v_world;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    v_world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    gl_Position = u_view_projection * vec4(v_world, 1.0);\n"
"}\n";

static const char* s_dist_fs =
"layout (location = 0) in vec3 v_world;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_light;\n" // xyz position, w radius.
"};\n"
"void main() { result = vec4(distance(v_world, u_light.xyz) / u_light.w, 0.0, 0.0, 1.0); }\n";

static const char* s_lit_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 12) in vec4 in_nmat0;\n"
"layout (location = 13) in vec4 in_nmat1;\n"
"layout (location = 14) in vec4 in_nmat2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_world;\n"
"layout (location = 1) out vec3 v_normal;\n"
"layout (location = 2) out vec4 v_color;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    v_world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = vec3(dot(in_nmat0.xyz, in_normal), dot(in_nmat1.xyz, in_normal), dot(in_nmat2.xyz, in_normal));\n"
"    v_color = in_mesh_attributes;\n"
"    gl_Position = u_view_projection * vec4(v_world, 1.0);\n"
"}\n";

static const char* s_lit_fs =
"layout (location = 0) in vec3 v_world;\n"
"layout (location = 1) in vec3 v_normal;\n"
"layout (location = 2) in vec4 v_color;\n"
"layout (location = 0) out vec4 result;\n"
"layout (set = 2, binding = 0) uniform samplerCube u_shadow;\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_light;\n"
"};\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    vec3 to_light = u_light.xyz - v_world;\n"
"    float dist = length(to_light) / u_light.w;\n"
"    vec3 l = normalize(to_light);\n"
"    // The light's stored view along this direction; farther than it saw means shadow.\n"
"    float seen = texture(u_shadow, -l).x;\n"
"    float lit = dist - 0.01 <= seen ? 1.0 : 0.0;\n"
"    float diffuse = max(dot(n, l), 0.0) * lit;\n"
"    float atten = clamp(1.0 - dist, 0.0, 1.0);\n"
"    atten *= atten;\n"
"    vec3 warm = vec3(1.0, 0.85, 0.6);\n"
"    vec3 col = v_color.rgb * (0.06 + diffuse * atten * warm * 1.4);\n"
"    result = vec4(col, 1.0);\n"
"}\n";

// A unit cube with normals (interleaved), the only mesh in the scene.
static CF_Mesh s_make_cube()
{
	typedef struct { CF_V3 p, n; } Vtx;
	Vtx verts[36];
	int k = 0;
	for (int f = 0; f < 6; ++f) {
		int axis = f >> 1;
		float sign = (f & 1) ? -1.0f : 1.0f;
		CF_V3 n = cf_v3(axis == 0 ? sign : 0, axis == 1 ? sign : 0, axis == 2 ? sign : 0);
		CF_V3 u = cf_v3(n.y, n.z, n.x);
		CF_V3 w = cf_cross_v3(n, u);
		CF_V3 q[4];
		q[0] = cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u, -1), cf_mul_v3_f(w, -1)));
		q[1] = cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u,  1), cf_mul_v3_f(w, -1)));
		q[2] = cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u,  1), cf_mul_v3_f(w,  1)));
		q[3] = cf_add_v3(n, cf_add_v3(cf_mul_v3_f(u, -1), cf_mul_v3_f(w,  1)));
		int idx[6] = { 0, 1, 2, 0, 2, 3 };
		for (int i = 0; i < 6; ++i) { verts[k].p = q[idx[i]]; verts[k].n = n; k++; }
	}
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = 0;
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = sizeof(CF_V3);
	CF_Mesh m = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vtx));
	cf_mesh_update_vertex_data(m, verts, 36);
	return m;
}

// One box: position + half-extents + color, through the transform stack.
static void s_box(CF_Mesh cube, CF_V3 at, CF_V3 he, CF_V4 color)
{
	cf_draw3d_push();
	cf_draw3d_translate(at);
	cf_draw3d_scale(he);
	cf_draw3d_push_mesh_attributes(color);
	cf_draw3d_mesh(cube);
	cf_draw3d_pop_mesh_attributes();
	cf_draw3d_pop();
}

// The whole scene, camera-agnostic: floor slab and a ring of pillars.
static void s_scene(CF_Mesh cube)
{
	s_box(cube, cf_v3(0, -0.5f, 0), cf_v3(12, 0.5f, 12), cf_v4(0.55f, 0.56f, 0.60f, 1));
	for (int i = 0; i < PILLARS; ++i) {
		float a = 2.0f * CF_PI * (float)i / PILLARS;
		float h = 1.6f + 1.1f * (float)(i % 3);
		CF_V4 tint = cf_v4(0.7f + 0.25f * (float)(i & 1), 0.62f, 0.85f - 0.3f * (float)(i & 1), 1);
		s_box(cube, cf_v3(cosf(a) * 5.0f, h, sinf(a) * 5.0f), cf_v3(0.55f, h, 0.55f), tint);
	}
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("Point Light Shadows", 0, 0, 0, 960, 540, options, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_app_set_present_mode(CF_PRESENT_MODE_VSYNC);
	cf_clear_color(0.02f, 0.02f, 0.03f, 1.0f);

	CF_Mesh cube = s_make_cube();
	CF_Shader dist_shd = cf_make_shader_from_source(s_dist_vs, s_dist_fs);
	CF_Shader lit_shd = cf_make_shader_from_source(s_lit_vs, s_lit_fs);

	// The shadow cube: float color faces holding distance-to-light, six canvases attached.
	CF_TextureParams tp = cf_texture_defaults(FACE_SIZE, FACE_SIZE);
	tp.texture_type = CF_TEXTURE_TYPE_CUBE;
	tp.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	tp.usage = CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_SAMPLER_BIT;
	CF_Texture shadow_cube = cf_make_texture(tp);
	CF_Canvas faces[6];
	for (int i = 0; i < 6; ++i) {
		CF_CanvasParams params = cf_canvas_defaults(FACE_SIZE, FACE_SIZE);
		params.attach_target = shadow_cube;
		params.attach_layer = i;
		params.depth_stencil_enable = true;
		faces[i] = cf_make_canvas(params);
		cf_canvas_set_clear_color(faces[i], cf_make_color_rgb_f(1, 1, 1)); // "Saw nothing": max distance.
	}

	// Cube face bases: look directions and ups per face (+X -X +Y -Y +Z -Z).
	CF_V3 face_dir[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
	CF_V3 face_up[6]  = { { 0, -1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, -1, 0 }, { 0, -1, 0 } };

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		// The light wanders around the ring's middle; everything below follows it live.
		CF_V3 light = cf_v3(cosf(t * 0.7f) * 2.2f, 1.7f + sinf(t * 1.1f) * 0.8f, sinf(t * 0.7f) * 2.2f);
		float lu[4] = { light.x, light.y, light.z, LIGHT_RADIUS };
		cf_draw3d_set_uniform("u_light", lu, CF_UNIFORM_TYPE_FLOAT4, 1);

		// Six face passes: same scene, 90 degree frustums from the light.
		cf_draw3d_push_shader(dist_shd);
		for (int i = 0; i < 6; ++i) {
			cf_draw3d_push_projection(cf_perspective(CF_PI * 0.5f, 1.0f, 0.1f, LIGHT_RADIUS));
			cf_draw3d_push_view(cf_look_at(light, cf_add_v3(light, face_dir[i]), face_up[i]));
			s_scene(cube);
			cf_draw3d_pop_view();
			cf_draw3d_pop_projection();
			cf_render_to(faces[i], true);
		}
		cf_draw3d_pop_shader();

		// Lit pass under the orbiting main camera.
		int w, h;
		cf_app_get_size(&w, &h);
		float cam_a = t * 0.15f;
		CF_V3 eye = cf_v3(cosf(cam_a) * 14.0f, 9.5f, sinf(cam_a) * 14.0f);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 4.0f, (float)w / (float)h, 0.1f, 100.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 1.2f, 0), cf_v3(0, 1, 0)));
		cf_draw3d_push_shader(lit_shd);
		cf_draw3d_set_texture("u_shadow", shadow_cube);
		s_scene(cube);
		cf_draw3d_pop_shader();

		// The light itself, through the built-in shapes.
		cf_draw3d_push_color(cf_make_color_rgb_f(1.0f, 0.9f, 0.6f));
		cf_draw3d_sphere(light, 0.15f);
		cf_draw3d_pop_color();

		cf_draw3d_pop_view();
		cf_draw3d_pop_projection();
		cf_app_draw_onto_screen(true);

	}

	for (int i = 0; i < 6; ++i) cf_destroy_canvas(faces[i]);
	cf_destroy_texture(shadow_cube);
	cf_destroy_shader(dist_shd);
	cf_destroy_shader(lit_shd);
	cf_destroy_mesh(cube);
	cf_destroy_app();
	return 0;
}
