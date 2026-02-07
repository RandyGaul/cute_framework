// GPU N-body galaxy collision simulation.
//
// 16,384 stars stored as position/velocity in a 128x128 RGBA32F texture.
// A compute shader runs gravitational N-body simulation with shared-memory
// tiling and leapfrog integration. Two spiral galaxies approach each other
// and merge, producing tidal tails and bridges.
//
// Controls: none (just watch the galaxies collide).

#include <cute.h>
#include <math.h>
#include <stdlib.h>

#define TEX_SIZE       128
#define PARTICLE_COUNT (TEX_SIZE * TEX_SIZE)
#define SUB_STEPS      4
#define DT             0.0005f
#define CENTRAL_MASS   150.0f
#define GALAXY_G       0.0001f
#define SOFTENING      0.005f
#define QUAD_SIZE      0.004f
#define BULGE_FRAC     0.1f
#define PI             3.14159265f

// Galaxy collision: two galaxies offset from center with approach velocities.
#define GAL_RADIUS     0.30f
#define GAL_A_CX      -0.25f
#define GAL_A_CY       0.04f
#define GAL_A_VX       0.025f
#define GAL_A_VY      -0.008f
#define GAL_B_CX       0.25f
#define GAL_B_CY      -0.04f
#define GAL_B_VX      -0.025f
#define GAL_B_VY       0.008f

// ---------------------------------------------------------------------------
// Simulation compute shader.
// One thread per particle. Reads state from sampler, writes to storage image.
// Central-body gravity + particle-particle N-body via shared memory tiling.
#define TILE_SIZE 256
const char* s_sim_cs =
	"layout(set = 0, binding = 0) uniform sampler2D u_state_in;\n"
	"layout(set = 1, binding = 0, rgba32f) uniform writeonly image2D u_state_out;\n"
	"layout(set = 2, binding = 0) uniform sim_params {\n"
	"    float u_dt;\n"
	"    float u_softening_sq;\n"
	"    float u_central_mass;\n"
	"    float u_G;\n"
	"    float u_tex_size;\n"
	"};\n"
	"layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n"
	"shared vec4 tile[256];\n"
	"void main()\n"
	"{\n"
	"    ivec2 my_coord = ivec2(gl_GlobalInvocationID.xy);\n"
	"    int tex_sz = int(u_tex_size);\n"
	"    vec2 my_uv = (vec2(my_coord) + 0.5) / u_tex_size;\n"
	"    vec4 me = texture(u_state_in, my_uv);\n"
	"    vec2 pos = me.xy;\n"
	"    vec2 vel = me.zw;\n"
	"    vec2 acc = vec2(0.0);\n"
	"    vec2 d = -pos;\n"
	"    float dist_sq = dot(d, d) + u_softening_sq;\n"
	"    float inv_dist = inversesqrt(dist_sq);\n"
	"    float inv_dist3 = inv_dist * inv_dist * inv_dist;\n"
	"    acc += d * (u_G * u_central_mass * inv_dist3);\n"
	"    uint gid = uint(my_coord.y * tex_sz + my_coord.x);\n"
	"    uint lid = gl_LocalInvocationIndex;\n"
	"    uint total = uint(tex_sz * tex_sz);\n"
	"    uint num_tiles = (total + 255u) / 256u;\n"
	"    for (uint t = 0u; t < num_tiles; t++) {\n"
	"        uint idx = t * 256u + lid;\n"
	"        if (idx < total) {\n"
	"            ivec2 tc = ivec2(int(idx) % tex_sz, int(idx) / tex_sz);\n"
	"            tile[lid] = texelFetch(u_state_in, tc, 0);\n"
	"        } else {\n"
	"            tile[lid] = vec4(0.0);\n"
	"        }\n"
	"        barrier();\n"
	"        for (uint j = 0u; j < 256u; j++) {\n"
	"            uint other = t * 256u + j;\n"
	"            if (other >= total || other == gid) continue;\n"
	"            vec2 dp = tile[j].xy - pos;\n"
	"            float dsq = dot(dp, dp) + u_softening_sq;\n"
	"            float id2 = inversesqrt(dsq);\n"
	"            float id3 = id2 * id2 * id2;\n"
	"            acc += dp * (u_G * id3);\n"
	"        }\n"
	"        barrier();\n"
	"    }\n"
	"    vel += acc * u_dt;\n"
	"    pos += vel * u_dt;\n"
	"    if (my_coord.x < tex_sz && my_coord.y < tex_sz)\n"
	"        imageStore(u_state_out, my_coord, vec4(pos, vel));\n"
	"}\n";

// ---------------------------------------------------------------------------
// Star vertex shader.
// Per-vertex: quad corner position and local UV for falloff.
// Per-instance: UV coordinate into state texture to fetch star position.
const char* s_star_vs =
	"layout(location = 0) in vec2 in_pos;\n"
	"layout(location = 1) in vec2 in_local;\n"
	"layout(location = 2) in vec2 in_uv;\n"
	"layout(location = 0) out vec2 v_local;\n"
	"layout(location = 1) out float v_speed;\n"
	"layout(set = 0, binding = 0) uniform sampler2D u_state;\n"
	"layout(set = 1, binding = 0) uniform star_params {\n"
	"    float u_quad_size;\n"
	"    float u_aspect;\n"
	"};\n"
	"void main()\n"
	"{\n"
	"    vec4 s = texture(u_state, in_uv);\n"
	"    vec2 star_pos = s.xy;\n"
	"    vec2 star_vel = s.zw;\n"
	"    v_speed = length(star_vel);\n"
	"    v_local = in_local;\n"
	"    vec2 offset = in_pos * u_quad_size;\n"
	"    offset.x /= u_aspect;\n"
	"    gl_Position = vec4(star_pos + offset, 0.0, 1.0);\n"
	"}\n";

// ---------------------------------------------------------------------------
// Star fragment shader.
// Gaussian circular falloff, velocity-based color mapping, premultiplied output.
const char* s_star_fs =
	"layout(location = 0) in vec2 v_local;\n"
	"layout(location = 1) in float v_speed;\n"
	"layout(location = 0) out vec4 result;\n"
	"void main()\n"
	"{\n"
	"    float r = length(v_local);\n"
	"    if (r > 1.0) discard;\n"
	"    float intensity = exp(-r * r * 4.0);\n"
	"    float t = clamp(v_speed * 3.0, 0.0, 1.0);\n"
	"    vec3 warm = vec3(1.0, 0.7, 0.3);\n"
	"    vec3 cool = vec3(0.5, 0.7, 1.0);\n"
	"    vec3 color = mix(warm, cool, t);\n"
	"    result = vec4(color * intensity, intensity);\n"
	"}\n";

// ---------------------------------------------------------------------------
// Quad vertex data.
typedef struct QuadVertex
{
	CF_V2 pos;
	CF_V2 local;
} QuadVertex;

// Per-instance data: UV into state texture.
typedef struct StarInstance
{
	CF_V2 uv;
} StarInstance;

// ---------------------------------------------------------------------------
// Random float in [lo, hi].
float randf(float lo, float hi)
{
	return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

int main(int argc, char* argv[]) {
	CF_Result result = cf_make_app("Galaxy N-Body", 0, 0, 0, 900, 900, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// -- State textures (ping-pong pair). --
	CF_TextureParams tp = cf_texture_defaults(TEX_SIZE, TEX_SIZE);
	tp.pixel_format = CF_PIXEL_FORMAT_R32G32B32A32_FLOAT;
	tp.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	tp.filter = CF_FILTER_NEAREST;
	CF_Texture state[2];
	state[0] = cf_make_texture(tp);
	state[1] = cf_make_texture(tp);

	// -- Generate galaxy collision initial conditions. --
	// Two spiral galaxies at offset positions with opposing bulk velocities.
	float* init_data = (float*)malloc(PARTICLE_COUNT * 4 * sizeof(float));
	srand(42);
	int half = PARTICLE_COUNT / 2;
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		float* p = init_data + i * 4;

		// Which galaxy?
		int gal = (i >= half) ? 1 : 0;
		float cx = gal ? GAL_B_CX : GAL_A_CX;
		float cy = gal ? GAL_B_CY : GAL_A_CY;
		float bvx = gal ? GAL_B_VX : GAL_A_VX;
		float bvy = gal ? GAL_B_VY : GAL_A_VY;
		float dir = gal ? -1.0f : 1.0f; // Counter-rotating galaxies.
		int local_i = gal ? (i - half) : i;

		float r, angle;
		if (local_i < (int)(half * BULGE_FRAC)) {
			// Central bulge.
			r = randf(0.0f, GAL_RADIUS * 0.15f);
			angle = randf(0.0f, 2.0f * PI);
		} else {
			// Spiral arms.
			r = randf(0.01f, 1.0f);
			r = -0.2f * logf(r);
			if (r > GAL_RADIUS) r = randf(0.05f, GAL_RADIUS);

			int arm = local_i % 2;
			float base_angle = arm * PI;
			float spiral = 2.5f * logf(r + 0.1f);
			angle = base_angle + spiral + randf(-0.25f, 0.25f);
		}

		float px = cx + r * cosf(angle);
		float py = cy + r * sinf(angle);

		// Orbital velocity around local galaxy center.
		float speed = sqrtf(GALAXY_G * (float)half / (r + SOFTENING)) * 0.3f;
		float vx = bvx + dir * (-speed * sinf(angle));
		float vy = bvy + dir * ( speed * cosf(angle));

		vx += randf(-0.005f, 0.005f);
		vy += randf(-0.005f, 0.005f);

		p[0] = px;
		p[1] = py;
		p[2] = vx;
		p[3] = vy;
	}
	cf_texture_update(state[0], init_data, PARTICLE_COUNT * 4 * (int)sizeof(float));
	free(init_data);

	// -- Compute shader + material for simulation. --
	CF_ComputeShader sim_shader = cf_make_compute_shader_from_source(s_sim_cs);
	CF_Material sim_material = cf_make_material();

	// -- Graphics shader + material for star rendering. --
	CF_Shader star_shader = cf_make_shader_from_source(s_star_vs, s_star_fs);
	CF_Material star_material = cf_make_material();

	// Additive blend state.
	CF_RenderState rs = cf_render_state_defaults();
	rs.blend.enabled = true;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	cf_material_set_render_state(star_material, rs);

	// -- Mesh: instanced star quads. --
	// Two triangles forming a quad, with local coords for Gaussian falloff.
	QuadVertex quad_verts[6] = {
		{ cf_v2(-1, -1), cf_v2(-1, -1) },
		{ cf_v2( 1, -1), cf_v2( 1, -1) },
		{ cf_v2( 1,  1), cf_v2( 1,  1) },
		{ cf_v2(-1, -1), cf_v2(-1, -1) },
		{ cf_v2( 1,  1), cf_v2( 1,  1) },
		{ cf_v2(-1,  1), cf_v2(-1,  1) },
	};

	CF_VertexAttribute attrs[3] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(QuadVertex, pos);
	attrs[1].name = "in_local";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(QuadVertex, local);
	attrs[2].name = "in_uv";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(StarInstance, uv);
	attrs[2].per_instance = true;

	CF_Mesh mesh = cf_make_mesh(sizeof(quad_verts), attrs, 3, sizeof(QuadVertex));
	cf_mesh_update_vertex_data(mesh, quad_verts, 6);

	// Instance data: one UV per particle pointing into the state texture.
	StarInstance* instances = (StarInstance*)malloc(PARTICLE_COUNT * sizeof(StarInstance));
	for (int i = 0; i < PARTICLE_COUNT; i++) {
		int x = i % TEX_SIZE;
		int y = i / TEX_SIZE;
		instances[i].uv = cf_v2(((float)x + 0.5f) / TEX_SIZE, ((float)y + 0.5f) / TEX_SIZE);
	}
	cf_mesh_set_instance_buffer(mesh, PARTICLE_COUNT * (int)sizeof(StarInstance), (int)sizeof(StarInstance));
	cf_mesh_update_instance_data(mesh, instances, PARTICLE_COUNT);
	free(instances);

	int cur = 0;

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// -- Compute pass: N-body simulation sub-steps. --
		float dt = DT;
		float softening_sq = SOFTENING * SOFTENING;
		float central_mass = CENTRAL_MASS;
		float grav = GALAXY_G;
		float tex_size = (float)TEX_SIZE;

		for (int step = 0; step < SUB_STEPS; step++) {
			cf_material_set_texture_cs(sim_material, "u_state_in", state[cur]);
			cf_material_set_uniform_cs(sim_material, "u_dt", &dt, CF_UNIFORM_TYPE_FLOAT, 1);
			cf_material_set_uniform_cs(sim_material, "u_softening_sq", &softening_sq, CF_UNIFORM_TYPE_FLOAT, 1);
			cf_material_set_uniform_cs(sim_material, "u_central_mass", &central_mass, CF_UNIFORM_TYPE_FLOAT, 1);
			cf_material_set_uniform_cs(sim_material, "u_G", &grav, CF_UNIFORM_TYPE_FLOAT, 1);
			cf_material_set_uniform_cs(sim_material, "u_tex_size", &tex_size, CF_UNIFORM_TYPE_FLOAT, 1);

			int groups = TEX_SIZE / 16;
			CF_ComputeDispatch dispatch = cf_compute_dispatch_defaults(groups, groups, 1);
			dispatch.rw_textures = &state[1 - cur];
			dispatch.rw_texture_count = 1;
			cf_dispatch_compute(sim_shader, sim_material, dispatch);

			cur = 1 - cur;
		}

		// -- Graphics pass: render stars as additive glowing quads. --
		cf_apply_canvas(cf_app_get_canvas(), true);

		cf_material_set_texture_vs(star_material, "u_state", state[cur]);
		float quad_size = QUAD_SIZE;
		float aspect = 1.0f; // Square window.
		cf_material_set_uniform_vs(star_material, "u_quad_size", &quad_size, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_vs(star_material, "u_aspect", &aspect, CF_UNIFORM_TYPE_FLOAT, 1);

		cf_apply_mesh(mesh);
		cf_apply_shader(star_shader, star_material);
		cf_draw_elements();

		cf_app_draw_onto_screen(false);
	}

	cf_destroy_compute_shader(sim_shader);
	cf_destroy_shader(star_shader);
	cf_destroy_material(sim_material);
	cf_destroy_material(star_material);
	cf_destroy_mesh(mesh);
	cf_destroy_texture(state[0]);
	cf_destroy_texture(state[1]);
	cf_destroy_app();

	return 0;
}
