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
#include <stdio.h>
#include <string.h>

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
#define HRC_MIP_BLOCK 8   // minmax-mip block size (world px) ~ "8px cells"

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
	CF_Canvas minmax_mip;        // coarse (absMin, absMax, emisMax) blocks for marching
	CF_ComputeShader cs_minmax_mip;
	int mip_march;               // minmax-mip cell skipping on the trace DDA + c-1 march
	int c1_selective;            // skip the c-1 march in uniform-open cells (use R_0 only)
	int max_levels;              // cap the cascade this many levels (-1 = full N); R at cap is the R_N=0 boundary
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

// Total bytes of all grid-dependent cascade SSBOs at a given grid resolution.
// uvec2 = 8 bytes/texel; two T buffers per level (rad+trn), three R buffers
// (ping-pong pair + zero), four frustum buffers. Buffer height = grid rows.
size_t hrc_buffer_bytes(int grid)
{
	int n = POW2_LOG2(grid);
	size_t total = 0;
	for (int i = 0; i <= n; i++) {
		int rays = 2 * (1 << i) + 1;
		int probes = grid >> i;
		size_t w = (size_t)probes * rays;
		total += (size_t)2 * w * grid * 8; // rad + trn
	}
	size_t r_row = (size_t)grid * 2 * grid * 8;
	total += 3 * r_row; // r_rad[2] + r_zero
	total += 4 * r_row; // frustum[4]
	return total;
}

// Allocate the grid-dependent cascade SSBOs for the currently-set grid.
void hrc_alloc_buffers()
{
	int grid = hrc.grid;
	int n = hrc.n;
	for (int i = 0; i <= n; i++) {
		hrc.vrays_rad[i] = hrc_make_buf(hrc.vrays_w[i], grid);
		hrc.vrays_trn[i] = hrc_make_buf(hrc.vrays_w[i], grid);
	}
	for (int i = 0; i < 2; i++)
		hrc.r_rad[i] = hrc_make_buf(grid * 2, grid);
	hrc.r_zero = hrc_make_buf(grid * 2, grid);
	{
		int sz = grid * 2 * grid * 8;
		void* zeros = cf_calloc(sz, 1);
		cf_update_storage_buffer(hrc.r_zero, zeros, sz);
		cf_free(zeros);
	}
	for (int i = 0; i < 4; i++)
		hrc.frustum[i] = hrc_make_buf(grid * 2, grid);
}

// Destroy all grid-dependent cascade SSBOs (guarded so unused levels are skipped).
void hrc_free_buffers()
{
	for (int i = 0; i <= HRC_MAX_N; i++) {
		if (hrc.vrays_rad[i].id) { cf_destroy_storage_buffer(hrc.vrays_rad[i]); hrc.vrays_rad[i].id = 0; }
		if (hrc.vrays_trn[i].id) { cf_destroy_storage_buffer(hrc.vrays_trn[i]); hrc.vrays_trn[i].id = 0; }
	}
	for (int i = 0; i < 2; i++)
		if (hrc.r_rad[i].id) { cf_destroy_storage_buffer(hrc.r_rad[i]); hrc.r_rad[i].id = 0; }
	if (hrc.r_zero.id) { cf_destroy_storage_buffer(hrc.r_zero); hrc.r_zero.id = 0; }
	for (int i = 0; i < 4; i++)
		if (hrc.frustum[i].id) { cf_destroy_storage_buffer(hrc.frustum[i]); hrc.frustum[i].id = 0; }
}

// Change grid and reallocate the cascade buffers to fit (destroy + recreate).
void hrc_resize(int grid)
{
	size_t before = hrc_buffer_bytes(hrc.grid);
	hrc_free_buffers();
	hrc_set_grid(grid);
	hrc_alloc_buffers();
	// Report to stderr so the bench CSV on stdout stays clean.
	fprintf(stderr, "[hrc] grid -> %d: cascade buffers %.1f MB -> %.1f MB\n",
		grid, before / (1024.0 * 1024.0), hrc_buffer_bytes(grid) / (1024.0 * 1024.0));
}

// Analytical per-grid memory report (printed once at init).
void hrc_print_memory_table()
{
	printf("[hrc] per-grid cascade buffer footprint:\n");
	int grids[4] = { 128, 256, 512, 1024 };
	for (int i = 0; i < 4; i++)
		printf("[hrc]   grid %4d: %7.1f MB\n", grids[i], hrc_buffer_bytes(grids[i]) / (1024.0 * 1024.0));
	fflush(stdout);
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
	hrc.mip_march = 1; // minmax-mip cell skipping on by default
	hrc.max_levels = -1; // full cascade by default
	// Mip+max: mipmap-averaged emissivity (smooth lights) + max-absorption
	// (conservative occlusion). Pure mipmap averaging melts thin opaque walls:
	// a 2px absorption-4 wall box-filtered at 2x becomes absorption 2 and leaks
	// ~13% per crossing (obvious in the pinhole test).
	hrc.prefilter_mode = 4;
	hrc.blend_width = 4.0f;

	// Scene input canvases (nearest-neighbor: discrete pixel grid, no sRGB-space blending).
	// Mipmaps allocated for hardware mipmap prefilter mode.
	hrc.emissivity = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, true);
	hrc.absorption = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, true);

	// Per-grid cascade SSBOs (T levels, R ping-pong + zero, frustum outputs).
	// Allocated for the ACTIVE grid only and reallocated on [H] grid change,
	// instead of the old max-world-size allocation (~400MB with dense directions).
	hrc_alloc_buffers();

	// Final output canvas (linear filtering for smooth display).
	hrc.fluence = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR, false);

	// Min/max absorption canvas (grid-res, allocated at world size to handle runtime grid changes).
	hrc.minmax = hrc_make_canvas(HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_PIXEL_FORMAT_R16G16_FLOAT, CF_FILTER_NEAREST, false);

	// Coarse minmax mip for accelerated marching: (absMin, absMax, emisMax) per
	// HRC_MIP_BLOCK-sized world block, at world/block resolution.
	hrc.minmax_mip = hrc_make_canvas(HRC_WORLD_SIZE / HRC_MIP_BLOCK, HRC_WORLD_SIZE / HRC_MIP_BLOCK, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST, false);

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
	hrc.cs_minmax_mip = load_compute_shader("/hrc_data/hrc_minmax_mip.c_shd");
	hrc.cs_prefilter = load_compute_shader("/hrc_data/hrc_prefilter.c_shd");
	hrc.cs_prefilter_gauss = load_compute_shader("/hrc_data/hrc_prefilter_gauss.c_shd");
	hrc.cs_feedback = load_compute_shader("/hrc_data/hrc_feedback.c_shd");
}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	hrc_free_buffers();
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_canvas(hrc.minmax);
	cf_destroy_canvas(hrc.minmax_mip);
	cf_destroy_compute_shader(hrc.cs_minmax_mip);
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
// Cascade compute pipeline.

void hrc_compute()
{
	CF_Texture emiss_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture absrp_tex = cf_canvas_get_target(hrc.absorption);
	CF_Texture fluence_tex = cf_canvas_get_target(hrc.fluence);
	CF_Texture minmax_tex = cf_canvas_get_target(hrc.minmax);
	CF_Texture minmax_mip_tex = cf_canvas_get_target(hrc.minmax_mip);

	int dim = hrc.grid;
	int N = hrc.n;

	// Cascade level cap: stop early and treat R at the cap as the R_N=0 boundary.
	// Only T_0..T_cap are built and only R_{cap-1}..R_0 are merged. This drops the
	// longest-range (lowest-frequency) light in exchange for fewer extend/merge passes.
	int cap = N;
	if (hrc.max_levels >= 1 && hrc.max_levels < N) cap = hrc.max_levels;

	// Build the coarse minmax mip (absMin, absMax, emisMax per block) from the
	// final scene textures. The trace DDA and c-1 march sample it to skip
	// uniform blocks. Built after feedback so it reflects this frame's emission.
	if (hrc.mip_march) {
		int world = HRC_WORLD_SIZE;
		int block = HRC_MIP_BLOCK;
		int blocks = world / block;
		cf_material_set_texture_cs(hrc.mat_minmax, "u_absorption", absrp_tex);
		cf_material_set_texture_cs(hrc.mat_minmax, "u_emissivity", emiss_tex);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_minmax, "u_block", &block, CF_UNIFORM_TYPE_INT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(
			hrc_div_ceil(blocks, HRC_WG), hrc_div_ceil(blocks, HRC_WG), 1);
		CF_Texture rw_tex[1] = { minmax_mip_tex };
		d.rw_textures = rw_tex;
		d.rw_texture_count = 1;
		cf_dispatch_compute(hrc.cs_minmax_mip, hrc.mat_minmax, d);
	}

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
	int mip_block = HRC_MIP_BLOCK;
	cf_material_set_texture_cs(hrc.mat_trace, "u_minmax_mip", minmax_mip_tex);
	cf_material_set_uniform_cs(hrc.mat_trace, "u_use_mip", &hrc.mip_march, CF_UNIFORM_TYPE_INT, 1);
	cf_material_set_uniform_cs(hrc.mat_trace, "u_mip_block", &mip_block, CF_UNIFORM_TYPE_INT, 1);

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

		// Extend remaining levels (up to the cap).
		int ext_start = hrc.trace_levels > 0 ? hrc.trace_levels : 1;
		for (int i = ext_start; i <= cap; i++) {
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

		// Merge R_{cap-1} down to R_0. R rows hold 2 * dim entries (dense directions).
		// R at the cap is the zero boundary (r_zero), same role R_N=0 plays at full depth.
		int r_ping = 0;
		for (int i = cap - 1; i >= 0; i--) {
			int params[6] = { i, dim, hrc.vrays_w[i], hrc.vrays_w[i + 1], dim * 2, dim * 2 };
			cf_material_set_uniform_cs(hrc.mat_merge, "u_cascade", params + 0, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_world_size", params + 1, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_curr_w", params + 2, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_t_next_w", params + 3, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_prev_w", params + 4, CF_UNIFORM_TYPE_INT, 1);
			cf_material_set_uniform_cs(hrc.mat_merge, "u_r_curr_w", params + 5, CF_UNIFORM_TYPE_INT, 1);

			CF_ComputeDispatch d = cf_compute_dispatch_defaults(
				hrc_div_ceil(dim * 2, HRC_WG),
				hrc_div_ceil(dim, HRC_WG),
				1
			);
			CF_StorageBuffer r_prev = (i == cap - 1) ? hrc.r_zero : hrc.r_rad[1 - r_ping];
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
		int mip_block = HRC_MIP_BLOCK;
		cf_material_set_texture_cs(hrc.mat_composite, "u_emissivity", emiss_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_absorption", absrp_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_minmax", minmax_tex);
		cf_material_set_texture_cs(hrc.mat_composite, "u_minmax_mip", minmax_mip_tex);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_world_size", &world, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_grid_size", &dim, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_debug_mode", &debug, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_cminus1", &hrc.cminus1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_upscale_mode", &hrc.upscale_mode, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_use_mip", &hrc.mip_march, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_mip_block", &mip_block, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(hrc.mat_composite, "u_c1_selective", &hrc.c1_selective, CF_UNIFORM_TYPE_INT, 1);

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
int freeze_time = 0; // HRC_FREEZE=1: hold time_acc fixed (freezes the pinhole gap sweep etc.)
int test_scene = 0; // 0 = off, 1 = static centered light, 2 = slowly orbiting light

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

void draw_test_shapes(int channel)
{
	if (test_scene == 3) draw_cornell(channel);
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
		test_scene = (test_scene + 1) % 5;
	}
	if (cf_key_just_pressed(CF_KEY_H)) {
		if      (hrc.grid == 128)  hrc_resize(256);
		else if (hrc.grid == 256)  hrc_resize(512);
		else if (hrc.grid == 512)  hrc_resize(1024);
		else                       hrc_resize(128);
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
	if (!freeze_time) time_acc += dt;
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
	cf_draw_canvas(display, cf_v2(0, 0), cf_v2(ws, ws));
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
		"[T] Test scene: %s",
		hrc.blend_boundary ? "on" : "off",
		hrc.blend_width,
		hrc.cminus1 ? "on" : "off",
		mode_names[hrc.debug_mode],
		hrc.grid, hrc.grid == HRC_WORLD_SIZE ? "1x" : hrc.grid == HRC_WORLD_SIZE/2 ? "2x" : hrc.grid == HRC_WORLD_SIZE/4 ? "4x" : "8x",
		hrc.upscale_mode == 0 ? "nearest" : hrc.upscale_mode == 1 ? "minmax" : "bilateral",
		hrc.prefilter_mode == 0 ? "off" : hrc.prefilter_mode == 1 ? "box" : hrc.prefilter_mode == 2 ? "mipmap" : hrc.prefilter_mode == 3 ? "gauss" : "mip+max",
		hrc.trace_levels, hrc.n + 1,
		test_scene == 0 ? "off" : test_scene == 1 ? "static 8x8" : test_scene == 2 ? "orbiting 8x8" : test_scene == 3 ? "cornell" : "pinhole"
	);

	float half = (float)HRC_WORLD_SIZE * 0.5f;
	cf_draw_push_color(cf_color_white());
	cf_push_font_size(16.0f);
	cf_draw_text(buf, cf_v2(-half + 10.0f, half - 20.0f), -1);
	cf_pop_font_size();
	cf_draw_pop_color();
}

//--------------------------------------------------------------------------------------------------
// Benchmark mode (HRC_BENCH=1).
//
// Iterates a fixed config sweep over deterministic scenes. For each config the
// sample renders warmup frames (long enough for cornell's multibounce feedback
// to settle), then times BENCH_TIMED frames with the wall clock, reads back the
// fluence canvas, and prints one CSV row with average frame ms plus quality vs
// the in-run 1x-grid reference for the same scene (RMSE on tonemapped luma +
// max channel diff, both on the displayed rgba8 output). Exits when the sweep
// completes. Run a Release build for meaningful timings.

typedef struct BenchConfig
{
	int scene;        // HRC_TEST scene id (1 static, 3 cornell, 4 pinhole)
	int grid;
	int trace_levels;
	int upscale_mode;
	int cminus1;
	int mip_march;    // minmax-mip cell skipping
	int max_levels;   // cascade level cap (-1 = full)
	int c1_selective; // c-1 selectivity (item 4)
	int is_reference; // first row per scene; its readback becomes the quality baseline
	const char* extras;
} BenchConfig;

#define BENCH_MAX_CONFIGS 128
#define BENCH_WARMUP 30           // pipeline compile + steady state
#define BENCH_WARMUP_FEEDBACK 120 // cornell: multibounce feedback settles ~60 frames; use 2x margin
#define BENCH_TIMED 60

typedef struct Bench
{
	int active;
	int index;
	int count;
	BenchConfig configs[BENCH_MAX_CONFIGS];
	int state;       // 0 = apply config, 1 = warmup, 2 = timed, 3 = wait on readback
	int frames_left;
	uint64_t t0;
	double frame_ms;
	CF_Readback readback;
	uint8_t* ref[5];    // per-scene reference pixels, indexed by scene id
	uint8_t* ab_ref[5]; // per-scene "nomip" image, for the mip-neutrality A/B diff
} Bench;

Bench bench;

BenchConfig* bench_add(int scene, int grid, int upscale, int cm1, int is_ref, const char* extras)
{
	CF_ASSERT(bench.count < BENCH_MAX_CONFIGS);
	BenchConfig* c = &bench.configs[bench.count++];
	c->scene = scene;
	c->grid = grid;
	c->trace_levels = 3;
	c->upscale_mode = upscale;
	c->cminus1 = cm1;
	c->mip_march = 1;      // mip on by default
	c->max_levels = -1;    // full cascade by default
	c->c1_selective = 0;   // off by default
	c->is_reference = is_ref;
	c->extras = extras;
	return c;
}

void bench_init()
{
	bench.active = 1;
	bench.state = 0;
	freeze_time = 1;
	time_acc = 1.0f; // fixed pinhole gap position
	cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE); // uncap present so GPU time is measurable

	int scenes[3] = { 1, 3, 4 };
	for (int s = 0; s < 3; s++) {
		int scene = scenes[s];
		// 1x-grid reference row (quality baseline for this scene).
		bench_add(scene, HRC_WORLD_SIZE, 1, 1, 1, "reference");
		int grids[2] = { 512, 256 };
		for (int g = 0; g < 2; g++) {
			bench_add(scene, grids[g], 0, 1, 0, "");
			bench_add(scene, grids[g], 1, 1, 0, "");
			bench_add(scene, grids[g], 2, 1, 0, "");
			bench_add(scene, grids[g], 1, 0, 0, "");
		}
		// Minmax-mip marching A/B at grid 512 (item 3 correctness/perf gate).
		// nomip runs first and stashes its image; the mip row's rmse/maxdiff are
		// measured DIRECTLY against that nomip image (neutrality), not the reference.
		bench_add(scene, 512, 1, 1, 0, "nomip")->mip_march = 0;
		bench_add(scene, 512, 1, 1, 0, "mip")->mip_march = 1;
		// c-1 selectivity at grid 512 (item 4): skip the c-1 march in open cells.
		bench_add(scene, 512, 1, 1, 0, "c1sel")->c1_selective = 1;
		// Cascade level cap (item 5): stop N-1 / N-2 levels deep at grid 512 (N=9).
		{
			int N = POW2_LOG2(512);
			bench_add(scene, 512, 1, 1, 0, "cap_N-1")->max_levels = N - 1;
			bench_add(scene, 512, 1, 1, 0, "cap_N-2")->max_levels = N - 2;
		}
	}

	printf("scene,grid,prefilter,upscale,cm1,mip,cap,c1sel,extras,frame_ms,rmse,maxdiff\n");
	fflush(stdout);
}

// Clear last frame's fluence so cornell's feedback loop always converges from
// the same starting state regardless of which config ran before.
void bench_clear_feedback()
{
	begin_canvas_draw();
	cf_render_to(hrc.fluence_lin, true);
	end_canvas_draw();
}

void bench_apply(BenchConfig* c)
{
	test_scene = c->scene;
	if (c->grid != hrc.grid) hrc_resize(c->grid);
	hrc.trace_levels = c->trace_levels;
	hrc.upscale_mode = c->upscale_mode;
	hrc.cminus1 = c->cminus1;
	hrc.mip_march = c->mip_march;
	hrc.c1_selective = c->c1_selective;
	hrc.max_levels = c->max_levels;
	bench_clear_feedback();
}

void bench_metrics(const uint8_t* test, const uint8_t* ref, double* rmse, double* maxdiff)
{
	int n = HRC_WORLD_SIZE * HRC_WORLD_SIZE;
	double sum_sq = 0.0;
	int md = 0;
	for (int i = 0; i < n; i++) {
		const uint8_t* a = test + i * 4;
		const uint8_t* b = ref + i * 4;
		// Tonemapped luma on the displayed sRGB bytes.
		double la = (0.2126 * a[0] + 0.7152 * a[1] + 0.0722 * a[2]) / 255.0;
		double lb = (0.2126 * b[0] + 0.7152 * b[1] + 0.0722 * b[2]) / 255.0;
		double d = la - lb;
		sum_sq += d * d;
		for (int ch = 0; ch < 3; ch++) {
			int cd = abs((int)a[ch] - (int)b[ch]);
			if (cd > md) md = cd;
		}
	}
	*rmse = sqrt(sum_sq / (double)n);
	*maxdiff = (double)md / 255.0;
}

const char* bench_scene_name(int scene)
{
	return scene == 1 ? "static" : scene == 2 ? "orbiting" : scene == 3 ? "cornell" : "pinhole";
}

// Called after cf_app_update, before the frame's draws (config must be in
// place before the scene canvases render).
void bench_pre_frame()
{
	if (bench.state == 0) {
		bench_apply(&bench.configs[bench.index]);
		BenchConfig* c = &bench.configs[bench.index];
		bench.frames_left = (c->scene == 3) ? BENCH_WARMUP_FEEDBACK : BENCH_WARMUP;
		bench.state = 1;
	}
}

// Called after hrc_compute (the frame's GPU work has been submitted).
void bench_post_frame()
{
	BenchConfig* c = &bench.configs[bench.index];
	switch (bench.state) {
	case 1: // warmup
		if (--bench.frames_left <= 0) {
			bench.frames_left = BENCH_TIMED;
			bench.t0 = cf_get_ticks();
			bench.state = 2;
		}
		break;
	case 2: // timed
		if (--bench.frames_left <= 0) {
			uint64_t t1 = cf_get_ticks();
			bench.frame_ms = (double)(t1 - bench.t0) / (double)cf_get_tick_frequency() * 1000.0 / (double)BENCH_TIMED;
			bench.readback = cf_canvas_readback(hrc.fluence);
			bench.state = 3;
		}
		break;
	case 3: // wait on async readback (scene keeps rendering; it's static)
		if (cf_readback_ready(bench.readback)) {
			int size = cf_readback_size(bench.readback);
			CF_ASSERT(size == HRC_WORLD_SIZE * HRC_WORLD_SIZE * 4);
			uint8_t* pixels = (uint8_t*)cf_alloc(size);
			cf_readback_data(bench.readback, pixels, size);
			cf_destroy_readback(bench.readback);

			double rmse = 0.0, maxdiff = 0.0;
			if (c->is_reference) {
				cf_free(bench.ref[c->scene]);
				bench.ref[c->scene] = pixels; // take ownership
			} else if (strcmp(c->extras, "nomip") == 0) {
				// Measure vs reference AND stash the image for the mip A/B.
				bench_metrics(pixels, bench.ref[c->scene], &rmse, &maxdiff);
				cf_free(bench.ab_ref[c->scene]);
				bench.ab_ref[c->scene] = pixels; // take ownership
			} else if (strcmp(c->extras, "mip") == 0) {
				// Direct neutrality diff against the nomip image (not the reference).
				CF_ASSERT(bench.ab_ref[c->scene]);
				bench_metrics(pixels, bench.ab_ref[c->scene], &rmse, &maxdiff);
				cf_free(pixels);
			} else {
				CF_ASSERT(bench.ref[c->scene]);
				bench_metrics(pixels, bench.ref[c->scene], &rmse, &maxdiff);
				cf_free(pixels);
			}

			printf("%s,%d,%d,%d,%d,%d,%d,%d,%s,%.3f,%.5f,%.4f\n",
				bench_scene_name(c->scene), c->grid, hrc.prefilter_mode, c->upscale_mode, c->cminus1,
				c->mip_march, c->max_levels, c->c1_selective, c->extras, bench.frame_ms, rmse, maxdiff);
			fflush(stdout);

			bench.index++;
			bench.state = 0;
			if (bench.index >= bench.count) {
				printf("BENCH DONE\n");
				fflush(stdout);
				for (int i = 0; i < 5; i++) { cf_free(bench.ref[i]); cf_free(bench.ab_ref[i]); }
				bench.active = 0;
				cf_app_signal_shutdown();
			}
		}
		break;
	}
}

//--------------------------------------------------------------------------------------------------
// Entry point.

int main(int argc, char* argv[])
{
	cf_make_app("HRC - Holographic Radiance Cascades", 0, 0, 0, HRC_WORLD_SIZE, HRC_WORLD_SIZE, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);

	hrc_init();
	hrc_print_memory_table();
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
		if ((env = getenv("HRC_FREEZE")))      freeze_time = atoi(env);
		if ((env = getenv("HRC_MIP")))         hrc.mip_march = atoi(env);
		if ((env = getenv("HRC_CM1SEL")))      hrc.c1_selective = atoi(env);
		if ((env = getenv("HRC_MAX_LEVELS")))  hrc.max_levels = atoi(env);
	}

	if (getenv("HRC_BENCH")) bench_init();

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_draw_push_shape_aa(0);
		if (bench.active) bench_pre_frame();
		handle_input();
		update_lights();
		draw_emissivity();
		draw_absorption();
		draw_diffuse();
		if (test_scene == 3) hrc_feedback();
		hrc_compute();
		display_fluence();
		if (!bench.active) draw_hud();
		cf_app_draw_onto_screen(true);
		if (bench.active) bench_post_frame();
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
