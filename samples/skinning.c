/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

// GPU skinning through cute_draw3d.h: a swaying kelp bed, sixty strands in one instanced draw.
//
// CF has no animation system, and this sample is the argument that it does not need one for
// skinning: the whole technique is (a) two vertex attributes -- joint indices and weights --
// declared like any other, and (b) a MAT4 array uniform holding this frame's bone matrices,
// set through cf_draw3d_set_uniform. The blend happens in the vertex shader below, upstream
// of the ordinary draw3d shader contract; everything downstream (instancing, transforms,
// lighting) neither knows nor cares that the mesh is animated.
//
// All sixty strands share one skeleton, so they coalesce into a single instanced draw -- each
// strand's rotation turns the shared sway into its own direction, which is variety enough for
// a field of plants. Strands needing truly unique skeletons would each set their own bone
// array, splitting into one draw per skeleton: that is the trade, and it is yours per object.
//
// The skeleton is a plain chain. Bone matrices arrive fully composed (posed * inverse-bind),
// so the shader's job is only the weighted blend -- the same contract every skinned-model
// format (glTF and friends) expects, just built procedurally here instead of loaded.

#include <cute.h>
#include <stdio.h>

#define BONES 6
#define KELP_HEIGHT 2.6f
#define BONE_SPACING (KELP_HEIGHT / (float)(BONES - 1))
#define RINGS 12
#define SIDES 6
#define STRANDS 60

//--------------------------------------------------------------------------------------------------
// Shaders.

// The skinning happens here, before the draw3d contract's model transform. in_joints holds
// two bone indices (a chain vertex never needs more; formats with four-bone rigs add two more
// lanes the same way) and in_weights their blend.
static const char* s_kelp_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_joints;\n"
"layout (location = 3) in vec2 in_weights;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec4 v_attrs;\n"
"layout (location = 2) out float v_height;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"    mat4 u_bones[6];\n"
"};\n"
"void main() {\n"
"    // Blend transformed positions rather than matrices -- two mat*vec products beat\n"
"    // building a blended mat4, and it is the same skinning math.\n"
"    vec4 p0 = vec4(in_pos, 1.0);\n"
"    vec4 n0 = vec4(in_normal, 0.0);\n"
"    vec3 pos = (u_bones[int(in_joints.x)] * p0 * in_weights.x + u_bones[int(in_joints.y)] * p0 * in_weights.y).xyz;\n"
"    vec3 nrm = normalize((u_bones[int(in_joints.x)] * n0 * in_weights.x + u_bones[int(in_joints.y)] * n0 * in_weights.y).xyz);\n"
"    vec4 p = vec4(pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_normal = normalize(vec3(dot(in_model0.xyz, nrm), dot(in_model1.xyz, nrm), dot(in_model2.xyz, nrm)));\n"
"    v_attrs = in_mesh_attributes;\n"
"    v_height = in_pos.y / 2.6;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

// Simple two-tone underwater shading: a directional key filtered blue-green, plus a fake
// caustic shimmer near the tips. Lighting is the sample's, not the framework's.
static const char* s_kelp_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec4 v_attrs;\n"
"layout (location = 2) in float v_height;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    vec3 n = normalize(v_normal);\n"
"    float ndl = max(dot(n, normalize(vec3(0.3, 0.8, 0.45))), 0.0);\n"
"    vec3 deep = vec3(0.03, 0.10, 0.09);\n"
"    vec3 color = v_attrs.rgb * (0.35 + 0.65 * ndl);\n"
"    color = mix(deep, color, 0.35 + 0.65 * v_height);\n"
"    result = vec4(color, 1.0);\n"
"}\n";

// The sea floor, an unskinned mesh under a plain contract shader -- unrelated meshes are
// separate draws, skinned or not.
static const char* s_floor_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 8)  in vec4 in_model0;\n"
"layout (location = 9)  in vec4 in_model1;\n"
"layout (location = 10) in vec4 in_model2;\n"
"layout (location = 15) in vec4 in_mesh_attributes;\n"
"layout (location = 0) out vec4 v_color;\n"
"layout (location = 1) out vec3 v_world;\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_view_projection;\n"
"};\n"
"void main() {\n"
"    vec4 p = vec4(in_pos, 1.0);\n"
"    vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));\n"
"    v_color = in_mesh_attributes;\n"
"    v_world = world;\n"
"    gl_Position = u_view_projection * vec4(world, 1.0);\n"
"}\n";

static const char* s_floor_fs =
"layout (location = 0) in vec4 v_color;\n"
"layout (location = 1) in vec3 v_world;\n"
"layout (location = 0) out vec4 result;\n"
"void main() {\n"
"    float fade = exp(-length(v_world.xz) * 0.14);\n"
"    result = vec4(v_color.rgb * (0.4 + 0.6 * fade), 1.0);\n"
"}\n";

//--------------------------------------------------------------------------------------------------
// The kelp strand: a tapered tube along +y with per-vertex joints and weights. Each vertex
// blends the two bones bracketing its height -- the standard chain rig.

typedef struct KelpVertex
{
	CF_V3 pos;
	CF_V3 normal;
	CF_V2 joints;  // Two bone indices, as floats.
	CF_V2 weights;
} KelpVertex;

static CF_Mesh s_make_kelp(void)
{
	// RINGS rings of SIDES vertices, stitched into quads. The radius tapers toward the tip.
	KelpVertex verts[RINGS * SIDES];
	for (int r = 0; r < RINGS; ++r) {
		float f = (float)r / (float)(RINGS - 1);
		float y = f * KELP_HEIGHT;
		float radius = 0.10f * (1.0f - 0.85f * f);
		float along = y / BONE_SPACING;
		float j0 = CF_FLOORF(along);
		if (j0 > BONES - 2) j0 = BONES - 2;
		float w1 = along - j0;
		for (int s = 0; s < SIDES; ++s) {
			float a = (float)s / (float)SIDES * 2.0f * CF_PI;
			KelpVertex* v = &verts[r * SIDES + s];
			v->pos = cf_v3(CF_COSF(a) * radius, y, CF_SINF(a) * radius);
			v->normal = cf_v3(CF_COSF(a), 0, CF_SINF(a));
			v->joints = cf_v2(j0, j0 + 1);
			v->weights = cf_v2(1.0f - w1, w1);
		}
	}
	uint32_t indices[(RINGS - 1) * SIDES * 6];
	int ic = 0;
	for (int r = 0; r < RINGS - 1; ++r) {
		for (int s = 0; s < SIDES; ++s) {
			uint32_t a = r * SIDES + s;
			uint32_t b = r * SIDES + (s + 1) % SIDES;
			uint32_t c = a + SIDES;
			uint32_t d = b + SIDES;
			indices[ic++] = a; indices[ic++] = c; indices[ic++] = b;
			indices[ic++] = b; indices[ic++] = c; indices[ic++] = d;
		}
	}
	CF_VertexAttribute attrs[4] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(KelpVertex, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(KelpVertex, normal);
	attrs[2].name = "in_joints";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(KelpVertex, joints);
	attrs[3].name = "in_weights";
	attrs[3].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[3].offset = CF_OFFSET_OF(KelpVertex, weights);
	CF_Mesh mesh = cf_make_mesh((int)sizeof(verts), attrs, 4, (int)sizeof(KelpVertex));
	cf_mesh_update_vertex_data(mesh, verts, RINGS * SIDES);
	cf_mesh_set_index_buffer(mesh, (int)sizeof(indices), 32);
	cf_mesh_update_index_data(mesh, indices, ic);
	return mesh;
}

static CF_Mesh s_make_floor(void)
{
	// A single quad, wound CCW from above.
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

// Poses the chain for time t and writes the final skinning matrices (posed * inverse-bind).
// Each bone adds a little more sway than its parent, like current dragging on a stem; the
// inverse bind for a chain along +y is just a translation back down.
static void s_pose_bones(CF_M4x4* bones, float t)
{
	CF_M4x4 world = cf_m4_identity();
	for (int i = 0; i < BONES; ++i) {
		float sway = 0.16f * (float)i;
		float ax = CF_SINF(t * 1.1f + i * 0.6f) * sway * 0.5f;
		float az = CF_SINF(t * 0.8f + i * 0.45f + 1.3f) * sway;
		CF_M4x4 local = cf_m4_identity();
		if (i > 0) local = cf_m4_translate(cf_v3(0, BONE_SPACING, 0));
		local = cf_mul(local, cf_mul(cf_m4_rotate_x(ax), cf_m4_rotate_z(az)));
		world = cf_mul(world, local);
		bones[i] = cf_mul(world, cf_m4_translate(cf_v3(0, -(float)i * BONE_SPACING, 0)));
	}
}

int main(int argc, char* argv[])
{
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
	CF_Result result = cf_make_app("cute_draw3d -- skinning", 0, 0, 0, 1280, 720, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_canvas_set_clear_color(cf_app_get_canvas(), cf_make_color_rgb_f(0.02f, 0.09f, 0.11f));

	CF_Mesh kelp = s_make_kelp();
	CF_Mesh sea_floor = s_make_floor();
	CF_Shader kelp_shd = cf_make_shader_from_source(s_kelp_vs, s_kelp_fs);
	CF_Shader floor_shd = cf_make_shader_from_source(s_floor_vs, s_floor_fs);

	// Kelp sways past vertical in a strong current; culling would wink facets out.
	CF_RenderState kelp_rs = cf_render_state_3d_defaults();
	kelp_rs.cull_mode = CF_CULL_MODE_NONE;

	// The bed: a scattering of strands on a disc, each with its own spot, spin, height and
	// tint. All of it per-instance state -- none of it splits the single instanced draw.
	typedef struct Strand { CF_V3 at; float spin; float scale; CF_V4 tint; } Strand;
	Strand strands[STRANDS];
	CF_Rnd rnd = cf_rnd_seed(11);
	for (int i = 0; i < STRANDS; ++i) {
		float a = cf_rnd_range_float(&rnd, 0, 2.0f * CF_PI);
		float r = CF_SQRTF(cf_rnd_range_float(&rnd, 0.05f, 1.0f)) * 4.5f;
		strands[i].at = cf_v3(CF_COSF(a) * r, 0, CF_SINF(a) * r);
		strands[i].spin = cf_rnd_range_float(&rnd, 0, 2.0f * CF_PI);
		strands[i].scale = cf_rnd_range_float(&rnd, 0.55f, 1.35f);
		float g = cf_rnd_range_float(&rnd, 0.55f, 0.95f);
		strands[i].tint = cf_v4(g * 0.35f, g, g * cf_rnd_range_float(&rnd, 0.45f, 0.7f), 1.0f);
	}

	float t = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		t += CF_DELTA_TIME;

		int w, h;
		cf_app_get_size(&w, &h);
		float ca = t * 0.12f;
		CF_V3 eye = cf_v3(CF_SINF(ca) * 7.5f, 2.6f + 0.6f * CF_SINF(t * 0.2f), CF_COSF(ca) * 7.5f);
		cf_draw3d_push_projection(cf_perspective(CF_PI / 3.4f, (float)w / (float)h, 0.1f, 60.0f));
		cf_draw3d_push_view(cf_look_at(eye, cf_v3(0, 1.1f, 0), cf_v3(0, 1, 0)));

		// This frame's pose, one MAT4 array shared by every strand.
		CF_M4x4 bones[BONES];
		s_pose_bones(bones, t);
		cf_draw3d_set_uniform("u_bones", bones, CF_UNIFORM_TYPE_MAT4, BONES);

		// The floor.
		cf_draw3d_push_shader(floor_shd);
		cf_draw3d_push();
		cf_draw3d_scale(cf_v3(30.0f, 1.0f, 30.0f));
		cf_draw3d_push_mesh_attributes(cf_v4(0.35f, 0.33f, 0.24f, 1));
		cf_draw3d_mesh(sea_floor);
		cf_draw3d_pop_mesh_attributes();
		cf_draw3d_pop();
		cf_draw3d_pop_shader();

		// The bed: sixty consecutive same-state submissions coalesce into one skinned,
		// instanced draw.
		cf_draw3d_push_shader(kelp_shd);
		cf_draw3d_push_render_state(kelp_rs);
		for (int i = 0; i < STRANDS; ++i) {
			cf_draw3d_push();
			cf_draw3d_translate(strands[i].at);
			cf_draw3d_rotate(cf_quat_from_axis_angle(cf_v3(0, 1, 0), strands[i].spin));
			cf_draw3d_scale(cf_v3(strands[i].scale));
			cf_draw3d_push_mesh_attributes(strands[i].tint);
			cf_draw3d_mesh(kelp);
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
			"%d kelp strands, one shared skeleton: one skinned, instanced draw.\n"
			"Bones are a MAT4 array uniform; the blend is the sample's vertex shader.", STRANDS);
		cf_draw_push_color(cf_make_color_rgb_f(0.75f, 0.88f, 0.88f));
		cf_draw_text(hud, cf_v2(-(float)w * 0.5f + 20.0f, (float)h * 0.5f - 20.0f), -1);
		cf_draw_pop_color();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_shader(kelp_shd);
	cf_destroy_shader(floor_shd);
	cf_destroy_mesh(kelp);
	cf_destroy_mesh(sea_floor);
	cf_destroy_app();
	return 0;
}
