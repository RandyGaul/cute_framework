// Holographic Radiance Cascades
//
// Demonstrates HRC 2D global illumination based on the Amitabha-style SSBO pipeline with f16 packing and 4-rotation frustum.
//
// Uses dense directions: cascade level n carries 2^(n+1)+1 ray directions with
// all-integer y-offsets (twice the paper's angular resolution). The paper's
// v_n(k) = (2^n, 2k - 2^n) only produces even offsets, which parity-segregates
// probe rows and causes the checkerboard artifact Eq 21's blur exists to hide.
// Dense directions couple every row, so the default output runs blur-free
// (debug mode 5 keeps the legacy blur for A/B comparison).
//
// Reference: Freeman, Sannikov, Margel (2025) "Holographic Radiance Cascades"
// https://arxiv.org/pdf/2505.02041
// https://github.com/entropylost/amitabha

#include <cute.h>

//--------------------------------------------------------------------------------------------------
// Configuration.

#define POW2_LOG2(n) ( \
	(n) >= 4096 ? 12 : \
	(n) >= 2048 ? 11 : \
	(n) >= 1024 ? 10 : \
	(n) >=  512 ?  9 : \
	(n) >=  256 ?  8 : \
	(n) >=  128 ?  7 : \
	(n) >=   64 ?  6 : \
	(n) >=   32 ?  5 : \
	(n) >=   16 ?  4 : \
	(n) >=    8 ?  3 : \
	(n) >=    4 ?  2 : \
	(n) >=    2 ?  1 : 0 \
)

#define HRC_WORLD_SIZE    1024
#define HRC_MAX_N         POW2_LOG2(HRC_WORLD_SIZE)
#define HRC_WG            16
#define HRC_ABS_THRESHOLD 0.1f
#define HRC_NUM_DEBUG 8   // debug modes 0..7

//--------------------------------------------------------------------------------------------------
// Shader loading.

CF_ComputeShader load_compute_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	cf_free(src);
	return cs;
}
//--------------------------------------------------------------------------------------------------
// HRC state.

typedef struct Hrc
{
	CF_Canvas emissivity;
	CF_Canvas absorption;
	CF_StorageBuffer vrays_rad[HRC_MAX_N + 1];
	CF_StorageBuffer vrays_trn[HRC_MAX_N + 1];
	CF_StorageBuffer r_rad[2];
	CF_StorageBuffer r_zero;
	CF_StorageBuffer frustum[4];
	CF_Canvas fluence;
	CF_Material mat_trace;
	CF_Material mat_extend;
	CF_Material mat_merge;
	CF_Material mat_composite;
	CF_Material mat_copy;
	CF_ComputeShader cs_seed;
	CF_ComputeShader cs_trace;
	CF_ComputeShader cs_extend;
	CF_ComputeShader cs_merge;
	CF_ComputeShader cs_copy;
	CF_ComputeShader cs_composite;
	int vrays_w[HRC_MAX_N + 1];
	int debug_mode;
	int grid;
	int n;
	int trace_levels; // how many cascade levels use direct trace (0..n+1)
	int cminus1;     // c-1 gathering enabled
	int upscale_mode; // 0=nearest, 1=minmax bilinear, 2=joint bilateral
	int blend_boundary; // angle-based weight blending near ±45° edges
	float blend_width;  // blend falloff width in direction slots
	int prefilter_mode; // 0=off, 1=box, 2=mipmap
	CF_Canvas minmax;
	CF_Material mat_minmax;
	CF_ComputeShader cs_minmax;
	CF_Canvas emissivity_filtered;
	CF_Canvas absorption_filtered;
	CF_Material mat_prefilter;
	CF_ComputeShader cs_prefilter;
	CF_ComputeShader cs_prefilter_gauss;
	CF_Canvas diffuse;      // per-pixel albedo for multibounce feedback (cornell test)
	CF_Canvas fluence_lin;  // last frame's linear fluence (feedback input)
	CF_Material mat_feedback;
	CF_ComputeShader cs_feedback;
} Hrc;

Hrc hrc;

//--------------------------------------------------------------------------------------------------
// Helpers.

CF_StorageBuffer hrc_make_buf(int w, int h)
{
	CF_StorageBufferParams p = cf_storage_buffer_defaults(w * h * 8);
	p.compute_readable = true;
	p.compute_writable = true;
	return cf_make_storage_buffer(p);
}

CF_Canvas hrc_make_canvas(int w, int h, CF_PixelFormat fmt, CF_Filter filter, bool allocate_mipmaps)
{
	CF_CanvasParams p = cf_canvas_defaults(w, h);
	p.target.pixel_format = fmt;
	p.target.filter = filter;
	p.target.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	p.target.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	p.target.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	if (allocate_mipmaps) {
		p.target.allocate_mipmaps = true;
		p.target.mip_count = 0;
	}
	return cf_make_canvas(p);
}

int hrc_div_ceil(int a, int b)
{
	return (a + b - 1) / b;
}

//--------------------------------------------------------------------------------------------------
// Init / shutdown.

void hrc_set_grid(int grid)
{
	hrc.grid = grid;
	hrc.n = POW2_LOG2(grid);
	for (int i = 0; i <= hrc.n; i++) {
		int interval = 1 << i;
		int rays = 2 * interval + 1; // dense directions: all-integer y-offsets
		int probes = grid >> i;
		hrc.vrays_w[i] = probes * rays;
	}
}

void hrc_init()
{
	CF_MEMSET(&hrc, 0, sizeof(hrc));

	// Start at half resolution.
	hrc_set_grid(HRC_WORLD_SIZE / 2);
	hrc.trace_levels = 3; // trace T_0..T_2, extend T_3..T_N
	hrc.cminus1 = 1;
	hrc.upscale_mode = 1;
	// Off by default: with dense directions + bracketed cone gathering the
	// quadrant seams already balance, and the blend only carves dark diagonals
	// (measured spoke/neighborhood ratios: blend off 0.98-1.05, blend on 0.90-0.96).
	hrc.blend_boundary = 0;
	// Mip+max: mipmap-averaged emissivity (smooth lights) + max-absorption
	// (conservative occlusion). Pure mipmap averaging melts thin opaque walls:
	// a 2px absorption-4 wall box-filtered at 2x becomes absorption 2 and leaks
	// ~13% per crossing (obvious in the pinhole test).
	hrc.prefilter_mode = 4;
	hrc.blend_width = 4.0f;

	// Precompute max T cascade dimensions for allocation.
	int max_vrays_w[HRC_MAX_N + 1];
	for (int i = 0; i <= HRC_MAX_N; i++) {
		int interval = 1 << i;
		int rays = 2 * interval + 1; // dense directions
		int probes = HRC_WORLD_SIZE >> i;
		max_vrays_w[i] = probes * rays;
	}

	// Scene input canvases (nearest-neighbor: discrete pixel grid, no sRGB-space blending).
	// Mipmaps allocated for hardware mipmap prefilter mode.
	hrc.emissivity = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, true);
	hrc.absorption = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, true);

	// Per-cascade T SSBOs (uvec2 per texel = 8 bytes, f16-packed). Allocated at max size.
	for (int i = 0; i <= HRC_MAX_N; i++) {
		hrc.vrays_rad[i] = hrc_make_buf(max_vrays_w[i], HRC_WORLD_SIZE);
		hrc.vrays_trn[i] = hrc_make_buf(max_vrays_w[i], HRC_WORLD_SIZE);
	}

	// R ping-pong SSBOs + zero buffer for R_N = 0.
	// Dense directions: R_n holds (grid >> n) * 2^(n+1) = 2 * grid entries per row.
	for (int i = 0; i < 2; i++)
		hrc.r_rad[i] = hrc_make_buf(HRC_WORLD_SIZE * 2, HRC_WORLD_SIZE);
	hrc.r_zero = hrc_make_buf(HRC_WORLD_SIZE * 2, HRC_WORLD_SIZE);
	{
		int sz = HRC_WORLD_SIZE * 2 * HRC_WORLD_SIZE * 8;
		void* zeros = cf_calloc(sz, 1);
		cf_update_storage_buffer(hrc.r_zero, zeros, sz);
		cf_free(zeros);
	}

	// Per-frustum output SSBOs (4 rotations, 2 cones per probe).
	for (int i = 0; i < 4; i++)
		hrc.frustum[i] = hrc_make_buf(HRC_WORLD_SIZE * 2, HRC_WORLD_SIZE);

	// Final output canvas (linear filtering for smooth display).
	hrc.fluence = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR, false);

	// Min/max absorption canvas (grid-res, allocated at world size to handle runtime grid changes).
	hrc.minmax = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16_FLOAT, CF_FILTER_NEAREST, false);

	// Prefiltered scene inputs (grid-res, allocated at world size).
	hrc.emissivity_filtered = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, false);
	hrc.absorption_filtered = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, false);

	// Multibounce feedback: diffuse albedo + last frame's linear fluence.
	hrc.diffuse = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, false);
	hrc.fluence_lin = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, false);

	// Materials.
	hrc.mat_trace = cf_make_material();
	hrc.mat_extend = cf_make_material();
	hrc.mat_merge = cf_make_material();
	hrc.mat_composite = cf_make_material();
	hrc.mat_copy = cf_make_material();
	hrc.mat_minmax = cf_make_material();
	hrc.mat_prefilter = cf_make_material();
	hrc.mat_feedback = cf_make_material();

	// Compute shaders (loaded from hrc_data/ next to the executable).
	hrc.cs_seed = load_compute_shader("/hrc_data/hrc_seed.c_shd");
	hrc.cs_trace = load_compute_shader("/hrc_data/hrc_trace.c_shd");
	hrc.cs_extend = load_compute_shader("/hrc_data/hrc_extend.c_shd");
	hrc.cs_merge = load_compute_shader("/hrc_data/hrc_merge.c_shd");
	hrc.cs_copy = load_compute_shader("/hrc_data/hrc_copy.c_shd");
	hrc.cs_composite = load_compute_shader("/hrc_data/hrc_composite.c_shd");
	hrc.cs_minmax = load_compute_shader("/hrc_data/hrc_minmax.c_shd");
	hrc.cs_prefilter = load_compute_shader("/hrc_data/hrc_prefilter.c_shd");
	hrc.cs_prefilter_gauss = load_compute_shader("/hrc_data/hrc_prefilter_gauss.c_shd");
	hrc.cs_feedback = load_compute_shader("/hrc_data/hrc_feedback.c_shd");
}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	for (int i = 0; i <= HRC_MAX_N; i++) {
		cf_destroy_storage_buffer(hrc.vrays_rad[i]);
		cf_destroy_storage_buffer(hrc.vrays_trn[i]);
	}
	for (int i = 0; i < 2; i++)
		cf_destroy_storage_buffer(hrc.r_rad[i]);
	cf_destroy_storage_buffer(hrc.r_zero);
	for (int i = 0; i < 4; i++)
		cf_destroy_storage_buffer(hrc.frustum[i]);
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_canvas(hrc.minmax);
	cf_destroy_canvas(hrc.emissivity_filtered);
	cf_destroy_canvas(hrc.absorption_filtered);
	cf_destroy_canvas(hrc.diffuse);
	cf_destroy_canvas(hrc.fluence_lin);
	cf_destroy_material(hrc.mat_feedback);
	cf_destroy_compute_shader(hrc.cs_feedback);
	cf_destroy_material(hrc.mat_trace);
	cf_destroy_material(hrc.mat_extend);
	cf_destroy_material(hrc.mat_merge);
	cf_destroy_material(hrc.mat_composite);
	cf_destroy_material(hrc.mat_copy);
	cf_destroy_material(hrc.mat_minmax);
	cf_destroy_material(hrc.mat_prefilter);
	cf_destroy_compute_shader(hrc.cs_seed);
	cf_destroy_compute_shader(hrc.cs_trace);
	cf_destroy_compute_shader(hrc.cs_extend);
	cf_destroy_compute_shader(hrc.cs_merge);
	cf_destroy_compute_shader(hrc.cs_copy);
	cf_destroy_compute_shader(hrc.cs_composite);
	cf_destroy_compute_shader(hrc.cs_minmax);
	cf_destroy_compute_shader(hrc.cs_prefilter);
	cf_destroy_compute_shader(hrc.cs_prefilter_gauss);
}

//--------------------------------------------------------------------------------------------------
// Off-screen light injection uniforms (resolved each frame in update_inject_uniforms).

typedef struct InjectUniforms
{
	int   merge_mode;    // merge shader: 0 = none, 2 = correct boundary injection
	int   composite_mode;// composite shader: 1 = naive unoccluded add, else 0
	int   type;          // 0 = infinite directional, 1 = finite point
	float dirx, diry;    // directional travel dir (from light into scene)
	float posx, posy;    // normalized world position
	float r, g, b;       // color hue (x anti-flicker fade); intensity is separate
	float falloff;       // point inverse-square coefficient
	float intensity;     // E: calibrated per-direction irradiance scale
	float angradius;     // directional soft-cone plateau half-angle (radians)
	float radius;        // point emitter radius (normalized world units)
	float softness;      // penumbra shoulder angular width (radians)
} InjectUniforms;

InjectUniforms g_inject;

// Push the static off-screen-light uniforms onto a material (merge or composite).
void set_inject_uniforms(CF_Material mat)
{
	cf_material_set_uniform_cs(mat, "u_light_type", &g_inject.type, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_cs(mat, "u_light_dirx", &g_inject.dirx, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_diry", &g_inject.diry, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_posx", &g_inject.posx, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_posy", &g_inject.posy, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_r", &g_inject.r, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_g", &g_inject.g, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_b", &g_inject.b, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_falloff", &g_inject.falloff, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_intensity", &g_inject.intensity, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_angradius", &g_inject.angradius, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_radius", &g_inject.radius, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(mat, "u_light_softness", &g_inject.softness, CF_UNIFORM_TYPE_FLOAT, 1);
}

//--------------------------------------------------------------------------------------------------
// Cascade compute pipeline.

void hrc_compute()
{
	CF_Texture emiss_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture absrp_tex = cf_canvas_get_target(hrc.absorption);
	CF_Texture fluence_tex = cf_canvas_get_target(hrc.fluence);
	CF_Texture minmax_tex = cf_canvas_get_target(hrc.minmax);

	int dim = hrc.grid;
	int N = hrc.n;

	// The trace shader marches raw full-resolution scene textures (prefiltering
	// quantizes moving emitters into visible flicker and melts thin occluders),
	// so no prefilter pass feeds the radiance path anymore. Only the probe
	// lattice is grid-res; each grid column is marched in S world-pixel steps.
	CF_Texture trace_emiss = emiss_tex;
	CF_Texture trace_absrp = absrp_tex;
	float mip_level = 0.0f;
	float absrp_mip_level = 0.0f;
	int upscale = HRC_WORLD_SIZE / dim;
	cf_material_set_uniform_cs(hrc.mat_trace, "u_mip_level", &mip_level, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(hrc.mat_trace, "u_absrp_mip_level", &absrp_mip_level, CF_UNIFORM_TYPE_FLOAT, 1);
	cf_material_set_uniform_cs(hrc.mat_trace, "u_upscale", &upscale, CF_UNIFORM_TYPE_INT, 1);

	// Off-screen boundary radiance injection (MECHANISM 1). Static per frame;
	// u_rotate and u_is_top are set per merge dispatch below.
	set_inject_uniforms(hrc.mat_merge);
	cf_material_set_uniform_cs(hrc.mat_merge, "u_inject_mode", &g_inject.merge_mode, CF_UNIFORM_TYPE_INT, 1);

	for (int j = 0; j < 4; j++) {
		// Seed T_0 when no levels are traced (sample at probe, no raymarching).
		if (hrc.trace_levels == 0) {
			int params[4] = { 0, j, dim, hrc.vrays_w[0] };
			cf_material_set_texture_cs(hrc.mat_trace, "u_emissivity", trace_emiss);
			cf_material_set_texture_cs(hrc.mat_trace, "u_absorption", trace_absrp);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_rotate", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_size", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[0], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[0], hrc.vrays_trn[0] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_seed, hrc.mat_trace, d);
		}

		// Direct trace T_0..T_{trace_levels-1}.
		for (int i = 0; i < hrc.trace_levels; i++) {
			int params[5] = { i, j, dim, hrc.vrays_w[i], hrc.blend_boundary };
			cf_material_set_texture_cs(hrc.mat_trace, "u_emissivity", trace_emiss);
			cf_material_set_texture_cs(hrc.mat_trace, "u_absorption", trace_absrp);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_rotate", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_world_size", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_blend_boundary", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_trace, "u_blend_width", &hrc.blend_width, CF_UNIFORM_TYPE_FLOAT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[i], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[i], hrc.vrays_trn[i] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_trace, hrc.mat_trace, d);
		}

		// Extend remaining levels.
		int ext_start = hrc.trace_levels > 0 ? hrc.trace_levels : 1;
		for (int i = ext_start; i <= N; i++) {
			int params[4] = { i, dim, hrc.vrays_w[i - 1], hrc.vrays_w[i] };
			cf_material_set_uniform_cs(hrc.mat_extend, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_world_size", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_prev_w", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_extend, "u_curr_w", params + 3, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(hrc.vrays_w[i], HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer ro[2] = { hrc.vrays_rad[i - 1], hrc.vrays_trn[i - 1] };
			d.ro_buffers = ro;
			d.ro_buffer_count = 2;
			CF_StorageBuffer rw[2] = { hrc.vrays_rad[i], hrc.vrays_trn[i] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 2;
			cf_dispatch_compute(hrc.cs_extend, hrc.mat_extend, d);
		}

		// Merge R_{N-1} down to R_0. R rows hold 2 * dim entries (dense directions).
		int r_ping = 0;
		for (int i = N - 1; i >= 0; i--) {
			int params[6] = { i, dim, hrc.vrays_w[i], hrc.vrays_w[i + 1], dim * 2, dim * 2 };
			int is_top = (i == N - 1) ? 1 : 0; // R_{i+1} == R_N base case at top
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_world_size", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_curr_w", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_next_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_prev_w", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_curr_w", params + 5, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_rotate", &j, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_is_top", &is_top, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(dim * 2, HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer r_prev = (i == N - 1) ? hrc.r_zero : hrc.r_rad[1 - r_ping];
			CF_StorageBuffer ro[5] = {
				hrc.vrays_rad[i], hrc.vrays_trn[i],
				hrc.vrays_rad[i + 1], hrc.vrays_trn[i + 1],
				r_prev
			};
			d.ro_buffers = ro;
			d.ro_buffer_count = 5;
			CF_StorageBuffer rw[1] = { hrc.r_rad[r_ping] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_dispatch_compute(hrc.cs_merge, hrc.mat_merge, d);
			r_ping = 1 - r_ping;
		}

		// Copy R_0 -> frustum[j] (2 cones per probe).
		{
			int count = dim * dim * 2;
			cf_material_set_uniform_cs(hrc.mat_copy, "u_count", &count, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(count, 256),
				1,
				1
			);
			CF_StorageBuffer ro[1] = { hrc.r_rad[1 - r_ping] };
			d.ro_buffers = ro;
			d.ro_buffer_count = 1;
			CF_StorageBuffer rw[1] = { hrc.frustum[j] };
			d.rw_buffers = rw;
			d.rw_buffer_count = 1;
			cf_dispatch_compute(hrc.cs_copy, hrc.mat_copy, d);
		}
	}

	// Minmax absorption pass: compute per-grid-cell min/max absorption for bilinear R_0 upscaling.
	// Dispatched at world resolution so the composite shader can read at world coordinates directly.
	if (hrc.upscale_mode == 1) {
		int world = HRC_WORLD_SIZE;
		cf_material_set_texture_cs(hrc.mat_minmax, "u_absorption", absrp_tex);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_grid_size", &dim, CF_UNIFORM_TYPE_INT, 1);

		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(world, HRC_WG),
			hrc_div_ceil(world, HRC_WG),
			1
		);
		CF_Texture rw_tex[1] = { minmax_tex };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 1;
		cf_dispatch_compute(hrc.cs_minmax, hrc.mat_minmax, d);
	}

	// Composite: c-1 gathering, sum 4 quadrants, cross blur, output.
	{
		int world = HRC_WORLD_SIZE;
		float abs_thresh = HRC_ABS_THRESHOLD;
		int debug = hrc.debug_mode <= 5 ? hrc.debug_mode : 0;
		cf_material_set_texture_cs(hrc.mat_composite, "u_emissivity", emiss_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_absorption", absrp_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_minmax", minmax_tex);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_grid_size", &dim, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_debug_mode", &debug, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_cminus1", &hrc.cminus1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_upscale_mode", &hrc.upscale_mode, CF_UNIFORM_TYPE_INT, 1);

		// Naive off-screen light (MODE 1 negative control) params.
		set_inject_uniforms(hrc.mat_composite);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_inject_mode", &g_inject.composite_mode, CF_UNIFORM_TYPE_INT, 1);

		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(world, HRC_WG),
			hrc_div_ceil(world, HRC_WG),
			1
		);
		CF_StorageBuffer ro[4] = {
			hrc.frustum[0], hrc.frustum[1],
			hrc.frustum[2], hrc.frustum[3]
		};
		d.ro_buffers = ro;
		d.ro_buffer_count = 4;
		CF_Texture rw_tex[2] = { fluence_tex, cf_canvas_get_target(hrc.fluence_lin) };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 2;
		cf_dispatch_compute(hrc.cs_composite, hrc.mat_composite, d);
	}
}

// Multibounce feedback (amitabha multibounce.rs): inject last frame's fluence,
// tinted by albedo, into the emissivity canvas. Converges across frames.
void hrc_feedback()
{
	int world = HRC_WORLD_SIZE;
	cf_material_set_uniform_cs(hrc.mat_feedback, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);

	CF_ComputeDispatch d = cf_compute_dispatch_defaults(
		hrc_div_ceil(world, HRC_WG),
		hrc_div_ceil(world, HRC_WG),
		1
	);
	CF_Texture ro_tex[3] = {
		cf_canvas_get_target(hrc.fluence_lin),
		cf_canvas_get_target(hrc.diffuse),
		cf_canvas_get_target(hrc.absorption)
	};
	d.ro_textures = ro_tex;
	d.ro_texture_count = 3;
	CF_Texture rw_tex[1] = { cf_canvas_get_target(hrc.emissivity) };
	d.rw_textures = rw_tex;
	d.rw_texture_count = 1;
	cf_dispatch_compute(hrc.cs_feedback, hrc.mat_feedback, d);
}

//--------------------------------------------------------------------------------------------------
// Demo scene state.

typedef struct OrbLight
{
	float radius;
	float speed;
	float angle;
	CF_Color color;
} OrbLight;

float time_acc = 0.0f;
int test_scene = 0; // 0 = off, 1 = static centered light, 2 = slowly orbiting light, 3 = cornell, 4 = pinhole, 5 = off-screen demo

//--------------------------------------------------------------------------------------------------
// Off-screen light demo (test_scene == 5).
//
// A bright HDR point light ORBITS on a radius large enough to leave and re-enter
// the screen (part of each orbit is off-screen on every side). On-screen walls
// shadow the interior when the light sits off an edge. Modes select how the
// off-screen light is handled:
//   0 = no injection      (light simply vanishes off-screen -- the baseline pop)
//   1 = naive unoccluded  (WRONG: light bleeds through on-screen walls; control)
//   2 = correct boundary  (MECHANISM 1: injected in-cascade, walls still occlude)
//   3 = padded + fade     (MECHANISM 2: sim a margin, display center crop, fade)
int   inject_mode = 0;

#define HRC_MARGIN_FACTOR 1.3f   // mode 3 padded region = screen x this factor
#define OFF_ORBIT_RADIUS  680.0f // > half world (512), so it leaves/re-enters
#define OFF_LIGHT_HDR     10.0f  // linear HDR radiance (on-screen emitter body)
#define OFF_LIGHT_RADIUS  20.0f  // emitter disk radius (world px)
// Soft-cone off-screen light defaults (finite angular size, smooth falloff).
// intensity E is calibrated so the off-screen soft light matches an on-screen
// disc of the same intended brightness (see A/B), NOT the raw HDR body value
// that made the injected boundary source flood the scene.
#define OFF_LIGHT_INTENSITY 8.0f  // calibrated per-direction irradiance scale
#define OFF_DIR_ANG_DEG    8.0f   // directional soft-cone plateau half-angle (deg)
#define OFF_SOFT_DEG       12.0f  // penumbra shoulder angular width (deg)

// Warm HDR light color in LINEAR radiance units (matches emiss decode: pow*16).
CF_V2 off_light_pos;      // world pixels (may lie outside [0, world])
float off_light_fade;     // mode-3 anti-flicker fade 0..1 across the margin band
int   off_light_onscreen; // 1 if within [0, world]^2

static float off_clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static float off_smooth(float e0, float e1, float x)
{
	float t = off_clamp01((x - e0) / (e1 - e0));
	return t * t * (3.0f - 2.0f * t);
}
static float off_min4(float a, float b, float c, float d)
{
	float m = a < b ? a : b;
	m = m < c ? m : c;
	return m < d ? m : d;
}

// Compute the orbiting light's world position + mode-3 fade for this frame.
// Deterministic when HRC_ORBIT is set (screenshot seeding): angle taken directly.
void update_offscreen_light()
{
	float ws = (float)HRC_WORLD_SIZE;
	float half = ws * 0.5f;

	// HRC_LX/HRC_LY explicitly place the light (world pixels) for controlled
	// straight-line anti-flicker sweeps; otherwise orbit (HRC_ORBIT seeds phase).
	const char* lxs = getenv("HRC_LX");
	const char* lys = getenv("HRC_LY");
	if (lxs && lys) {
		off_light_pos = cf_v2((float)atof(lxs), (float)atof(lys));
	} else {
		float ang;
		const char* orbit_env = getenv("HRC_ORBIT");
		if (orbit_env) ang = (float)atof(orbit_env);
		else           ang = time_acc * 0.4f;
		off_light_pos = cf_v2(half + cosf(ang) * OFF_ORBIT_RADIUS,
		                      half + sinf(ang) * OFF_ORBIT_RADIUS);
	}

	float pnx = off_light_pos.x / ws;
	float pny = off_light_pos.y / ws;
	off_light_onscreen = (pnx >= 0.0f && pnx < 1.0f && pny >= 0.0f && pny < 1.0f) ? 1 : 0;

	// Anti-flicker fade: 0 at the padded outer edge, ramping to 1 by the time the
	// light reaches the visible screen crop, so the ramp happens entirely in the
	// off-screen margin band and visible pixels only ever see full strength.
	if (inject_mode == 3) {
		// Ramp 0 (world edge) -> 1 by 60% into the margin band, so the light is at
		// full strength before it reaches the visible crop AND margin lights that
		// sit behind an off-screen occluder (Case B) are still bright enough to
		// cast a shadow. The whole ramp still lives in the off-screen margin.
		float margin_norm = (1.0f - 1.0f / HRC_MARGIN_FACTOR) * 0.5f;
		float m = off_min4(pnx, 1.0f - pnx, pny, 1.0f - pny); // dist to nearest world edge
		off_light_fade = off_smooth(0.0f, margin_norm * 0.6f, m);
	} else {
		off_light_fade = 1.0f;
	}
}

void update_inject_uniforms()
{
	CF_MEMSET(&g_inject, 0, sizeof(g_inject));
	float ws = (float)HRC_WORLD_SIZE;
	g_inject.type = 1;
	g_inject.falloff = 0.5f;
	// Soft-cone parameters (env-overridable so the A/B harness can sweep them
	// without a rebuild): angular_radius, softness, point radius, intensity.
	float dir_ang = OFF_DIR_ANG_DEG;
	float soft_deg = OFF_SOFT_DEG;
	float pt_radius_px = OFF_LIGHT_RADIUS;
	g_inject.intensity = OFF_LIGHT_INTENSITY;
	const char* e;
	if ((e = getenv("HRC_LANG")))  dir_ang = (float)atof(e);        // directional plateau (deg)
	if ((e = getenv("HRC_LSOFT"))) soft_deg = (float)atof(e);       // penumbra shoulder (deg)
	if ((e = getenv("HRC_LRAD")))  pt_radius_px = (float)atof(e);   // point emitter radius (px)
	if ((e = getenv("HRC_LINT")))  g_inject.intensity = (float)atof(e); // intensity E
	g_inject.angradius = dir_ang * CF_PI / 180.0f;
	g_inject.softness  = soft_deg * CF_PI / 180.0f;
	g_inject.radius    = pt_radius_px / ws; // normalized world units
	if (test_scene != 5) return;

	const char* lt = getenv("HRC_LIGHTTYPE");
	if (lt) g_inject.type = atoi(lt); // 0 = directional, 1 = point

	g_inject.posx = off_light_pos.x / ws;
	g_inject.posy = off_light_pos.y / ws;

	// Directional travel dir: from the light toward the world center.
	float tx = 0.5f - g_inject.posx, ty = 0.5f - g_inject.posy;
	float tl = sqrtf(tx * tx + ty * ty);
	if (tl < 1e-5f) tl = 1.0f;
	g_inject.dirx = tx / tl;
	g_inject.diry = ty / tl;

	// Color is now a unit-ish hue; brightness lives in intensity E (x fade).
	float b = off_light_fade;
	g_inject.r = 1.00f * b;
	g_inject.g = 0.85f * b;
	g_inject.b = 0.60f * b;

	// Mode 2 = MECHANISM 1 (in-cascade boundary injection, correctly occluded).
	// Mode 3 = MECHANISM 2 relies on the padded in-world emitter + fade, no
	// boundary injection. Mode 1 = naive composite add. Mode 0 = nothing.
	if (inject_mode == 2) g_inject.merge_mode = 2;
	if (inject_mode == 1) g_inject.composite_mode = 1;
}

// Test light center for this frame. The light is 8x8 world pixels -- the paper
// calls out sub-8x8 emitters as a known HRC limitation, so the reference test
// stays at the supported minimum. Mode 2 orbits slowly to expose swimming.
CF_V2 test_light_center()
{
	float half = (float)HRC_WORLD_SIZE * 0.5f;
	CF_V2 c = cf_v2(half, half);
	if (test_scene == 2) {
		c.x += cosf(time_acc * 0.25f) * 60.0f;
		c.y += sinf(time_acc * 0.25f) * 60.0f;
	}
	return c;
}

void draw_test_light(CF_Color color)
{
	CF_V2 c = test_light_center();
	cf_draw_push_color(color);
	cf_draw_quad_fill(cf_make_aabb(cf_v2(c.x - 4.0f, c.y - 4.0f), cf_v2(c.x + 4.0f, c.y + 4.0f)), 0);
	cf_draw_pop_color();
}

// Cornell (test 3) and pinhole (test 4) scenes, ported from amitabha's
// multibounce.rs and scene.rs for reference comparison. One function draws
// all shapes into a chosen channel: 0 = emissivity, 1 = absorption, 2 = diffuse.
void draw_rect_ch(float x0, float y0, float x1, float y1, CF_Color c)
{
	if (c.r == 0 && c.g == 0 && c.b == 0) return;
	cf_draw_push_color(c);
	cf_draw_quad_fill(cf_make_aabb(cf_v2(x0, y0), cf_v2(x1, y1)), 0);
	cf_draw_pop_color();
}

void draw_circle_ch(float x, float y, float r, CF_Color c)
{
	if (c.r == 0 && c.g == 0 && c.b == 0) return;
	cf_draw_push_color(c);
	cf_draw_circle_fill2(cf_v2(x, y), r);
	cf_draw_pop_color();
}

#define CH(e, a, d) (channel == 0 ? (e) : channel == 1 ? (a) : (d))

void draw_cornell(int channel)
{
	CF_Color black = cf_make_color_rgb_f(0, 0, 0);
	// cf_draw vertex colors clamp to 8-bit [0,1], so the absorption canvas
	// stores per-pixel OPACITY (1 - exp(-absorption)) and the emissivity canvas
	// stores emission/16; the cascade shaders decode on sample. Solid = opaque.
	// Opacity 0.9 (absorption ~2.3/px): walls stay sealed across their 20px
	// thickness but surface texels still receive fluence, so the multibounce
	// feedback can tint them (opacity 1.0 starves the bounce and kills bleed).
	CF_Color solid = cf_make_color_rgb_f(0.9f, 0.9f, 0.9f);
	CF_Color grey  = cf_make_color_rgb_f(0.9f, 0.9f, 0.9f);
	// Canvas y is up: visual top = world y near 1024. Ceiling (top) / floor (bottom).
	draw_rect_ch(0, 1004, 1024, 1024, CH(black, solid, grey));
	draw_rect_ch(0, 0, 1024, 20, CH(black, solid, grey));
	// Red left wall, blue right wall (diffuse tint; absorption stays neutral).
	draw_rect_ch(0, 0, 20, 1024, CH(black, solid, cf_make_color_rgb_f(0.9f, 0.05f, 0.05f)));
	draw_rect_ch(1004, 0, 1024, 1024, CH(black, solid, cf_make_color_rgb_f(0.05f, 0.05f, 0.9f)));
	// Light frame + emitter slot at top center.
	draw_rect_ch(444, 1004, 580, 1024, CH(black, solid, black));
	draw_rect_ch(448, 1004, 576, 1024, CH(cf_make_color_rgb_f(0.467f, 0.467f, 0.467f), cf_make_color_rgb_f(0.632f, 0.632f, 0.632f), black));
	// Occluder circle on the floor (pure absorber, zero albedo).
	draw_circle_ch(320, 212, 192, CH(black, solid, black));
	// Translucent smoky box on the floor.
	draw_rect_ch(672, 20, 864, 404, CH(black, cf_make_color_rgb_f(0.0198f, 0.0149f, 0.0198f), cf_make_color_rgb_f(0.8f, 0.8f, 0.8f)));
}

void draw_pinhole(int channel)
{
	CF_Color black = cf_make_color_rgb_f(0, 0, 0);
	// cf_draw vertex colors clamp to 8-bit [0,1], so the absorption canvas
	// stores per-pixel OPACITY (1 - exp(-absorption)) and the emissivity canvas
	// stores emission/16; the cascade shaders decode on sample. Solid = opaque.
	CF_Color solid = cf_make_color_rgb_f(1.0f, 1.0f, 1.0f);
	// Wall with a moving 10px gap (the gap sweeps to expose swimming).
	// HRC_NOGAP=1 closes the gap entirely: any light on the right side is then
	// a true wall leak rather than smeared gap light.
	if (getenv("HRC_NOGAP")) {
		draw_rect_ch(511, 0, 513, 1024, CH(black, solid, black));
	} else {
		float l = sinf(time_acc * 0.5f) * 200.0f;
		draw_rect_ch(511, 0, 513, 507 + l, CH(black, solid, black));
		draw_rect_ch(511, 517 + l, 513, 1024, CH(black, solid, black));
	}
	// 7 colored emitter strips along the left edge (golden-angle-ish hues).
	CF_Color strip[7] = {
		cf_make_color_rgb_f(0.70f, 0.30f, 1.00f),
		cf_make_color_rgb_f(1.00f, 0.80f, 0.20f),
		cf_make_color_rgb_f(0.20f, 0.90f, 0.90f),
		cf_make_color_rgb_f(1.00f, 0.30f, 0.50f),
		cf_make_color_rgb_f(0.30f, 1.00f, 0.40f),
		cf_make_color_rgb_f(0.35f, 0.50f, 1.00f),
		cf_make_color_rgb_f(1.00f, 0.55f, 0.15f),
	};
	for (int i = -3; i <= 3; i++) {
		CF_Color c = strip[i + 3];
		// Emission encodes at /16; full-scale channel = effective 16x brightness.
		CF_Color e = c;
		float cy = 512.0f - (float)i * 40.0f;
		// Opaque surface emitters like the reference (radiance = emiss * (1 - T)
		// needs full absorption for full emission; the surface texels radiate,
		// the interior blocks).
		draw_rect_ch(19, cy - 20, 29, cy + 20, CH(e, solid, black));
	}
}

// A/B reference: a STATIC on-screen disc light of matched angular size, placed to
// shadow the same left wall from a comparable direction as the off-screen light.
// This is the "good" reference the off-screen soft-cone shadow is measured against
// (soft penumbra, no staircase) and the brightness calibration target. Enabled by
// HRC_REFLIGHT; position/size overridable via HRC_REFX/HRC_REFY/HRC_REFRAD.
void draw_ref_light(int channel)
{
	if (!getenv("HRC_REFLIGHT")) return;
	float rx = 140.0f, ry = 512.0f, rr = 8.0f;
	const char* e;
	if ((e = getenv("HRC_REFX")))   rx = (float)atof(e);
	if ((e = getenv("HRC_REFY")))   ry = (float)atof(e);
	if ((e = getenv("HRC_REFRAD"))) rr = (float)atof(e);
	if (channel == 1) {
		// Opaque so it can radiate (rad = emiss * (1 - T)).
		draw_circle_ch(rx, ry, rr, cf_make_color_rgb_f(0.632f, 0.632f, 0.632f));
	} else {
		// Same warm hue + HDR level as the off-screen light body.
		float inv = 1.0f / 2.2f;
		float er = powf(OFF_LIGHT_HDR * 1.00f / 16.0f, inv);
		float eg = powf(OFF_LIGHT_HDR * 0.85f / 16.0f, inv);
		float eb = powf(OFF_LIGHT_HDR * 0.60f / 16.0f, inv);
		draw_circle_ch(rx, ry, rr, cf_make_color_rgb_f(er, eg, eb));
	}
}

// Off-screen demo scene. channel: 0 = emissivity, 1 = absorption.
// On-screen occluder walls + (mode 3 only) an off-screen margin occluder, plus
// the orbiting light drawn as an emitter while it is on-screen.
void draw_offscreen(int channel)
{
	CF_Color solid = cf_make_color_rgb_f(1.0f, 1.0f, 1.0f); // opaque occluder
	draw_ref_light(channel);

	if (channel == 1) {
		cf_draw_push_color(solid);
		// Three on-screen occluder walls. When the light is off the left/right
		// edge these cast shadows into the interior (Case A).
		draw_rect_ch(292.0f, 372.0f, 308.0f, 652.0f, solid); // left vertical wall
		draw_rect_ch(716.0f, 372.0f, 732.0f, 652.0f, solid); // right vertical wall
		draw_rect_ch(372.0f, 716.0f, 652.0f, 732.0f, solid); // top horizontal wall
		cf_draw_pop_color();

		// Case B: an off-screen occluder living inside the mode-3 padded margin.
		// It exists only when mechanism 2 is active -- the boundary-injection
		// mechanism (mode 2) has no knowledge of geometry beyond the screen, so
		// this wall is absent from its world. Mode 3 simulates the margin and so
		// shadows the visible crop correctly where mode 2 cannot.
		if (inject_mode == 3 && !getenv("HRC_NOCCL")) {
			draw_rect_ch(92.0f, 400.0f, 108.0f, 624.0f, solid); // left margin band (off crop)
		}
	}

	// The light body (opaque emitter) while it is on-screen; off-screen frames
	// rely on boundary injection / padded sim instead.
	if (off_light_onscreen) {
		float b = off_light_fade;
		if (channel == 1) {
			// Opaque so it can radiate (rad = emiss * (1 - T)).
			draw_circle_ch(off_light_pos.x, off_light_pos.y, OFF_LIGHT_RADIUS, cf_make_color_rgb_f(0.632f, 0.632f, 0.632f));
		} else {
			// Emission encode: stored = (Blin/16)^(1/2.2) so decode pow*16 = Blin.
			float inv = 1.0f / 2.2f;
			float er = powf(OFF_LIGHT_HDR * 1.00f * b / 16.0f, inv);
			float eg = powf(OFF_LIGHT_HDR * 0.85f * b / 16.0f, inv);
			float eb = powf(OFF_LIGHT_HDR * 0.60f * b / 16.0f, inv);
			draw_circle_ch(off_light_pos.x, off_light_pos.y, OFF_LIGHT_RADIUS, cf_make_color_rgb_f(er, eg, eb));
		}
	}
}

void draw_test_shapes(int channel)
{
	if (test_scene == 3) draw_cornell(channel);
	else if (test_scene == 5) draw_offscreen(channel);
	else draw_pinhole(channel);
}

OrbLight orbs[4];

void scene_init()
{
	orbs[0] = (OrbLight){ 140.0f, 0.7f, 0.0f,        cf_make_color_rgb_f(1.0f, 0.2f, 0.1f) };
	orbs[1] = (OrbLight){ 160.0f, -0.5f, CF_PI * 0.5f,   cf_make_color_rgb_f(0.1f, 1.0f, 0.2f) };
	orbs[2] = (OrbLight){ 120.0f, 0.9f, CF_PI,            cf_make_color_rgb_f(0.2f, 0.3f, 1.0f) };
	orbs[3] = (OrbLight){ 180.0f, -0.3f, CF_PI * 1.5f,    cf_make_color_rgb_f(1.0f, 0.9f, 0.1f) };
}

//--------------------------------------------------------------------------------------------------
// Input handling.

void handle_input()
{
	if (cf_key_just_pressed(CF_KEY_D)) {
		hrc.debug_mode = (hrc.debug_mode + 1) % HRC_NUM_DEBUG;
	}
	if (cf_key_just_pressed(CF_KEY_T)) {
		test_scene = (test_scene + 1) % 6;
	}
	if (cf_key_just_pressed(CF_KEY_I)) {
		inject_mode = (inject_mode + 1) % 4;
	}
	if (cf_key_just_pressed(CF_KEY_H)) {
		if      (hrc.grid == 128)  hrc_set_grid(256);
		else if (hrc.grid == 256)  hrc_set_grid(512);
		else if (hrc.grid == 512)  hrc_set_grid(1024);
		else                       hrc_set_grid(128);
		if (hrc.trace_levels > hrc.n + 1)
			hrc.trace_levels = hrc.n + 1;
	}
	if (cf_key_just_pressed(CF_KEY_R)) {
		hrc.trace_levels = (hrc.trace_levels + 1) % (hrc.n + 2);
	}
	if (cf_key_just_pressed(CF_KEY_C)) {
		hrc.cminus1 = !hrc.cminus1;
	}
	if (cf_key_just_pressed(CF_KEY_M)) {
		hrc.upscale_mode = (hrc.upscale_mode + 1) % 3;
	}
	if (cf_key_just_pressed(CF_KEY_P)) {
		hrc.prefilter_mode = (hrc.prefilter_mode + 1) % 5;
	}
	if (cf_key_just_pressed(CF_KEY_B)) {
		hrc.blend_boundary = !hrc.blend_boundary;
	}
	if (cf_key_just_pressed(CF_KEY_LEFTBRACKET)) {
		hrc.blend_width = cf_max(0.5f, hrc.blend_width - 0.5f);
	}
	if (cf_key_just_pressed(CF_KEY_RIGHTBRACKET)) {
		hrc.blend_width += 0.5f;
	}
}

//--------------------------------------------------------------------------------------------------
// Drawing helpers.

void begin_canvas_draw()
{
	cf_draw_push();
	float ws = (float)HRC_WORLD_SIZE;
	float half = ws * 0.5f;
	cf_draw_TSR_absolute(cf_v2(0, 0), cf_v2(1, 1), 0);
	cf_draw_projection(cf_ortho_2d(0, 0, ws, ws));
	cf_draw_translate(-half, -half);
}

void end_canvas_draw()
{
	cf_draw_pop();
}

void draw_circle_at(float x, float y, float r)
{
	cf_draw_circle_fill2(cf_v2(x, y), r);
}

// Push a render state with premultiplied alpha blending and the correct pixel format for f16 canvases.
void push_f16_render_state()
{
	CF_RenderState rs = cf_render_state_defaults();
	rs.blend.pixel_format = CF_PIXEL_FORMAT_R16G16B16A16_FLOAT;
	rs.blend.rgb_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.rgb_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.rgb_op = CF_BLEND_OP_ADD;
	rs.blend.alpha_src_blend_factor = CF_BLENDFACTOR_ONE;
	rs.blend.alpha_dst_blend_factor = CF_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	rs.blend.alpha_op = CF_BLEND_OP_ADD;
	cf_draw_push_render_state(rs);
}

// Per-frame light positions (computed once, drawn into both canvases).
typedef struct Light
{
	float x, y, r;
	CF_Color color;
} Light;

#define MAX_LIGHTS 16
Light frame_lights[MAX_LIGHTS];
int frame_light_count;

// Compute all light positions for this frame and store them.
void update_lights()
{
	float half = (float)HRC_WORLD_SIZE * 0.5f;
	float ws = (float)HRC_WORLD_SIZE;
	float dt = CF_DELTA_TIME;
	time_acc += dt;
	frame_light_count = 0;

	// Orbiting lights.
	for (int i = 0; i < 4; i++) {
		orbs[i].angle += orbs[i].speed * dt;
		float cx = half + cosf(orbs[i].angle) * orbs[i].radius;
		float cy = half + sinf(orbs[i].angle) * orbs[i].radius;
		frame_lights[frame_light_count++] = (Light){ cx, cy, 15.0f, orbs[i].color };
	}

	// Corner accent lights.
	{
		float margin = 60.0f;
		CF_Color corners[4] = {
			cf_make_color_rgb_f(0.0f, 0.4f, 0.4f),
			cf_make_color_rgb_f(0.5f, 0.3f, 0.0f),
			cf_make_color_rgb_f(0.4f, 0.1f, 0.3f),
			cf_make_color_rgb_f(0.2f, 0.5f, 0.0f),
		};
		float lx[4] = { margin, ws - margin, margin, ws - margin };
		float ly[4] = { margin, margin, ws - margin, ws - margin };
		for (int i = 0; i < 4; i++) {
			frame_lights[frame_light_count++] = (Light){ lx[i], ly[i], 8.0f, corners[i] };
		}
	}
}

// Draw all lights as colored circles (for emissivity canvas).
void draw_lights_emissive()
{
	for (int i = 0; i < frame_light_count; i++) {
		CF_Color c = frame_lights[i].color;
		// Emission decode is pow(c, 2.2) * 16, so the sRGB-space encode scale is
		// 16^(-1/2.2) = 0.2832 (a plain /16 lands ~28x too dark after the curve).
		cf_draw_push_color(cf_make_color_rgb_f(c.r * 0.2832f, c.g * 0.2832f, c.b * 0.2832f));
		draw_circle_at(frame_lights[i].x, frame_lights[i].y, frame_lights[i].r);
		cf_draw_pop_color();
	}
}

// Draw all lights as white circles (for absorption canvas).
// Light sources are opaque emitters -- they must absorb to emit.
void draw_lights_absorbing()
{
	cf_draw_push_color(cf_make_color_rgb_f(0.632f, 0.632f, 0.632f)); // absorption 1.0 -> opacity
	for (int i = 0; i < frame_light_count; i++) {
		draw_circle_at(frame_lights[i].x, frame_lights[i].y, frame_lights[i].r);
	}
	cf_draw_pop_color();
}

//--------------------------------------------------------------------------------------------------
// Draw emissivity (lights).

void draw_emissivity()
{
	begin_canvas_draw();
	push_f16_render_state();

	if (test_scene == 1 || test_scene == 2) {
		draw_test_light(cf_make_color_rgb_f(0.2832f, 0.2832f, 0.2832f)); // emission 1.0, sRGB-space encode
	} else if (test_scene >= 3) {
		draw_test_shapes(0);
	} else {
		draw_lights_emissive();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.emissivity, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Draw absorption (shadow casters + light sources).

void draw_absorption()
{
	begin_canvas_draw();
	push_f16_render_state();

	if (test_scene == 1 || test_scene == 2) {
		draw_test_light(cf_make_color_rgb_f(0.632f, 0.632f, 0.632f)); // absorption 1.0 -> opacity
	} else if (test_scene >= 3) {
		draw_test_shapes(1);
	} else {
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 1.0f));

		float ws = (float)HRC_WORLD_SIZE;
		float half = ws * 0.5f;

		// 5 circular pillars in quincunx pattern.
		draw_circle_at(half, half, 25.0f);
		draw_circle_at(half - 110.0f, half - 110.0f, 20.0f);
		draw_circle_at(half + 110.0f, half - 110.0f, 20.0f);
		draw_circle_at(half - 110.0f, half + 110.0f, 20.0f);
		draw_circle_at(half + 110.0f, half + 110.0f, 20.0f);

		// 2 angled wall segments.
		cf_draw_quad_fill2(
			cf_v2(100.0f, 200.0f), cf_v2(110.0f, 200.0f),
			cf_v2(200.0f, 310.0f), cf_v2(190.0f, 310.0f), 0
		);
		cf_draw_quad_fill2(
			cf_v2(400.0f, 200.0f), cf_v2(390.0f, 200.0f),
			cf_v2(300.0f, 310.0f), cf_v2(310.0f, 310.0f), 0
		);

		// 3 small scattered square blocks.
		cf_draw_quad_fill(cf_make_aabb(cf_v2(340.0f, 400.0f), cf_v2(365.0f, 425.0f)), 0);
		cf_draw_quad_fill(cf_make_aabb(cf_v2(150.0f, 380.0f), cf_v2(175.0f, 405.0f)), 0);
		cf_draw_quad_fill(cf_make_aabb(cf_v2(370.0f, 120.0f), cf_v2(395.0f, 145.0f)), 0);

		cf_draw_pop_color();

		// Light sources must absorb to emit (radiative transfer: rad = emiss * (1 - T)).
		draw_lights_absorbing();
	}

	cf_draw_pop_render_state();
	cf_render_to(hrc.absorption, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Draw diffuse albedo (multibounce feedback input; only the cornell test uses it).

void draw_diffuse()
{
	begin_canvas_draw();
	push_f16_render_state();
	if (test_scene == 3) draw_test_shapes(2);
	cf_draw_pop_render_state();
	cf_render_to(hrc.diffuse, true);
	end_canvas_draw();
}

//--------------------------------------------------------------------------------------------------
// Display fluence onto screen.

void display_fluence()
{
	CF_Canvas display = hrc.fluence;
	if (hrc.debug_mode == 6) display = hrc.emissivity;
	else if (hrc.debug_mode == 7) display = hrc.absorption;

	float ws = (float)HRC_WORLD_SIZE;
	// MECHANISM 2 (mode 3): the full world is the padded region; display only the
	// center screen crop by zooming so the crop fills the window. Off-screen
	// margin geometry (simulated) is cropped out of view but still shadows.
	float scale = (test_scene == 5 && inject_mode == 3) ? HRC_MARGIN_FACTOR : 1.0f;
	cf_draw_canvas(display, cf_v2(0, 0), cf_v2(ws * scale, ws * scale));
}

//--------------------------------------------------------------------------------------------------
// HUD overlay.

void draw_hud()
{
	const char* mode_names[] = {
		"0: Normal (no blur)",
		"1: Rot 0 (+x)",
		"2: Rot 1 (+y)",
		"3: Rot 2 (-x)",
		"4: Rot 3 (-y)",
		"5: Cross blur (legacy)",
		"6: Emissivity",
		"7: Absorption",
	};

	char buf[512];
	snprintf(buf, sizeof(buf),
		"[B] Blend boundary: %s\n"
		"[[] []] Blend width: %.1f\n"
		"[C] c-1 gathering: %s\n"
		"[D] Debug: %s\n"
		"[H] Grid: %d (%s)\n"
		"[M] Upscale: %s\n"
		"[P] Prefilter: %s\n"
		"[R] Trace: %d / %d\n"
		"[I] Off-screen: %s\n"
		"[T] Test scene: %s",
		hrc.blend_boundary ? "on" : "off",
		hrc.blend_width,
		hrc.cminus1 ? "on" : "off",
		mode_names[hrc.debug_mode],
		hrc.grid, hrc.grid == HRC_WORLD_SIZE ? "1x" : hrc.grid == HRC_WORLD_SIZE/2 ? "2x" : hrc.grid == HRC_WORLD_SIZE/4 ? "4x" : "8x",
		hrc.upscale_mode == 0 ? "nearest" : hrc.upscale_mode == 1 ? "minmax" : "bilateral",
		hrc.prefilter_mode == 0 ? "off" : hrc.prefilter_mode == 1 ? "box" : hrc.prefilter_mode == 2 ? "mipmap" : hrc.prefilter_mode == 3 ? "gauss" : "mip+max",
		hrc.trace_levels, hrc.n + 1,
		inject_mode == 0 ? "0 none" : inject_mode == 1 ? "1 naive (bleed)" : inject_mode == 2 ? "2 boundary inject" : "3 padded+fade",
		test_scene == 0 ? "off" : test_scene == 1 ? "static 8x8" : test_scene == 2 ? "orbiting 8x8" : test_scene == 3 ? "cornell" : test_scene == 4 ? "pinhole" : "off-screen demo"
	);

	float half = (float)HRC_WORLD_SIZE * 0.5f;
	cf_draw_push_color(cf_color_white());
	cf_push_font_size(16.0f);
	cf_draw_text(buf, cf_v2(-half + 10.0f, half - 20.0f), -1);
	cf_pop_font_size();
	cf_draw_pop_color();
}

//--------------------------------------------------------------------------------------------------
// Entry point.

int main(int argc, char* argv[])
{
	cf_make_app("HRC - Holographic Radiance Cascades", 0, 0, 0, HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	hrc_init();
	scene_init();

	// Dev overrides for headless/scripted runs (same toggles as the HUD keys).
	{
		const char* env;
		if ((env = getenv("HRC_TEST")))        test_scene = atoi(env);
		if ((env = getenv("HRC_DEBUG")))       hrc.debug_mode = atoi(env);
		if ((env = getenv("HRC_BLEND")))       hrc.blend_boundary = atoi(env);
		if ((env = getenv("HRC_BLEND_WIDTH"))) hrc.blend_width = (float)atof(env);
		if ((env = getenv("HRC_CM1")))         hrc.cminus1 = atoi(env);
		if ((env = getenv("HRC_TRACE")))       hrc.trace_levels = atoi(env);
		if ((env = getenv("HRC_INJECT")))      inject_mode = atoi(env);
	}

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_draw_push_shape_aa(0);
		handle_input();
		update_lights();
		update_offscreen_light();
		update_inject_uniforms();
		draw_emissivity();
		draw_absorption();
		draw_diffuse();
		if (test_scene == 3) hrc_feedback();
		hrc_compute();
		display_fluence();
		draw_hud();
		cf_app_draw_onto_screen(true);
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
