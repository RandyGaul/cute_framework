// GPU-driven particles: the whole loop lives on the GPU.
//
// 65,536 ember particles simulated by a compute shader in a storage buffer, drawn by
// pull instancing (gl_InstanceIndex reads the same buffer -- no per-instance vertex
// data at all), with the draw's argument block written by a second compute pass and
// consumed via cf_draw_elements_indirect. The CPU never touches a particle and never
// learns how many are alive: simulate -> write args -> draw, zero readback.
//
// This is the modern counterpart to the galaxy sample (which predates storage buffers
// and round-trips state through textures). SDL_GPU backends only -- compute and
// indirect draws don't exist on GLES3/web.
//
// Controls: none (embers rise from a ring and swirl; the camera orbits).
// Harness: --shot <t> saves a numbered png, --exit-at <t> quits cleanly.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>

#define PARTICLE_COUNT 65536
#define GROUP_SIZE     256
#define EMIT_PER_SEC   16384.0f // Ramp: the GPU brings particles online over ~4 seconds.

// ---------------------------------------------------------------------------
// Simulation: one thread per particle, state in place. A particle is two vec4s --
// (pos.xyz, life) and (vel.xyz, seed). Dead particles respawn on a ring with a
// hashed velocity; particles beyond the ramp count stay dormant so the indirect
// instance count below is honest about what's alive.
const char* s_sim_cs =
"layout(std430, set = 1, binding = 0) buffer Particles { vec4 data[]; } u_particles;\n"
"layout(set = 2, binding = 0) uniform sim_params {\n"
"    float u_time;\n"
"    float u_dt;\n"
"    float u_alive;\n"
"};\n"
"layout(local_size_x = 256) in;\n"
"uint hash(uint x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }\n"
"float hash01(uint x) { return float(hash(x) & 0xffffffu) / 16777215.0; }\n"
"void main()\n"
"{\n"
"    uint i = gl_GlobalInvocationID.x;\n"
"    vec4 ps = u_particles.data[i * 2u + 0u];\n"
"    vec4 vs = u_particles.data[i * 2u + 1u];\n"
"    if (float(i) >= u_alive) return;\n"
"    ps.w -= u_dt;\n"
"    if (ps.w <= 0.0) {\n"
"        // Respawn on the emitter ring, velocity hashed from the particle index\n"
"        // and respawn generation so no two lives repeat.\n"
"        uint gen = uint(vs.w) + 1u;\n"
"        uint h = hash(i * 747796405u + gen * 2891336453u);\n"
"        float a = hash01(h) * 6.2831853;\n"
"        float r = 0.55 + 0.1 * hash01(h ^ 0x9e3779b9u);\n"
"        ps.xyz = vec3(cos(a) * r, 0.02 * hash01(h ^ 0x85ebca6bu), sin(a) * r);\n"
"        ps.w = 2.0 + 3.0 * hash01(h ^ 0xc2b2ae35u);\n"
"        vs.xyz = vec3(0.0, 0.15 + 0.25 * hash01(h ^ 0x27d4eb2fu), 0.0);\n"
"        vs.w = float(gen);\n"
"    }\n"
"    // Swirl about y, gentle inward pull, buoyant rise with a hashed flicker.\n"
"    vec3 p = ps.xyz;\n"
"    vec3 swirl = vec3(-p.z, 0.0, p.x) * 0.6;\n"
"    vec3 inward = vec3(-p.x, 0.0, -p.z) * 0.25;\n"
"    vec3 rise = vec3(0.0, 0.35 + 0.1 * sin(u_time * 3.0 + float(i)), 0.0);\n"
"    vs.xyz += (swirl + inward + rise - vs.xyz) * (1.0 - exp(-2.0 * u_dt));\n"
"    ps.xyz += vs.xyz * u_dt;\n"
"    u_particles.data[i * 2u + 0u] = ps;\n"
"    u_particles.data[i * 2u + 1u] = vs;\n"
"}\n";

// ---------------------------------------------------------------------------
// Argument pass: one thread writes the indirect draw's argument block. The layout is
// CF_DrawIndirectArgs -- four uint32s, exactly one uvec4: (vertex_count,
// instance_count, first_vertex, first_instance). The instance count ramps on the GPU;
// the CPU issues the draw without ever knowing it.
const char* s_args_cs =
"layout(std430, set = 1, binding = 0) writeonly buffer Args { uvec4 data[]; } u_args;\n"
"layout(set = 2, binding = 0) uniform args_params {\n"
"    float u_alive;\n"
"};\n"
"layout(local_size_x = 1) in;\n"
"void main()\n"
"{\n"
"    u_args.data[0] = uvec4(6u, uint(u_alive), 0u, 0u);\n"
"}\n";

// ---------------------------------------------------------------------------
// Render: pull instancing. The mesh holds only 6 corner vertices; everything
// per-particle comes from the same storage buffer the simulation writes, indexed by
// gl_InstanceIndex. Camera-facing quads sized and faded by remaining life.
const char* s_draw_vs =
"layout(location = 0) in vec2 in_corner;\n"
"layout(location = 0) out vec2 v_local;\n"
"layout(location = 1) out float v_heat;\n"
"layout(std430, set = 0, binding = 0) readonly buffer Particles { vec4 u_particles[]; };\n"
"layout(set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_vp;\n"
"    vec4 u_right;\n"
"    vec4 u_up;\n"
"};\n"
"void main()\n"
"{\n"
"    vec4 ps = u_particles[uint(gl_InstanceIndex) * 2u + 0u];\n"
"    vec4 vs = u_particles[uint(gl_InstanceIndex) * 2u + 1u];\n"
"    float fade = clamp(ps.w * 0.8, 0.0, 1.0);\n"
"    float size = 0.007 * (0.5 + fade);\n"
"    v_local = in_corner;\n"
"    v_heat = clamp(length(vs.xyz) * 1.5, 0.0, 1.0) * fade;\n"
"    vec3 world = ps.xyz + (u_right.xyz * in_corner.x + u_up.xyz * in_corner.y) * size;\n"
"    gl_Position = u_vp * vec4(world, 1.0);\n"
"}\n";

const char* s_draw_fs =
"layout(location = 0) in vec2 v_local;\n"
"layout(location = 1) in float v_heat;\n"
"layout(location = 0) out vec4 result;\n"
"void main()\n"
"{\n"
"    float r2 = dot(v_local, v_local);\n"
"    float glow = exp(-r2 * 6.0) * v_heat * 0.35;\n"
"    vec3 hot = vec3(1.0, 0.85, 0.45);\n"
"    vec3 cool = vec3(1.0, 0.3, 0.05);\n"
"    result = vec4(mix(cool, hot, v_heat) * glow, glow);\n"
"}\n";

// ---------------------------------------------------------------------------
// Screenshot/exit harness, same contract as the fireflies and model3d samples.
static int s_shot_count;
static void s_screenshot(void)
{
	int w, h;
	CF_Canvas canvas = cf_app_get_canvas();
	cf_canvas_get_size(canvas, &w, &h);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));
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
		snprintf(path, sizeof(path), "/gpu_particles_shot_%02d.png", s_shot_count++);
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

	CF_Result result = cf_make_app("GPU Particles -- compute, storage, indirect", 0, 0, 0, 1280, 720, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;
	cf_fs_set_write_directory(cf_fs_get_base_directory()); // Screenshots land beside the exe.
	cf_clear_color(0.015f, 0.01f, 0.02f, 1.0f); // Near-black: additive embers need darkness.

	// Particle state: written by compute, pulled by the vertex stage. Starts zeroed,
	// so every particle begins dead and respawns on the GPU as the ramp reaches it.
	CF_StorageBufferParams sbp = cf_storage_buffer_defaults(PARTICLE_COUNT * 2 * (int)sizeof(CF_V4));
	sbp.compute_writable = true;
	sbp.compute_readable = true;
	sbp.graphics_readable = true;
	CF_StorageBuffer particles = cf_make_storage_buffer(sbp);
	{
		void* zeros = cf_calloc(PARTICLE_COUNT * 2 * (int)sizeof(CF_V4), 1);
		cf_update_storage_buffer(particles, zeros, PARTICLE_COUNT * 2 * (int)sizeof(CF_V4));
		cf_free(zeros);
	}

	// The indirect argument block, GPU-written and never read back.
	CF_StorageBufferParams abp = cf_storage_buffer_defaults((int)sizeof(CF_DrawIndirectArgs));
	abp.compute_writable = true;
	abp.indirect_drawable = true;
	CF_StorageBuffer args = cf_make_storage_buffer(abp);

	CF_ComputeShader sim_shader = cf_make_compute_shader_from_source(s_sim_cs);
	CF_ComputeShader args_shader = cf_make_compute_shader_from_source(s_args_cs);
	CF_Material sim_material = cf_make_material();
	CF_Material args_material = cf_make_material();

	CF_Shader draw_shader = cf_make_shader_from_source(s_draw_vs, s_draw_fs);
	CF_Material draw_material = cf_make_material();
	CF_RenderState rs = cf_render_state_defaults();
	rs.blend.enabled = true;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE;
	cf_material_set_render_state(draw_material, rs);

	// Six corners; every other vertex byte a particle needs lives in the storage buffer.
	CF_V2 corners[6] = {
		{ -1, -1 }, { 1, -1 }, { 1, 1 },
		{ -1, -1 }, { 1, 1 }, { -1, 1 },
	};
	CF_VertexAttribute attr = { 0 };
	attr.name = "in_corner";
	attr.format = CF_VERTEX_FORMAT_FLOAT2;
	attr.offset = 0;
	CF_Mesh mesh = cf_make_mesh(sizeof(corners), &attr, 1, sizeof(CF_V2));
	cf_mesh_update_vertex_data(mesh, corners, 6);

	float time = 0;
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		float dt = CF_DELTA_TIME;
		time += dt;
		float alive = EMIT_PER_SEC * time;
		if (alive > (float)PARTICLE_COUNT) alive = (float)PARTICLE_COUNT;

		// -- Compute: simulate every live particle in place. --
		cf_material_set_uniform_cs(sim_material, "u_time", &time, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(sim_material, "u_dt", &dt, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(sim_material, "u_alive", &alive, CF_UNIFORM_TYPE_FLOAT, 1);
		CF_ComputeDispatch sim = cf_compute_dispatch_defaults(PARTICLE_COUNT / GROUP_SIZE, 1, 1);
		sim.rw_buffers = &particles;
		sim.rw_buffer_count = 1;
		cf_dispatch_compute(sim_shader, sim_material, sim);

		// -- Compute: write the indirect draw arguments. --
		cf_material_set_uniform_cs(args_material, "u_alive", &alive, CF_UNIFORM_TYPE_FLOAT, 1);
		CF_ComputeDispatch ad = cf_compute_dispatch_defaults(1, 1, 1);
		ad.rw_buffers = &args;
		ad.rw_buffer_count = 1;
		cf_dispatch_compute(args_shader, args_material, ad);

		// -- Render: one indirect draw, arguments straight from the GPU. --
		int w, h;
		cf_canvas_get_size(cf_app_get_canvas(), &w, &h);
		float orbit = time * 0.25f;
		CF_V3 eye = cf_v3(cosf(orbit) * 2.5f, 1.4f, sinf(orbit) * 2.5f);
		CF_M4x4 view = cf_look_at(eye, cf_v3(0, 0.8f, 0), cf_v3(0, 1, 0));
		CF_M4x4 proj = cf_perspective(CF_PI / 3.0f, (float)w / (float)h, 0.1f, 20.0f);
		CF_M4x4 vp = cf_mul_m4(proj, view);
		// Billboard basis: the view matrix's rows are the camera axes.
		CF_V4 right = cf_v4(view.elements[0], view.elements[4], view.elements[8], 0);
		CF_V4 up = cf_v4(view.elements[1], view.elements[5], view.elements[9], 0);
		cf_material_set_uniform_vs(draw_material, "u_vp", &vp, CF_UNIFORM_TYPE_MAT4, 1);
		cf_material_set_uniform_vs(draw_material, "u_right", &right, CF_UNIFORM_TYPE_FLOAT4, 1);
		cf_material_set_uniform_vs(draw_material, "u_up", &up, CF_UNIFORM_TYPE_FLOAT4, 1);

		cf_apply_canvas(cf_app_get_canvas(), true);
		cf_apply_mesh(mesh);
		cf_apply_shader(draw_shader, draw_material);
		cf_apply_vs_storage_buffers(&particles, 1);
		cf_draw_elements_indirect(args, 0, 1);

		if (shot_n && time >= shot_times[0]) {
			s_screenshot();
			for (int i = 1; i < shot_n; ++i) shot_times[i - 1] = shot_times[i];
			shot_n--;
		}
		cf_app_draw_onto_screen(false);
		if (exit_at > 0 && time >= exit_at) break;
	}

	cf_destroy_compute_shader(sim_shader);
	cf_destroy_compute_shader(args_shader);
	cf_destroy_shader(draw_shader);
	cf_destroy_material(sim_material);
	cf_destroy_material(args_material);
	cf_destroy_material(draw_material);
	cf_destroy_mesh(mesh);
	cf_destroy_storage_buffer(particles);
	cf_destroy_storage_buffer(args);
	cf_destroy_app();
	return 0;
}
