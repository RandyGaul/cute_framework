// HRC 2D global illumination -- a compute-driven radiance-cascade GI renderer,
// validated bit-exact against amitabha (Sannikov's reference implementation).
//
// The scene is authored as canvases holding physical units: linear EMISSION, an
// ABSORPTION coefficient, and a DIFFUSE albedo, per world pixel. Absorption is
// continuous, so a surface is just a cell with huge absorption and fog is a cell
// with a little -- the same code path gives hard shadows and volumetric
// transmission. Albedo feeds a multibounce pass so a surface reflects the light
// landing on it instead of reading as a black cutout.
//
// Pipeline: direct-trace the low cascade levels, merge_up the rest, merge down,
// then scatter-reconstruct per pixel. Output is directional (2D Fourier L1), so
// surfaces can be shaded from a normal rather than just a scalar fluence.
//
// Scenes, cycled with the LEFT/RIGHT arrow keys:
//   cornell   diffuse bounce and soft shadows in a closed box
//   glass     a translucent volume beside a solid occluder
//   pinhole   five coloured sources beaming through one gap
//   rectroom  a small emitter in a sealed room
//   circ      a single emitter in open space
//   showcase  coloured walls, haze, and a glowing volume
//   maze      an interactive bounce-light playground: a generated maze whose
//             walls catch and re-emit light. SPACE toggles a probe light that
//             rides the cursor, left click drops coloured lights, right click
//             removes the nearest one -- watch the bounce crawl around corners
//   text      type as the only geometry: emissive headlines light a hazy void
//             while a non-emissive line silhouettes and casts shadows
//
// Env vars:
//   HRC_SCENE=name      scene to start on (default cornell)
//   HRC_RES=WxH         window / world resolution (default 1024x1024)
//   HRC_SCALE=1|2|4     run the probe lattice at 1/N res and upscale
//   HRC_EXPOSURE=f      display exposure bias
//   HRC_SAT=f           post-tonemap chroma (1.0 = untouched)
//   HRC_BLUR=0|1        the paper's 1px cross blur over the fluence (default 1);
//                       C toggles it at runtime
//   HRC_SHOT=1          headless: render the starting scene, dump it, exit
//   HRC_PERF=N          headless: time N frames (vsync off), print ms/frame, exit
//   HRC_ZOOM_T=u        bloom only: pin the zoom phase to u in [0,1]
//   HRC_TRACE_LEVELS=n  direct-trace levels before extend takes over (default 3)
//   HRC_MAX_LEVELS=n    cap cascade depth (0 = full)
//   HRC_SKIP=tembfd     skip passes for cost attribution (timing only)
//
// The text scene exposes its own tuning knobs: HRC_TXT_E (emission), _OABS/_EABS
// (absorption), _OALB (albedo), _BIG/_SML (type size), _TRACK (letter-spacing),
// _FOG (haze), and HRC_OVL_E/_O/_TONE/_WHITE for the composited stamp.

#include <cute.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//--------------------------------------------------------------------------------------------------
// Configuration.

#define GI_REF      1024      // default world resolution (HRC_RES overrides)
#define GI_N_MAX    11        // array sizing: log2_ceil of the largest grid axis
#define GI_TRACE_LEVELS 3     // direct-trace levels 0..2, extend the rest
#define GI_ABS_THRESHOLD 0.1f

// Runtime world dimensions (set in main() from the window size).
int world_w = GI_REF, world_h = GI_REF;   // scene canvas + cascade resolution (view + 2*pad)
int view_w = GI_REF, view_h = GI_REF;     // the window: centre crop of the world
int g_pad = 0;                            // HRC_PAD: off-screen light ring, px per side




//--------------------------------------------------------------------------------------------------
// Shader loading.

CF_ComputeShader load_compute_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	if (!src) {
		printf("FATAL: failed to read %s\n", path);
		exit(1);
	}
	CF_ComputeShader cs = cf_make_compute_shader_from_source(src);
	const char* err = cf_shader_compile_error();
	if (err && err[0]) { printf("SHADER COMPILE ERROR in %s:\n%s\n", path, err); fflush(stdout); }
	cf_free(src);
	return cs;
}

CF_Shader load_draw_shader(const char* path)
{
	char* src = cf_fs_read_entire_file_to_memory_and_nul_terminate(path, NULL);
	if (!src) {
		printf("FATAL: failed to read %s\n", path);
		exit(1);
	}
	CF_Shader shd = cf_make_draw_shader_from_source(src);
	cf_free(src);
	return shd;
}

//--------------------------------------------------------------------------------------------------
// HRC state.

typedef struct Hrc
{
	CF_Canvas emissivity;
	CF_Canvas absorption;
	CF_Canvas diffuse;      // per-pixel albedo for multibounce feedback
	CF_Canvas fluence;      // tonemapped display (rgba8)
	CF_Canvas fluence_lin;  // linear fluence (feedback input)
} Hrc;

Hrc hrc;

CF_StorageBuffer hrc_make_buf(int w, int h)
{
	CF_StorageBufferParams p = cf_storage_buffer_defaults(w * h * 8);
	p.compute_readable = true;
	p.compute_writable = true;
	return cf_make_storage_buffer(p);
}

CF_Canvas hrc_make_canvas(int w, int h, CF_PixelFormat fmt, CF_Filter filter)
{
	CF_CanvasParams p = cf_canvas_defaults(w, h);
	p.target.pixel_format = fmt;
	p.target.filter = filter;
	p.target.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COLOR_TARGET_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_READ_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	p.target.wrap_u = CF_WRAP_MODE_CLAMP_TO_EDGE;
	p.target.wrap_v = CF_WRAP_MODE_CLAMP_TO_EDGE;
	return cf_make_canvas(p);
}

int hrc_div_ceil(int a, int b)
{
	return (a + b - 1) / b;
}

int hrc_log2_ceil(int x)
{
	int n = 0;
	while ((1 << n) < x) n++;
	return n;
}



void hrc_init()
{
	CF_MEMSET(&hrc, 0, sizeof(hrc));



	hrc.emissivity = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.absorption = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.diffuse = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	hrc.fluence = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R8G8B8A8_UNORM, CF_FILTER_LINEAR);
	hrc.fluence_lin = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);

}

void hrc_shutdown()
{
	cf_destroy_canvas(hrc.emissivity);
	cf_destroy_canvas(hrc.absorption);
	cf_destroy_canvas(hrc.diffuse);
	cf_destroy_canvas(hrc.fluence);
	cf_destroy_canvas(hrc.fluence_lin);
}

//--------------------------------------------------------------------------------------------------
// Runtime toggles.
int bounce_on = 1;  // multibounce feedback pass
int blur_on = 1;    // paper's 1px cross blur over the fluence (checkerboard fix)

// Perf knobs, all env-driven so a timing matrix needs no rebuilds.
//   HRC_TRACE_LEVELS=n  direct-trace levels 0..n-1, merge_up (extend) above
//                       (default GI_TRACE_LEVELS)
//   HRC_MAX_LEVELS=n    stop the cascade after n levels (0 = full depth). Far
//                       light beyond the capped interval length is lost, which
//                       an enclosed room never notices.
//   HRC_SKIP=letters    skip passes wholesale, for pass-cost attribution:
//                       t trace, e extend, m merge, f finish, d display, b bounce.
//                       The image is garbage with anything skipped; timing-only.
int g_trace_levels = GI_TRACE_LEVELS;
int g_max_levels = 0;
const char* g_skip = "";
int skip_pass(char c) { return strchr(g_skip, c) != NULL; }

// Absorption units-per-pixel factor fed to the marching shaders. Fixed scenes
// author per-pixel coefficients and leave this at 1 (bit-exact). The bloom
// zoom scene authors per DESIGN UNIT and sets this to (design units per world
// pixel) every frame, so optical depth is invariant under camera zoom -- a wall
// that seals a room close up still seals it from across the valley.
float g_abs_zoom = 1.0f;

// Wall-clock for animated scenes. Advanced by the interactive loop only, so a
// headless capture always renders the same frame and stays reproducible.
float g_scene_time = 0.0f;


//--------------------------------------------------------------------------------------------------
// Drawing helpers.

// Premultiplied alpha blending onto an f16 canvas: alpha-1 draws replace what's below.
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


//--------------------------------------------------------------------------------------------------
// f16 helpers.

float half_to_float(uint16_t h)
{
	uint32_t sign = (uint32_t)(h >> 15) & 1;
	uint32_t exp = (uint32_t)(h >> 10) & 0x1f;
	uint32_t man = (uint32_t)h & 0x3ff;
	if (exp == 0) return (sign ? -1.0f : 1.0f) * (float)man * (1.0f / 16777216.0f);
	if (exp == 31) return sign ? -1e30f : 1e30f;
	union { uint32_t u; float f; } v;
	v.u = (sign << 31) | ((exp + 112) << 23) | (man << 13);
	return v.f;
}


//--------------------------------------------------------------------------------------------------
// Entry point.

static void ap_hsv(float h, float s, float v, float* r, float* g, float* b)
{
	float c = v * s, x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f)), m = v - c;
	float rr = 0, gg = 0, bb = 0;
	if (h < 60)      { rr = c; gg = x; }
	else if (h < 120){ rr = x; gg = c; }
	else if (h < 180){ gg = c; bb = x; }
	else if (h < 240){ gg = x; bb = c; }
	else if (h < 300){ rr = x; bb = c; }
	else             { rr = c; bb = x; }
	*r = rr + m; *g = gg + m; *b = bb + m;
}




//--------------------------------------------------------------------------------------------------
// Non-square exact HRC (unified 2-group model): horizontal group (rot 0,2;
// cascade=width) + vertical group (rot 1,3; cascade=height), each 4 segments.
// Handles square and non-square uniformly. hrc_trace/merge/finish + hrc_display.
static struct {
	int inited, maxd, nlev;
	// Tbuf: fused rad+trn per ray (one uvec4 = 6 packed halves), so every trace/
	// extend/merge access is a single 16-byte transaction instead of two 8-byte
	// ones against split rad/trn buffers. Measured perf-neutral on an RTX-class
	// GPU (the L2 was absorbing the split traffic), kept because half the buffers
	// and bindings is simply less to hold: output verified bit-exact either way.
	CF_StorageBuffer Tbuf[12], Rbuf[2], Rzero, R0h, R0v;
	CF_Canvas dir[3];   // directional radiance, 2D Fourier L1: a0, a1, b1
	CF_ComputeShader cs_trace, cs_extend, cs_merge, cs_finish, cs_display, cs_feedback;
	CF_Material m_trace, m_extend, m_merge, m_finish, m_display, m_feedback;
} gi;

const char* scene_name(void);

// Per-scene display exposure for the raw-AgX tonemap (hrc_scene_draw sets it).
float g_exposure = 1.0f;
float g_bounce_boost = 1.0f; // demo gain on the feedback loop (maze runs 2x)

// HRC_SCALE: run the cascade at 1/N resolution and upscale on display.
// The DDA still marches the FULL-RES scene, so occlusion stays sharp; only the
// probe lattice and radiance buffers shrink (N^2 fewer probes).
int g_hrc_scale = 1;
int hrc_w(void) { return world_w / g_hrc_scale; }
int hrc_h(void) { return world_h / g_hrc_scale; }

// Cascade extents for the CURRENT scale. Storage is allocated once for scale 1 --
// the largest configuration -- so changing scale only changes how much of it is
// used. Reallocating instead would mean destroying buffers the GPU may still be
// reading from, which is not worth it for a display toggle.
static void hrc_apply_scale(void)
{
	int gw = hrc_w() / 2, gh = hrc_h() / 2;
	gi.maxd = gw > gh ? gw : gh;
	gi.nlev = hrc_log2_ceil(gi.maxd) + 1;   // levels 0..nlev-1
}

static void hrc_init_once(void)
{
	if (gi.inited) { hrc_apply_scale(); return; }
	// Allocate at scale 1 regardless of the starting scale, so the keys never need
	// to resize anything. MAXD/NLEV here are capacities, not the working extents.
	int gw = world_w / 2, gh = world_h / 2;
	int MAXD = gw > gh ? gw : gh;
	int NLEV = hrc_log2_ceil(MAXD) + 1;
	for (int n = 0; n < NLEV; n++) {
		int sx = hrc_div_ceil(MAXD, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int elems = sx * (4 * MAXD) * nrays;
		gi.Tbuf[n] = hrc_make_buf(elems * 2, 1); // uvec4 per element (hrc_make_buf sizes in 8-byte units)
	}
	int maxR = 0;
	for (int n = 0; n < NLEV; n++) { int e = hrc_div_ceil(MAXD, 1 << n) * (4 * MAXD) * (2 << n); if (e > maxR) maxR = e; }
	gi.Rbuf[0] = hrc_make_buf(maxR, 1);
	gi.Rbuf[1] = hrc_make_buf(maxR, 1);
	gi.Rzero   = hrc_make_buf(maxR, 1);
	{ void* z = cf_calloc((size_t)maxR * 8, 1); cf_update_storage_buffer(gi.Rzero, z, (size_t)maxR * 8); cf_free(z); }
	int r0sz = MAXD * 2 * (4 * MAXD); // = 8*MAXD*MAXD
	gi.R0h = hrc_make_buf(r0sz, 1);
	gi.R0v = hrc_make_buf(r0sz, 1);
	for (int i = 0; i < 3; i++)
		gi.dir[i] = hrc_make_canvas(world_w, world_h, CF_PIXEL_FORMAT_R16G16B16A16_FLOAT, CF_FILTER_NEAREST);
	gi.cs_trace   = load_compute_shader("/hrc_gi_data/hrc_trace.c_shd");
	gi.cs_extend  = load_compute_shader("/hrc_gi_data/hrc_extend.c_shd");
	gi.cs_merge   = load_compute_shader("/hrc_gi_data/hrc_merge.c_shd");
	gi.cs_finish  = load_compute_shader("/hrc_gi_data/hrc_finish.c_shd");
	gi.cs_display = load_compute_shader("/hrc_gi_data/hrc_display.c_shd");
	gi.cs_feedback= load_compute_shader("/hrc_gi_data/hrc_feedback.c_shd");
	gi.m_trace = cf_make_material(); gi.m_extend = cf_make_material(); gi.m_merge = cf_make_material();
	gi.m_finish = cf_make_material(); gi.m_display = cf_make_material();
	gi.m_feedback = cf_make_material();
	gi.inited = 1;
	hrc_apply_scale();
}

// Run one rotation group (trace -> merge -> R0_group).
static void hrc_group(CF_Texture emis_tex, CF_Texture opac_tex,
                         int rot0, int rot1, int casc_grid, int cross_grid, CF_StorageBuffer R0out)
{
	int NLEV = hrc_log2_ceil(casc_grid) + 1;
	if (g_max_levels > 0 && NLEV > g_max_levels) NLEV = g_max_levels > 2 ? g_max_levels : 2;
	// Direct-trace only the lowest g_trace_levels; build the rest with merge_up
	// (hrc_extend), which composites two half-length intervals from the level
	// below instead of re-marching the scene. Direct tracing costs O(2^n) per probe
	// at level n, so the deep levels dominate -- the reference traces 2 by default.
	for (int n = 0; !skip_pass('t') && n < NLEV && n < g_trace_levels; n++) {
		int sx = hrc_div_ceil(casc_grid, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int P[10] = { n, hrc_w()/2, hrc_h()/2, hrc_w(), hrc_h(), rot0, rot1, g_hrc_scale, world_w, world_h };
		cf_material_set_uniform_cs(gi.m_trace, "u_level",   P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_grid_w",  P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_grid_h",  P + 2, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_world_w", P + 3, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_world_h", P + 4, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_rot0",    P + 5, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_rot1",    P + 6, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_scale",   P + 7, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_full_w",  P + 8, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_full_h",  P + 9, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_trace, "u_abs_zoom", &g_abs_zoom, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_texture_cs(gi.m_trace, "u_emission", emis_tex);
		cf_material_set_texture_cs(gi.m_trace, "u_opacity",  opac_tex);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(4 * cross_grid, 64), hrc_div_ceil(sx * nrays, 4), 1);
		CF_StorageBuffer rw[1] = { gi.Tbuf[n] }; d.rw_buffers = rw; d.rw_buffer_count = 1;
		cf_dispatch_compute(gi.cs_trace, gi.m_trace, d);
	}
	// Extend remaining levels from the level below (merge_up).
	for (int n = g_trace_levels; !skip_pass('e') && n < NLEV; n++) {
		int sx = hrc_div_ceil(casc_grid, 1 << n), dirs = 2 << n, nrays = dirs + 1;
		int P[3] = { n, casc_grid, cross_grid };
		cf_material_set_uniform_cs(gi.m_extend, "u_level",      P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_extend, "u_casc_grid",  P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_extend, "u_cross_grid", P + 2, CF_UNIFORM_TYPE_INT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(4 * cross_grid, 64), hrc_div_ceil(sx * nrays, 4), 1);
		CF_StorageBuffer ro[1] = { gi.Tbuf[n - 1] }; d.ro_buffers = ro; d.ro_buffer_count = 1;
		CF_StorageBuffer rw[1] = { gi.Tbuf[n] }; d.rw_buffers = rw; d.rw_buffer_count = 1;
		cf_dispatch_compute(gi.cs_extend, gi.m_extend, d);
	}
	// merge down (NLEV-2)..0, final level 0 -> R0out
	int r_out = 0;
	for (int i = NLEV - 2; !skip_pass('m') && i >= 0; i--) {
		int sx = hrc_div_ceil(casc_grid, 1 << i), dirs = 2 << i;
		int P[3] = { i, casc_grid, cross_grid };
		cf_material_set_uniform_cs(gi.m_merge, "u_level",     P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_merge, "u_casc_grid", P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_merge, "u_cross_grid",P + 2, CF_UNIFORM_TYPE_INT, 1);
		CF_StorageBuffer Rprev = (i == NLEV - 2) ? gi.Rzero : gi.Rbuf[1 - r_out];
		CF_StorageBuffer Rdst  = (i == 0) ? R0out : gi.Rbuf[r_out];
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(4 * cross_grid, 64), hrc_div_ceil(dirs, 2), hrc_div_ceil(sx, 2));
		CF_StorageBuffer ro[3] = { gi.Tbuf[i], gi.Tbuf[i + 1], Rprev };
		d.ro_buffers = ro; d.ro_buffer_count = 3;
		CF_StorageBuffer rw[1] = { Rdst }; d.rw_buffers = rw; d.rw_buffer_count = 1;
		cf_dispatch_compute(gi.cs_merge, gi.m_merge, d);
		if (i > 0) r_out ^= 1;
	}
}

void hrc_compute(void)
{
	hrc_init_once();
	CF_Texture emis_tex = cf_canvas_get_target(hrc.emissivity);
	CF_Texture opac_tex = cf_canvas_get_target(hrc.absorption);

	// Multibounce: fold last frame's fluence, tinted by albedo, back into emission
	// before tracing. Skipping it leaves absorbing surfaces unlit, which is what the
	// B key demonstrates. This is what lights the visible FACE of an opaque surface --
	// the cascade only reports light ARRIVING at a cell, so a pure absorber has
	// nothing to show and reads as a black cutout. Runs after the scene draw (which
	// rewrote emissivity) and before the cascade, so each frame adds one bounce.
	if (bounce_on && !skip_pass('b')) {
		cf_material_set_uniform_cs(gi.m_feedback, "u_world_w", &world_w, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_feedback, "u_world_h", &world_h, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_feedback, "u_boost", &g_bounce_boost, CF_UNIFORM_TYPE_FLOAT, 1);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_w, 16), hrc_div_ceil(world_h, 16), 1);
		CF_Texture ro[2] = { cf_canvas_get_target(hrc.fluence_lin), cf_canvas_get_target(hrc.diffuse) };
		d.ro_textures = ro; d.ro_texture_count = 2;
		CF_Texture rw[1] = { emis_tex };
		d.rw_textures = rw; d.rw_texture_count = 1;
		cf_dispatch_compute(gi.cs_feedback, gi.m_feedback, d);
	}

	// horizontal group: rot 0,2  cascade=width;  vertical group: rot 1,3  cascade=height.
	int gw = hrc_w() / 2, gh = hrc_h() / 2;
	hrc_group(emis_tex, opac_tex, 0, 2, gw, gh, gi.R0h);
	hrc_group(emis_tex, opac_tex, 1, 3, gh, gw, gi.R0v);

	// finish: each rotation scatters into its OWN texture.  rot0->(R0h,local0)
	//         rot2->(R0h,local1)  rot1->(R0v,local0)  rot3->(R0v,local1).
	for (int i = 0; i < 3; i++) cf_render_to(gi.dir[i], true); // clear (scatter skips edges)
	int rots[4]      = { 0, 2, 1, 3 };
	int local[4]     = { 0, 1, 0, 1 };
	CF_StorageBuffer grp[4] = { gi.R0h, gi.R0h, gi.R0v, gi.R0v };
	for (int k = 0; !skip_pass('f') && k < 4; k++) {
		int rot = rots[k];
		int horiz = (rot == 0 || rot == 2);
		// Dispatch at FULL display res: the finish gathers per display pixel even
		// when the cascade runs reduced (display-res c-1, see hrc_finish).
		int world_c = horiz ? world_w : world_h;
		int world_x = horiz ? world_h : world_w;
		int P[9] = { gw, gh, hrc_w(), hrc_h(), rot, local[k], g_hrc_scale, world_w, world_h };
		cf_material_set_uniform_cs(gi.m_finish, "u_grid_w",   P + 0, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_grid_h",   P + 1, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_world_w",  P + 2, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_world_h",  P + 3, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_rot",      P + 4, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_local_rot",P + 5, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_scale",    P + 6, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_full_w",   P + 7, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_full_h",   P + 8, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_finish, "u_abs_zoom", &g_abs_zoom, CF_UNIFORM_TYPE_FLOAT, 1);
		cf_material_set_texture_cs(gi.m_finish, "u_emission", emis_tex);
		cf_material_set_texture_cs(gi.m_finish, "u_opacity",  opac_tex);
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_c, 8), hrc_div_ceil(world_x, 8), 1);
		CF_StorageBuffer ro[1] = { grp[k] }; d.ro_buffers = ro; d.ro_buffer_count = 1;
		CF_Texture rw[3] = { cf_canvas_get_target(gi.dir[0]), cf_canvas_get_target(gi.dir[1]),
		                     cf_canvas_get_target(gi.dir[2]) };
		d.rw_textures = rw; d.rw_texture_count = 3;
		cf_dispatch_compute(gi.cs_finish, gi.m_finish, d);
	}

	// display: AgX(radiance * exposure) -> fluence, linear -> fluence_lin.
	if (!skip_pass('d')) {
		const char* ev = getenv("HRC_EXPOSURE");
		float exposure = ev ? (float)atof(ev) : g_exposure;
		cf_material_set_texture_cs(gi.m_display, "u_a0", cf_canvas_get_target(gi.dir[0]));
		cf_material_set_texture_cs(gi.m_display, "u_a1", cf_canvas_get_target(gi.dir[1]));
		cf_material_set_texture_cs(gi.m_display, "u_b1", cf_canvas_get_target(gi.dir[2]));
		cf_material_set_texture_cs(gi.m_display, "u_emission", emis_tex);
		cf_material_set_texture_cs(gi.m_display, "u_absorption", opac_tex);
		cf_material_set_uniform_cs(gi.m_display, "u_blur", &blur_on, CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_display, "u_world_w",  &world_w,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_display, "u_world_h",  &world_h,  CF_UNIFORM_TYPE_INT, 1);
		cf_material_set_uniform_cs(gi.m_display, "u_exposure", &exposure, CF_UNIFORM_TYPE_FLOAT, 1);
		{ float abs_thresh = GI_ABS_THRESHOLD;
		  cf_material_set_uniform_cs(gi.m_display, "u_scale", &g_hrc_scale, CF_UNIFORM_TYPE_INT, 1);
		  cf_material_set_uniform_cs(gi.m_display, "u_abs_threshold", &abs_thresh, CF_UNIFORM_TYPE_FLOAT, 1); }
		{ const char* e = getenv("HRC_EMISSIVE");
		  float emissive = e ? (float)atof(e) : 1.0f;
		  cf_material_set_uniform_cs(gi.m_display, "u_emissive", &emissive, CF_UNIFORM_TYPE_FLOAT, 1);
		  // Only the type scene asks for extra chroma; everything else stays at 1.0.
		  const char* sv = getenv("HRC_SAT");
		  float sat = sv ? (float)atof(sv) : (!CF_STRCMP(scene_name(), "text") ? 1.20f : 1.0f);
		  cf_material_set_uniform_cs(gi.m_display, "u_saturation", &sat, CF_UNIFORM_TYPE_FLOAT, 1); }
		CF_ComputeDispatch d = cf_compute_dispatch_defaults(hrc_div_ceil(world_w, 16), hrc_div_ceil(world_h, 16), 1);
		CF_Texture rw[2] = { cf_canvas_get_target(hrc.fluence), cf_canvas_get_target(hrc.fluence_lin) };
		d.rw_textures = rw; d.rw_texture_count = 2;
		cf_dispatch_compute(gi.cs_display, gi.m_display, d);
	}
}

// Headless test scenes for the non-square exact HRC: rasterize a scene into
// emissivity/absorption, render via hrc_compute, capture -> cave_shot.raw, exit.
// HRC_SCENE = rectroom | cornell | pinhole  (use with HRC_RES).
// Rasterize the selected scene into emissivity/absorption. Shared by the
// interactive loop and the headless capture so both light the same thing.
// Every scene the sample can show, in cycle order. HRC_SCENE picks the starting
// one; the arrow keys move through them at runtime.
static const char* g_scene_list[] = {
	"cornell", "glass", "pinhole", "rectroom", "circ", "showcase", "orbit", "maze", "bloom", "text",
};
static const int g_scene_count = (int)(sizeof(g_scene_list) / sizeof(g_scene_list[0]));
static int g_scene_index = 0;

const char* scene_name(void)
{
	return g_scene_list[g_scene_index];
}

// Point the sample at a scene by name. Unknown names leave the selection alone so
// a typo in HRC_SCENE shows the default rather than crashing.
static void scene_select(const char* name)
{
	if (!name) return;
	for (int i = 0; i < g_scene_count; i++) {
		if (!CF_STRCMP(name, g_scene_list[i])) { g_scene_index = i; return; }
	}
	printf("unknown HRC_SCENE '%s' -- showing %s\n", name, scene_name());
	fflush(stdout);
}

static void scene_cycle(int delta)
{
	g_scene_index = (g_scene_index + delta + g_scene_count) % g_scene_count;
}

// The scene is drawn with the ordinary cf_draw API into two canvases:
// EMISSION (linear light emitted per pixel) and ABSORPTION (extinction
// coefficient per pixel). Every primitive -- boxes, circles, and vector TEXT --
// goes through the same path, so a glowing sign is just text drawn into the
// emission canvas and it lights the room like any other emitter.
//
// channel: 0 = emission, 1 = absorption, 2 = diffuse albedo (feeds the bounce
// pass), 3 = the translucent stamp composited over the finished image.
// Alpha of the translucent overlay stamp (channel 3), per line kind.
static float g_overlay_emis = 0.45f;
static float g_overlay_occ  = 0.72f;
static float g_overlay_tone  = 0.70f;  // value of the opaque stamp
static float g_overlay_white = 0.20f;  // how far the emissive stamp pulls to white

// One centred line of the "text" scene, authored into whichever channel is being
// drawn. Emissive lines carry emission plus just enough absorption to couple it;
// occluding lines carry no emission, absorption dense enough to seal the stroke,
// and an albedo so the bounce pass lights their visible face.
static void text_line(int channel, const char* str, float y, float size,
                      float er, float eg, float eb, float absorb, float albedo, float track)
{
	cf_push_font_size(size);
	CF_V2 sz = cf_text_size(str, -1);
	// Untracked lines fit by shrinking the type: a long headline scales down rather
	// than running off the frame. Tracked lines fit further down by closing up their
	// letter-spacing instead, which preserves the size.
	float fit_w = (float)view_w * 0.92f;
	if (track <= 0 && sz.x > fit_w) {
		cf_pop_font_size();
		cf_push_font_size(size * (fit_w / sz.x));
		sz = cf_text_size(str, -1);
	}
	CF_Color c;
	if (channel == 0)      c = cf_make_color_rgb_f(er, eg, eb);
	else if (channel == 1) c = cf_make_color_rgb_f(absorb, absorb, absorb * 1.06f);
	else if (channel == 2) c = cf_make_color_rgb_f(albedo, albedo, albedo * 1.03f);
	else {
		// Channel 3: the translucent stamp composited over the finished HRC image.
		// The volumetric pass below already produced the glow and the cast shadows;
		// this only puts a crisp, flat letterform back on top so the type reads.
		// Emissive lines stamp their own hue, occluding lines stamp dark.
		if (er + eg + eb > 0) {
			// A hot emitter's core reads near-white, with the hue carried by the glow
			// around it rather than the letterform itself. Stamping the raw normalized
			// emission colour oversaturates, so pull it most of the way to white.
			float m = er > eg ? (er > eb ? er : eb) : (eg > eb ? eg : eb);
			float k = g_overlay_white;
			c = cf_make_color_rgba_f(er / m * (1 - k) + k, eg / m * (1 - k) + k,
			                         eb / m * (1 - k) + k, g_overlay_emis);
		} else {
			// Neutral and faintly warm. This line is a lit surface, not an emitter, so
			// a hued stamp would read as though it were glowing; its contrast against
			// the wash comes from value instead of colour.
			float t = g_overlay_tone;
			c = cf_make_color_rgba_f(t * 1.04f, t * 1.00f, t * 0.97f, g_overlay_occ);
		}
	}
	cf_draw_push_color(c);
	float baseline = y + sz.y * 0.5f;
	if (track <= 0) {
		// No tracking: one call, so the font's own kerning applies.
		cf_draw_text(str, cf_v2(-sz.x * 0.5f, baseline), -1);
	} else {
		// Letter-spacing, which cf_draw has no API for: step glyph by glyph and add
		// the extra advance by hand. Measure the tracked width first so the line
		// still centres. ASCII only, which is all these captions are.
		float glyphs = 0;
		int n = 0;
		for (const char* p = str; *p; ++p) { char ch[2] = { *p, 0 }; glyphs += cf_text_width(ch, -1); n++; }
		// Auto-fit: tracking that would push the line past the frame gets pulled back,
		// so a longer string just tracks tighter instead of running off the edge.
		float maxw = (float)view_w * 0.92f;
		if (n > 1 && glyphs + track * (n - 1) > maxw) {
			track = (maxw - glyphs) / (float)(n - 1);
			if (track < 0) track = 0;
		}
		float total = glyphs + track * (n > 0 ? n - 1 : 0);
		float x = -total * 0.5f;
		for (const char* p = str; *p; ++p) {
			char ch[2] = { *p, 0 };
			cf_draw_text(ch, cf_v2(x, baseline), -1);
			x += cf_text_width(ch, -1) + track;
		}
	}
	cf_draw_pop_color();
	cf_pop_font_size();
}

//--------------------------------------------------------------------------------------------------
// Shared deterministic hash for procedural scene content.

static uint32_t arc_hash(uint32_t x)
{
	x ^= x >> 16; x *= 0x7feb352du;
	x ^= x >> 15; x *= 0x846ca68bu;
	x ^= x >> 16; return x;
}

//--------------------------------------------------------------------------------------------------
// "maze": an interactive bounce-light playground. A depth-first maze whose
// walls are finite-absorption diffuse surfaces, so every corridor face catches
// and re-emits whatever light reaches it. The feedback folds in one bounce per
// frame, which is the whole point of the scene: drop a light and watch the
// glow crawl down corridors and soak around corners frame by frame (B toggles
// the bounce off for contrast). SPACE toggles a probe light riding the cursor;
// left click drops a coloured light; right click removes the nearest one.

#define MZ_N 7         // cells per side
#define MZ_P 132.0f    // cell pitch in view units
#define MZ_T 14.0f     // wall thickness
#define MZ_MAX_LIGHTS 64

static uint8_t g_mz_right[MZ_N * MZ_N]; // wall between (c,r) and (c+1,r)
static uint8_t g_mz_top[MZ_N * MZ_N];   // wall between (c,r) and (c,r+1)
static int g_mz_inited = 0;
static struct { float x, y, hue; } g_mz_lights[MZ_MAX_LIGHTS];
static int g_mz_nlights = 0;
static int g_mz_probe_on = 0;
static float g_mz_mx = 0.0f, g_mz_my = 0.0f; // cursor in scene coords

static void maze_gen(void)
{
	uint8_t visited[MZ_N * MZ_N] = { 0 };
	int stack[MZ_N * MZ_N];
	int top = 0;
	uint32_t rng = 0x9E3779B9u;
	for (int i = 0; i < MZ_N * MZ_N; i++) { g_mz_right[i] = 1; g_mz_top[i] = 1; }
	stack[top++] = 0;
	visited[0] = 1;
	while (top > 0) {
		int cell = stack[top - 1];
		int c = cell % MZ_N, r = cell / MZ_N;
		// gather unvisited neighbours
		int opts[4], nopts = 0;
		if (c + 1 < MZ_N && !visited[cell + 1])      opts[nopts++] = 0; // right
		if (c - 1 >= 0   && !visited[cell - 1])      opts[nopts++] = 1; // left
		if (r + 1 < MZ_N && !visited[cell + MZ_N])   opts[nopts++] = 2; // up
		if (r - 1 >= 0   && !visited[cell - MZ_N])   opts[nopts++] = 3; // down
		if (!nopts) { top--; continue; }
		int d = opts[arc_hash(rng++) % nopts];
		int next = cell;
		if (d == 0)      { g_mz_right[cell] = 0;        next = cell + 1; }
		else if (d == 1) { g_mz_right[cell - 1] = 0;    next = cell - 1; }
		else if (d == 2) { g_mz_top[cell] = 0;          next = cell + MZ_N; }
		else             { g_mz_top[cell - MZ_N] = 0;   next = cell - MZ_N; }
		visited[next] = 1;
		stack[top++] = next;
	}
	// one starter light so the scene never opens pitch black (right click
	// removes it like any other). Spawn at the openest junction nearest the
	// centre: a dead-end pocket makes first-bounce light nearly invisible.
	{
		int best = MZ_N / 2 * MZ_N + MZ_N / 2, bopen = -1;
		float bdist = 1e30f;
		for (int r = 0; r < MZ_N; r++) {
			for (int c = 0; c < MZ_N; c++) {
				int cell = r * MZ_N + c;
				int open = 0;
				if (c + 1 < MZ_N && !g_mz_right[cell]) open++;
				if (c - 1 >= 0 && !g_mz_right[cell - 1]) open++;
				if (r + 1 < MZ_N && !g_mz_top[cell]) open++;
				if (r - 1 >= 0 && !g_mz_top[cell - MZ_N]) open++;
				float dx = c - (MZ_N - 1) * 0.5f, dy = r - (MZ_N - 1) * 0.5f;
				float d = dx * dx + dy * dy;
				if (open > bopen || (open == bopen && d < bdist)) { bopen = open; bdist = d; best = cell; }
			}
		}
		float o = -MZ_N * MZ_P * 0.5f;
		g_mz_lights[0].x = o + (best % MZ_N) * MZ_P + MZ_P * 0.5f;
		g_mz_lights[0].y = o + (best / MZ_N) * MZ_P + MZ_P * 0.5f;
		g_mz_lights[0].hue = 45.0f;
		g_mz_nlights = 1;
	}
	g_mz_inited = 1;
}

// Composite wall brush: a thin low-absorption SKIN wrapped around a dense
// core, drawn in two passes (all skins, then all cores) so segment joints
// never leave a low-absorption seam. One material cannot serve the bounce and
// stay light-tight: dense everywhere starves the feedback (a face cell sits in
// the shadow of its own absorption and holds no fluence to re-emit), thin
// everywhere turns the wall translucent under a hot light. The skin receives
// nearly unattenuated light and re-emits albedo's worth of it; the core seals.
#define MZ_SKIN 3.0f
static void mz_wall(int channel, int core_pass, float cx, float cy, float hw, float hh)
{
	if (!core_pass) {
		if (channel == 1)      cf_draw_push_color(cf_make_color_rgb_f(0.35f, 0.35f, 0.37f));
		else if (channel == 2) cf_draw_push_color(cf_make_color_rgb_f(0.85f, 0.83f, 0.79f));
		else return;
		cf_draw_quad_fill(cf_make_aabb(cf_v2(cx - hw, cy - hh), cf_v2(cx + hw, cy + hh)), 0);
		cf_draw_pop_color();
	} else {
		if (channel != 1 || hw <= MZ_SKIN || hh <= MZ_SKIN) return;
		cf_draw_push_color(cf_make_color_rgb_f(3.0f, 3.0f, 3.1f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(cx - hw + MZ_SKIN, cy - hh + MZ_SKIN), cf_v2(cx + hw - MZ_SKIN, cy + hh - MZ_SKIN)), 0);
		cf_draw_pop_color();
	}
}

static void mz_walls_all(int channel, int core_pass)
{
	float span = MZ_N * MZ_P;
	float o = -span * 0.5f;
	mz_wall(channel, core_pass, 0,  span * 0.5f, span * 0.5f + MZ_T * 0.5f, MZ_T * 0.5f);
	mz_wall(channel, core_pass, 0, -span * 0.5f, span * 0.5f + MZ_T * 0.5f, MZ_T * 0.5f);
	mz_wall(channel, core_pass,  span * 0.5f, 0, MZ_T * 0.5f, span * 0.5f + MZ_T * 0.5f);
	mz_wall(channel, core_pass, -span * 0.5f, 0, MZ_T * 0.5f, span * 0.5f + MZ_T * 0.5f);
	for (int r = 0; r < MZ_N; r++) {
		for (int c = 0; c < MZ_N; c++) {
			int cell = r * MZ_N + c;
			if (c + 1 < MZ_N && g_mz_right[cell])
				mz_wall(channel, core_pass, o + (c + 1) * MZ_P, o + r * MZ_P + MZ_P * 0.5f, MZ_T * 0.5f, MZ_P * 0.5f + MZ_T * 0.5f);
			if (r + 1 < MZ_N && g_mz_top[cell])
				mz_wall(channel, core_pass, o + c * MZ_P + MZ_P * 0.5f, o + (r + 1) * MZ_P, MZ_P * 0.5f + MZ_T * 0.5f, MZ_T * 0.5f);
		}
	}
}

static void maze_scene(int channel, float W, float H)
{
	if (!g_mz_inited) maze_gen();
	const float EMIT_ABS = 0.5f;

	// a whisper of haze so the corridors show beams, not just lit walls
	if (channel == 1) {
		cf_draw_push_color(cf_make_color_rgb_f(0.0007f, 0.0007f, 0.0008f));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(-W * 0.5f, -H * 0.5f), cf_v2(W * 0.5f, H * 0.5f)), 0);
		cf_draw_pop_color();
	}

	// walls: every skin first, every core second (see mz_wall)
	mz_walls_all(channel, 0);
	mz_walls_all(channel, 1);

	// lights: the cursor probe plus everything the user has dropped. Emitters
	// carry EMIT_ABS, never albedo (a light amplifying itself through the
	// bounce loop runs away).
	if (channel > 1) return;
	int emis = (channel == 0);
	// HOT lights on purpose: every bounce costs roughly an order of magnitude,
	// so seeing second-bounce light two corners away needs serious source flux.
	// The direct pool clips to white under AgX; the indirect crawl is the show.
	if (g_mz_probe_on) {
		cf_draw_push_color(emis ? cf_make_color_rgb_f(11.0f, 10.0f, 8.5f)
		                        : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
		cf_draw_circle_fill(cf_make_circle(cf_v2(g_mz_mx, g_mz_my), 15));
		cf_draw_pop_color();
	}
	for (int i = 0; i < g_mz_nlights; i++) {
		float r2, g2, b2; ap_hsv(g_mz_lights[i].hue, 0.7f, 1.0f, &r2, &g2, &b2);
		cf_draw_push_color(emis ? cf_make_color_rgb_f(9.0f * r2, 9.0f * g2, 9.0f * b2)
		                        : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
		cf_draw_circle_fill(cf_make_circle(cf_v2(g_mz_lights[i].x, g_mz_lights[i].y), 15));
		cf_draw_pop_color();
	}
}

static void scene_shapes(int channel, float W, float H)
{
	const char* name = scene_name();
	int cornell = !CF_STRCMP(name, "cornell");
	int pinhole = !CF_STRCMP(name, "pinhole");
	int circ    = !CF_STRCMP(name, "circ");
	int glass   = !CF_STRCMP(name, "glass");
	int rectroom= !CF_STRCMP(name, "rectroom");
	int orbit   = !CF_STRCMP(name, "orbit");
	if (!CF_STRCMP(name, "maze")) { if (channel < 3) maze_scene(channel, W, H); return; }
	int emis = (channel == 0);
	const float SOLID = 30000.0f;
	// Absorption for anything that EMITS. Never SOLID: at infinite absorption a light
	// occludes itself, so only the cells on its surface can radiate outward and the
	// emission picks up the shape of that rasterized boundary -- brighter along
	// whichever side the march reaches first, and spoked on small round sources. A
	// finite coefficient still reads as a solid bright object but lets the whole
	// volume emit, which is both symmetric and what a real lamp does.
	const float EMIT_ABS = 0.5f;

	// "text": type is very nearly the whole scene, after entropylost/amida's
	// "thisisradiancecascades". Two emissive headlines light a hazy room and one
	// non-emissive line silhouettes against that wash while casting long shadows.
	// The room is a plain bordered box whose walls carry a diffuse albedo, so the
	// neon washes back off them -- pink pooling at the top, mint below. Press B to
	// drop the bounce and the borders go black.
	if (!CF_STRCMP(name, "text")) {
		float EM  = getenv("HRC_TXT_E")   ? (float)atof(getenv("HRC_TXT_E"))   : 3.2f;
		float OA  = getenv("HRC_TXT_OABS")? (float)atof(getenv("HRC_TXT_OABS")): 0.25f;
		float OB  = getenv("HRC_TXT_OALB")? (float)atof(getenv("HRC_TXT_OALB")): 0.75f;
		float EA  = getenv("HRC_TXT_EABS")? (float)atof(getenv("HRC_TXT_EABS")): 0.20f;
		float BIG = getenv("HRC_TXT_BIG") ? (float)atof(getenv("HRC_TXT_BIG")) : 0.190f;
		float SML = getenv("HRC_TXT_SML") ? (float)atof(getenv("HRC_TXT_SML")) : 0.125f;
		float TRK = getenv("HRC_TXT_TRACK")?(float)atof(getenv("HRC_TXT_TRACK")):0.22f;
		// A little haze over the whole frame. Without a participating medium the
		// coloured light has nothing to scatter off and the field reads as washed-out
		// grey -- it is fluence through vacuum. Fog gives the light something to
		// stain, which is what makes the pink and mint actually fill the frame.
		// Drawn first: brushes overwrite, so any solid laid down before it would be
		// erased back to fog.
		if (channel == 1) {
			float fog = getenv("HRC_TXT_FOG") ? (float)atof(getenv("HRC_TXT_FOG")) : 0.005f;
			cf_draw_push_color(cf_make_color_rgb_f(fog, fog, fog * 1.15f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W * 0.5f, -H * 0.5f), cf_v2(W * 0.5f, H * 0.5f)), 0);
			cf_draw_pop_color();
		}
		// Borders. Solid to light transport, with an albedo so the bounce pass lights
		// their faces: without something to catch it the neon just falls off into an
		// empty void and there is no second bounce to see.
		{
			// Thin on purpose: the bounce pass lights a surface from a small
			// neighbourhood gather, so a thick wall only ever lights its inner few
			// pixels and the rest stays black. At this thickness the whole border
			// catches light and reads as a lit frame.
			float t = H * 0.009f;
			float a = getenv("HRC_TXT_WALL") ? (float)atof(getenv("HRC_TXT_WALL")) : 0.80f;
			// Finite absorption like every diffuse solid: the per-cell bounce
			// feeds on the fluence stored in the face cells, which an
			// effectively-infinite coefficient would zero out.
			CF_Color c = (channel == 1) ? cf_make_color_rgb_f(0.5f, 0.5f, 0.5f)
			                            : cf_make_color_rgb_f(a, a, a);
			if (channel == 1 || channel == 2) {
				cf_draw_push_color(c);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, H*0.5f - t), cf_v2(W*0.5f, H*0.5f)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(W*0.5f, -H*0.5f + t)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(-W*0.5f + t, H*0.5f)), 0);
				cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(W*0.5f - t, -H*0.5f), cf_v2(W*0.5f, H*0.5f)), 0);
				cf_draw_pop_color();
			}
		}
		// Zero emission, dense enough to seal the stroke, and a plain [0,1] albedo so
		// the bounce pass lights its face. Tracked out wide so it spans the frame.
		text_line(channel, "lookin cute ^_^", -H * 0.265f, H * SML, 0, 0, 0, OA, OB, H * SML * TRK);
		// Cute Framework's own palette, normalized to hue and scaled by emission:
		// pink #fea3a8 and mint #96dec6 off docs/stylesheets/cute.css and the logo.
		text_line(channel, "CUTE",       H * 0.150f, H * BIG, EM * 1.000f, EM * 0.520f, EM * 0.550f, EA, 0, 0);
		text_line(channel, "FRAMEWORK", -H * 0.055f, H * BIG, EM * 0.500f, EM * 1.000f, EM * 0.840f, EA, 0, 0);
		return;
	}

	// Channel 3 (the overlay stamp) is the type scene's alone.
	if (channel == 3) return;

	// Every solid surface carries a diffuse albedo so the bounce pass lights its
	// face -- without it a wall is a black cutout (the cascade only reports light
	// ARRIVING at a cell, and none arrives inside a pure absorber). Kept well
	// under 1: the bounce series converges to 1/(1-albedo) times the direct
	// light, so values near unity pile up over the settle frames and wash out.
	const float WALL_ALB = 0.70f;
	// Absorption for diffuse solids. Finite (the reference's walls use 0.5/px), NOT
	// SOLID: the per-cell bounce feedback re-emits the fluence stored AT a
	// surface cell, and at infinite absorption that is zero -- the face starves
	// and the object reads as a black cutout with a lit 1px rim. At 0.5 light
	// soaks the first few pixels of a face (visibly, like the reference's
	// Cornell walls) while anything thicker than ~15px stays fully opaque
	// (transmittance e^-0.5/px). Pinhole is the exception: its walls are thin
	// pure occluders whose job is a light-tight seal, so they stay SOLID.
	const float DIFF_ABS = 0.5f;
	float wall_abs = pinhole ? SOLID : DIFF_ABS;
	#define WALL(cx, cy, hw, hh) do { 		if (channel == 1) { cf_draw_push_color(cf_make_color_rgb_f(wall_abs, wall_abs, wall_abs)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } 		else if (channel == 2) { cf_draw_push_color(cf_make_color_rgb_f(WALL_ALB, WALL_ALB, WALL_ALB)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)
	#define LIGHT(cx, cy, hw, hh, r, g, b) do { 		if (channel > 1) break; 		cf_draw_push_color(emis ? cf_make_color_rgb_f(r, g, b) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS)); 		cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 		cf_draw_pop_color(); } while (0)
	#define FOG(cx, cy, hw, hh, d) do { 		if (channel == 1) { cf_draw_push_color(cf_make_color_rgb_f(d, d, (d) * 1.15f)); 			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2((cx)-(hw), (cy)-(hh)), cf_v2((cx)+(hw), (cy)+(hh))), 0); 			cf_draw_pop_color(); } } while (0)

	// Room shell shared by the enclosed scenes.
	if (cornell || glass || rectroom) {
		WALL(0, H*0.5f - 10, W*0.5f, 10);   // ceiling
		WALL(0, -H*0.5f + 10, W*0.5f, 10);  // floor
		WALL(-W*0.5f + 10, 0, 10, H*0.5f);  // left
		WALL(W*0.5f - 10, 0, 10, H*0.5f);   // right
	}

	if (circ) {
		// Emitters must not be SOLID. At infinite absorption only the disc's rasterized
		// rim can emit outward -- its own body blocks the rest -- and at this radius
		// that rim is a coarse pixel staircase whose extent differs along the axes and
		// the diagonals. The result is a +-45 degree modulation of the emitted light: a
		// hard X across the frame. A finite coefficient lets the whole volume emit, and
		// the source reads as the round, radially symmetric disc it is meant to be.
		// channel < 2: emitters carry no albedo -- a light re-emitting its own output
		// through the bounce loop would amplify itself.
		if (channel < 2) {
			cf_draw_push_color(emis ? cf_make_color_rgb_f(3, 3, 3) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
			cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), 8));
			cf_draw_pop_color();
		}
	} else if (rectroom) {
		// Finite absorption, same reason as circ: a SOLID emitter only radiates from
		// its staircased rim and throws +-45 degree spokes.
		if (channel < 2) {
			cf_draw_push_color(emis ? cf_make_color_rgb_f(2.5f, 2.5f, 2.5f) : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
			cf_draw_circle_fill(cf_make_circle(cf_v2(0, 0), 24));
			cf_draw_pop_color();
		}
	} else if (cornell) {
		LIGHT(0, H*0.5f - 26, W*0.09f, 5, 4.0f, 3.7f, 3.2f);
		WALL(-W*0.10f, -H*0.06f, W*0.028f, H*0.16f);
		WALL(W*0.12f, -H*0.14f, W*0.031f, H*0.10f);
	} else if (glass) {
		LIGHT(0, H*0.5f - 26, W*0.13f, 5, 4.0f, 3.7f, 3.2f);
		// see-through volume: finite absorption, so light is attenuated THROUGH it
		FOG(-W*0.12f, -H*0.06f, W*0.070f, H*0.20f, 0.017f);
		// solid circle occluder alongside: crisp umbra for contrast. Albedo on
		// channel 2 so its lit face shows -- NOT the SOLID coefficient, which would
		// be a runaway loop gain if it landed in the diffuse canvas.
		if (channel == 1) {
			cf_draw_push_color(cf_make_color_rgb_f(DIFF_ABS, DIFF_ABS, DIFF_ABS));
			cf_draw_circle_fill(cf_make_circle(cf_v2(W*0.14f, -H*0.06f), H*0.135f));
			cf_draw_pop_color();
		} else if (channel == 2) {
			cf_draw_push_color(cf_make_color_rgb_f(0.60f, 0.60f, 0.60f));
			cf_draw_circle_fill(cf_make_circle(cf_v2(W*0.14f, -H*0.06f), H*0.135f));
			cf_draw_pop_color();
		}
	} else if (pinhole) {
		float gap = H * 0.022f;
		WALL(0, H*0.25f + gap*0.5f, 3, H*0.25f - gap*0.5f);
		WALL(0, -H*0.25f - gap*0.5f, 3, H*0.25f - gap*0.5f);
		float cols[5][3] = { {4.0f,0.8f,0.3f},{0.3f,1.4f,3.2f},{0.5f,3.0f,0.4f},{2.6f,0.5f,3.4f},{3.2f,1.9f,0.3f} };
		for (int i = 0; i < 5; i++) {
			float y = (float)(i - 2) * (H * 0.14f);
			LIGHT(-W*0.47f + 30, y, 4, 16, cols[i][0], cols[i][1], cols[i][2]);
		}
	} else if (orbit) {
		// A small light on a circular path around a cluster of pillars, so the shadows
		// sweep as it travels. This is also the scene where the bounce pass earns its
		// keep: the walls carry a diffuse albedo, so their colour washes back into the
		// room. Press B to see the difference -- without it the walls are black and
		// only directly-lit surfaces show at all.
		WALL(0, H*0.5f - 12, W*0.5f, 12);
		WALL(0, -H*0.5f + 12, W*0.5f, 12);
		WALL(-W*0.5f + 12, 0, 12, H*0.5f);
		WALL(W*0.5f - 12, 0, 12, H*0.5f);
		if (channel == 2) {
			// Tinted walls: warm left, cool right, neutral floor/ceiling. Kept to
			// physically plausible reflectances (well under 1) -- the bounce series
			// converges to 1/(1-albedo), so values near unity pile up over the
			// settle frames and wash the room out.
			cf_draw_push_color(cf_make_color_rgb_f(0.75f, 0.13f, 0.08f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(-W*0.5f + 26, H*0.5f)), 0);
			cf_draw_pop_color();
			cf_draw_push_color(cf_make_color_rgb_f(0.08f, 0.20f, 0.75f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(W*0.5f - 26, -H*0.5f), cf_v2(W*0.5f, H*0.5f)), 0);
			cf_draw_pop_color();
			cf_draw_push_color(cf_make_color_rgb_f(0.68f, 0.68f, 0.66f));
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, H*0.5f - 26), cf_v2(W*0.5f, H*0.5f)), 0);
			cf_draw_box_rounded_fill(cf_make_aabb(cf_v2(-W*0.5f, -H*0.5f), cf_v2(W*0.5f, -H*0.5f + 26)), 0);
			cf_draw_pop_color();
		}
		// Pillars sit INSIDE the orbit radius so the light never travels through them
		// -- an emitter buried in an occluder would just switch itself off.
		// A single small block near the middle: enough to throw a shadow that sweeps
		// as the light orbits, without carving the room into black wedges that hide
		// the bounced fill this scene exists to show.
		WALL(0, 0, W*0.045f, H*0.045f);
		// The travelling light: a clean circular orbit clear of both the pillars and
		// the walls. Radius 10 keeps it above the paper's minimum source size, and
		// EMIT_ABS keeps it from occluding itself.
		if (channel < 2) {
			float t = g_scene_time * 0.5f;
			float ox = cosf(t) * W * 0.30f;
			float oy = sinf(t) * H * 0.30f;
			cf_draw_push_color(emis ? cf_make_color_rgb_f(9.0f, 7.6f, 5.6f)
			                        : cf_make_color_rgb_f(EMIT_ABS, EMIT_ABS, EMIT_ABS));
			cf_draw_circle_fill(cf_make_circle(cf_v2(ox, oy), 10));
			cf_draw_pop_color();
		}
	} else { // showcase: wide room, coloured walls, haze, glowing volume
		WALL(0, H*0.5f - 14, W*0.5f, 14);
		WALL(0, -H*0.5f + 14, W*0.5f, 14);
		WALL(-W*0.5f + 14, 0, 14, H*0.5f);
		WALL(W*0.5f - 14, 0, 14, H*0.5f);
		FOG(0, 0, W*0.5f - 14, H*0.5f - 14, 0.0030f);
		LIGHT(-W*0.47f + 31, 0, 7, H*0.42f, 6.0f, 0.40f, 0.12f);
		LIGHT(W*0.47f - 31, 0, 7, H*0.42f, 0.14f, 1.1f, 6.0f);
		LIGHT(0, H*0.5f - 34, W*0.09f, 5, 3.2f, 3.0f, 2.6f);
		WALL(-W*0.16f, H*0.04f, W*0.030f, H*0.16f);
		// The glowing gas volume emits; it carries no albedo (channel < 2), like
		// every other emitter.
		if (channel < 2) {
			cf_draw_push_color(emis ? cf_make_color_rgb_f(1.3f, 0.5f, 2.0f) : cf_make_color_rgb_f(0.020f, 0.016f, 0.024f));
			cf_draw_circle_fill(cf_make_circle(cf_v2(W*0.13f, -H*0.16f), H*0.13f));
			cf_draw_pop_color();
		}
	}

	#undef WALL
	#undef LIGHT
	#undef FOG
}

//--------------------------------------------------------------------------------------------------
// "bloom": a demoscene piece -- pure geometry, no representation. A phyllotaxis
// spiral (r = c*sqrt(n), theta = n * golden angle) of ~3400 emitters carries a
// hue wheel outward from a white-gold core; every 13th node runs brighter,
// which traces the 13-parastichy arms the pattern already contains. Dark vanes
// cast spoke shadows through the haze, thin rings halo like drafting lines, and
// heavily-feathered discs (cf_draw_push_shape_aa cranked far past its usual
// 1.5) lay down smoke for the light to stain. Phyllotaxis is self-similar, so
// the pull-back is composed at every scale -- the same reason a sunflower
// survives being looked at closely.

#define BLM_N     3400
#define BLM_C     470.0f          // r = BLM_C * sqrt(n)
#define BLM_GOLD  2.39996323f     // golden angle, radians

static int g_blm_ch;
// kind: 0 rect (half-extents a,b), 1 filled circle (radius a), 2 ring (radius
// a, stroke thickness b). aa is the shape antialias scale -- large values
// feather the border into soft smoke.
static void blm_shape(int kind, float cx, float cy, float a, float b, float rot, float aa,
                      float er, float eg, float eb, float ar, float ag, float ab_,
                      float dr, float dg, float db)
{
	CF_Color c;
	if (g_blm_ch == 0)      { if (er + eg + eb <= 0) return; c = cf_make_color_rgb_f(er, eg, eb); }
	else if (g_blm_ch == 1) { if (ar + ag + ab_ <= 0) return; c = cf_make_color_rgb_f(ar, ag, ab_); }
	else                    { if (dr + dg + db <= 0) return; c = cf_make_color_rgb_f(dr, dg, db); }
	cf_draw_push();
	cf_draw_push_shape_aa(aa);
	if (rot != 0) { cf_draw_translate(cx, cy); cf_draw_rotate(rot); cx = 0; cy = 0; }
	cf_draw_push_color(c);
	if (kind == 1)      cf_draw_circle_fill(cf_make_circle(cf_v2(cx, cy), a));
	else if (kind == 2) cf_draw_circle(cf_make_circle(cf_v2(cx, cy), a), b);
	else                cf_draw_quad_fill(cf_make_aabb(cf_v2(cx - a, cy - b), cf_v2(cx + a, cy + b)), 0);
	cf_draw_pop_color();
	cf_draw_pop_shape_aa();
	cf_draw_pop();
}

// Background layer: haze, smoke, and the guide rings -- everything that is
// rotation-invariant or too diffuse to read as moving. Recorded once.
static void blm_shapes_bg(void)
{
	blm_shape(0, 0, 0, 40000.0f, 40000.0f, 0, 1.5f, 0, 0, 0, 0.000022f, 0.000022f, 0.000026f, 0, 0, 0);
	for (int k = 0; k < 40; k++) {
		float t = (float)k / 39.0f;
		float ang = t * 4.7f + (float)(arc_hash((uint32_t)(k * 7 + 3)) % 100) * 0.006f;
		float r = 6500.0f + t * 20000.0f;
		float rad = 2200.0f + (float)(arc_hash((uint32_t)(k * 7 + 5)) % 2400);
		float hr, hg, hb; ap_hsv(fmodf(t * 300.0f + 210.0f, 360.0f), 0.5f, 1.0f, &hr, &hg, &hb);
		float f = 0.00100f;
		blm_shape(1, cosf(ang) * r, sinf(ang) * r, rad, 0, 0, 60.0f, 0, 0, 0,
		          f * (1.2f - 0.6f * hr), f * (1.2f - 0.6f * hg), f * (1.2f - 0.6f * hb), 0, 0, 0);
	}
	for (int k = 0; k < 14; k++) {
		float ang = (float)k * 1.7f + 0.9f;
		float r = 4200.0f + (float)(arc_hash((uint32_t)(k * 11 + 40)) % 14000);
		blm_shape(1, cosf(ang) * r, sinf(ang) * r, 1500.0f + (float)(arc_hash((uint32_t)(k * 11 + 41)) % 1500), 0, 0, 60.0f,
		          0, 0, 0, 0.00030f, 0.00030f, 0.00030f, 0.30f, 0.30f, 0.32f);
	}
	// rings: centred circles are invariant under the band rotations, so they
	// live here as still reference lines the moving layers play against
	{
		float rr[3] = { 2600.0f, 5300.0f, 8600.0f };
		for (int k = 0; k < 3; k++) {
			float hr, hg, hb; ap_hsv(fmodf(40.0f + rr[k] * 0.010f, 360.0f), 0.55f, 1.0f, &hr, &hg, &hb);
			blm_shape(2, 0, 0, rr[k], 85.0f, 0, 2.0f, 0.10f * hr, 0.10f * hg, 0.10f * hb, 0.010f, 0.010f, 0.010f, 0, 0, 0);
		}
	}
}

// The bead field, split into three radial bands that counter-rotate at replay
// like an astrolabe. Lamps (n % 55) are NOT recorded -- they draw live in
// blm_lights so they can pulse and precess against the bands.
#define BLM_BAND_R0 9500.0f
#define BLM_BAND_R1 18500.0f
// Four motion layers: inner, mid, and the outer band split into two
// interleaved streams (even/odd n) that counter-flow through each other --
// at the rim, neighbouring beads slide past one another instead of moving as
// one rigid sheet.
static const float g_blm_band_w[4] = { 0.045f, -0.032f, 0.022f, -0.014f }; // rad/s
static void blm_beads(int band)
{
	for (int n = 1; n <= BLM_N; n++) {
		float r = BLM_C * sqrtf((float)n);
		int b = r < BLM_BAND_R0 ? 0 : r < BLM_BAND_R1 ? 1 : ((n & 1) ? 2 : 3);
		if (b != band) continue;
		float th = (float)n * BLM_GOLD;
		float x = cosf(th) * r, y = sinf(th) * r;
		uint32_t hsh = arc_hash((uint32_t)n * 2654435761u);
		float h01 = (float)(hsh % 1000) * 0.001f;
		// order decays outward: perfect lattice at the core, growing positional
		// scatter toward the rim (r^2-weighted jitter)
		float rf = r / 27500.0f;
		float jr = rf * rf * 950.0f;
		x += ((float)(arc_hash(hsh + 1) % 1000) * 0.002f - 1.0f) * jr;
		y += ((float)(arc_hash(hsh + 2) % 1000) * 0.002f - 1.0f) * jr;
		float s = (14.0f + r * 0.0068f) * (0.45f + 2.0f * h01 * h01);
		if (n % 34 == 0) s *= 2.6f;
		// the rim goes bimodal: much of it thins to dust while every 144th node
		// is a glass boulder big enough to throw a real shadow
		if (rf > 0.5f && (hsh % 5) < 2) s *= 0.45f;
		if (n % 144 == 0) s *= 4.5f;
		float hue = fmodf(200.0f + r * 0.0125f, 360.0f);
		float hr, hg, hb; ap_hsv(hue, 0.88f, 1.0f, &hr, &hg, &hb);
		float A = (n % 144 == 0) ? 0.10f : 0.045f;
		float ar2 = A * (1.15f - 0.75f * hr), ag2 = A * (1.15f - 0.75f * hg), ab2 = A * (1.15f - 0.75f * hb);
		float dr2 = 0.55f * hr, dg2 = 0.55f * hg, db2 = 0.55f * hb;
		float er2 = 0.07f * hr, eg2 = 0.07f * hg, eb2 = 0.07f * hb;
		uint32_t kind = hsh % 10;
		if (kind < 5)      blm_shape(1, x, y, s, 0, 0, 2.0f, er2, eg2, eb2, ar2, ag2, ab2, dr2, dg2, db2);
		else if (kind < 8) blm_shape(0, x, y, s * 0.8f, s * 0.8f, th, 2.0f, er2, eg2, eb2, ar2, ag2, ab2, dr2, dg2, db2);
		else               blm_shape(2, x, y, s, s * 0.38f, 0, 2.0f, er2, eg2, eb2, ar2, ag2, ab2, dr2, dg2, db2);
	}
}

// Drifting volumetrics, drawn immediate every frame: great soft fog masses on
// slow incommensurate orbits. Light passing through them tints toward their
// hue; lamps behind them dim and halo.
static void blm_volumetrics(float t)
{
	for (int k = 0; k < 7; k++) {
		uint32_t h = arc_hash((uint32_t)(k * 977 + 5));
		float orbit = 8000.0f + (float)(h % 9000);
		float w = (0.038f + (float)(h % 97) * 0.00035f) * ((k & 1) ? 1.0f : -1.0f);
		float ph = (float)k * 2.4f;
		float x = cosf(ph + w * t) * orbit, y = sinf(ph + w * t) * orbit;
		float r = 2600.0f + (float)(h % 2800);
		float hr, hg, hb; ap_hsv((float)((k * 47) % 360), 0.5f, 1.0f, &hr, &hg, &hb);
		float f = 0.0012f;
		blm_shape(1, x, y, r, 0, 0, 70.0f, 0, 0, 0,
		          f * (1.2f - 0.6f * hr), f * (1.2f - 0.6f * hg), f * (1.2f - 0.6f * hb),
		          0.12f, 0.12f, 0.13f);
	}
	{
		float x = cosf(0.9f + t * 0.021f) * 16500.0f;
		float y = sinf(0.9f + t * 0.021f) * 16500.0f;
		blm_shape(1, x, y, 8500.0f, 0, 0, 80.0f, 0, 0, 0, 0.00075f, 0.00070f, 0.00090f, 0.10f, 0.10f, 0.11f);
	}
}

// Edge ramp for the live lights: 1 inside the VISIBLE rect, easing to 0 at the
// outer edge of the padded canvas (HRC_PAD). A light drifting toward the frame
// fades up across the border ring instead of popping the moment its pixels
// exist -- set each frame from the camera by bloom_draw.
static float g_blm_vis_hw, g_blm_vis_hh, g_blm_marg;
static float g_blm_zoom_u = 1.0f; // eased zoom phase, 0 = close-up (blm_lights adapts the core)
static float blm_edge_ramp(float x, float y)
{
	if (g_blm_marg <= 1.0f) return 1.0f;
	float dx = fabsf(x) - g_blm_vis_hw;
	float dy = fabsf(y) - g_blm_vis_hh;
	float d = dx > dy ? dx : dy;
	if (d <= 0.0f) return 1.0f;
	if (d >= g_blm_marg) return 0.0f;
	float q = 1.0f - d / g_blm_marg;
	return q * q * (3.0f - 2.0f * q);
}

// Soft ember swarm around the core, the close-zoom payoff: embers in three
// orbital shells whirling prograde with Keplerian-ish speed falloff -- layered
// clockwork parallax up close, invisible dust from afar. Each ember is
// VOLUMETRIC: pass 1 (early, with the fog) lays down a heavily feathered
// absorption puff with a whisper of albedo -- its own little participating
// medium -- and pass 2 (after the lights) sets a deep-orange emissive heart
// inside it, so the glow scatters within the puff instead of reading as a
// flat disc. Hearts shade orange to ember-red across the shells.
static void blm_embers(float t, int clouds_pass)
{
	static const float shell_r[3] = { 780.0f, 1450.0f, 2150.0f };
	for (int k = 0; k < 90; k++) {
		uint32_t h = arc_hash((uint32_t)(k * 613 + 17));
		int shell = k % 3;
		float rk = shell_r[shell] + (float)(h % 280) - 140.0f;
		float tt = (float)shell * 0.5f;
		float w = 0.5f * powf(900.0f / rk, 1.2f);
		float ang = (float)(h % 628) * 0.01f + w * t;
		float x = cosf(ang) * rk, y = sinf(ang) * rk;
		float ramp = blm_edge_ramp(x, y);
		if (ramp <= 0.0f) continue;
		float sz = 16.0f + (float)(arc_hash(h) % 34);
		if (clouds_pass) {
			// the puff: warm-shadowed smoke (absorbs blue hardest) that both
			// glows from its heart and catches the core light on its albedo
			blm_shape(1, x, y, sz * 3.4f, 0, 0, 40.0f, 0, 0, 0,
			          0.0038f, 0.0060f, 0.0095f, 0.16f, 0.11f, 0.07f);
		} else {
			float tw = 0.75f + 0.25f * sinf(t * (1.3f + tt) + (float)k);
			// kept dim on purpose: hot orange rides AgX's shoulder into yellow,
			// so the deep-ember read comes from LOW emission and a red-heavy ratio
			float e = (0.36f + 0.30f * (1.0f - tt)) * tw * ramp;
			blm_shape(1, x, y, sz, 0, 0, 16.0f,
			          (1.05f * (1.0f - tt) + 0.80f * tt) * e,
			          (0.28f * (1.0f - tt) + 0.13f * tt) * e,
			          (0.05f * (1.0f - tt) + 0.04f * tt) * e,
			          0.014f, 0.014f, 0.014f, 0, 0, 0);
		}
	}
}

// The lights, all live: a fleet of twelve orbiters on Keplerian-ish paths in
// mixed directions -- sizes run from hot little sparks to big soft suns
// (smaller burns hotter, so every light reads distinct) -- plus the breathing
// zoom-adaptive core. Moving lights dragging shadows around the glass field
// ARE the show; the recorded geometry just gives them something to relight.
static void blm_lights(float t)
{
	for (int k = 0; k < 12; k++) {
		uint32_t h = arc_hash((uint32_t)(k * 331 + 7));
		float R = 3600.0f + 22000.0f * ((float)k + 0.4f * (float)(h % 100) * 0.01f) / 12.0f;
		float w = 0.45f * powf(4200.0f / R, 0.85f) * ((h & 4) ? 1.0f : -1.0f);
		float ang = (float)(h % 628) * 0.01f + w * t;
		float x = cosf(ang) * R, y = sinf(ang) * R;
		float ramp = blm_edge_ramp(x, y);
		if (ramp <= 0.0f) continue;
		// size grows with orbit radius: quick hot sparks weave the inner field,
		// the big soft suns patrol the rim
		float h01 = (float)(h % 1000) * 0.001f;
		float rn = (R - 3600.0f) / 22000.0f;
		float sz = (110.0f + 800.0f * rn * rn) * (0.8f + 0.5f * h01);
		float hue = fmodf(30.0f + (float)k * 137.5f, 360.0f);
		float hr, hg, hb; ap_hsv(hue, (k % 3) ? 0.65f : 0.15f, 1.0f, &hr, &hg, &hb);
		float e = (0.9f + 260.0f / sz) * ramp;
		blm_shape(1, x, y, sz, 0, 0, 2.5f, e * hr, e * hg, e * hb, 0.010f, 0.010f, 0.010f, 0, 0, 0);
	}
	{
		// Exposure adaptation: at close zoom the core fills the frame and a
		// fixed emission whites it out, so its brightness follows the camera --
		// a soft glowing heart up close, a blazing beacon from far away.
		float breath = (0.85f + 0.30f * sinf(t * 0.9f)) * (0.18f + 0.82f * powf(g_blm_zoom_u, 0.7f));
		blm_shape(1, 0, 0, 440.0f, 0, 0, 8.0f, 1.5f * breath, 1.35f * breath, 1.05f * breath, 0.010f, 0.010f, 0.010f, 0, 0, 0);
		blm_shape(1, 0, 0, 250.0f, 0, 0, 3.0f, 2.5f * breath, 2.3f * breath, 1.9f * breath, 0.014f, 0.014f, 0.014f, 0, 0, 0);
	}
}

static CF_DrawList g_blm_bg[3], g_blm_band[4][3]; // [band][channel]
static int g_blm_recorded = 0;

static void bloom_draw(void)
{
	if (!g_blm_recorded) {
		for (int ch = 0; ch < 3; ch++) {
			g_blm_ch = ch;
			g_blm_bg[ch] = cf_make_draw_list();
			cf_draw_list_begin(g_blm_bg[ch]);
			blm_shapes_bg();
			cf_draw_list_end();
			for (int b = 0; b < 4; b++) {
				g_blm_band[b][ch] = cf_make_draw_list();
				cf_draw_list_begin(g_blm_band[b][ch]);
				blm_beads(b);
				cf_draw_list_end();
			}
		}
		g_blm_recorded = 1;
	}

	// Camera: 32s-per-leg eased ping-pong log zoom. The motion inside the frame
	// comes from the layers, not the camera: counter-rotating bands, precessing
	// pulsing lamps, orbiters, breathing core, drifting fog.
	float u;
	{
		const char* zt = getenv("HRC_ZOOM_T");
		if (zt) {
			u = (float)atof(zt);
		} else {
			const float T = 32.0f;
			float raw = fmodf(g_scene_time, 2.0f * T) / T;
			u = raw < 1.0f ? raw : 2.0f - raw;
		}
		if (u < 0) u = 0; if (u > 1) u = 1;
		u = u * u * (3.0f - 2.0f * u);
	}
	const float vw0 = 1600.0f, vw1 = 60000.0f;
	float vw = vw0 * powf(vw1 / vw0, u);
	float vh = vw * (float)view_h / (float)view_w;
	// the cascade canvas extends HRC_PAD pixels past the window on every side:
	// the projection covers the padded extent, the window shows the centre crop,
	// and lights ramp up across the border ring (blm_edge_ramp) before entering
	float vwp = vw * (float)world_w / (float)view_w;
	float vhp = vh * (float)world_h / (float)view_h;
	g_blm_vis_hw = vw * 0.5f;
	g_blm_vis_hh = vh * 0.5f;
	g_blm_marg = (vwp - vw) * 0.5f;
	g_blm_zoom_u = u;
	g_abs_zoom = vwp / (float)world_w;

	CF_Canvas targets[3] = { hrc.emissivity, hrc.absorption, hrc.diffuse };
	for (int ch = 0; ch < 3; ch++) {
		cf_draw_push();
		cf_draw_projection(cf_ortho_2d(0, 0, vwp, vhp));
		push_f16_render_state();
		cf_draw_list(g_blm_bg[ch]);
		g_blm_ch = ch;
		blm_volumetrics(g_scene_time);
		blm_embers(g_scene_time, 1);
		for (int b = 0; b < 4; b++) {
			cf_draw_push();
			cf_draw_rotate(g_blm_band_w[b] * g_scene_time);
			cf_draw_list(g_blm_band[b][ch]);
			cf_draw_pop();
		}
		blm_lights(g_scene_time);
		blm_embers(g_scene_time, 0);
		cf_draw_pop_render_state();
		cf_render_to(targets[ch], true);
		cf_draw_pop();
	}
}

// Rasterize the selected scene into emission + absorption. Shared by the
// interactive loop and the headless capture so both light the same thing.
void hrc_scene_draw(void)
{
	// Author to VIEW dims: the projection spans the padded canvas, so content
	// sized to the window lands centred with the HRC_PAD ring empty around it.
	float W = (float)view_w, H = (float)view_h;
	cf_app_update(NULL); // acquire a command buffer
	hrc_init_once();
	// The text scene is lit entirely by two large emitters filling the frame, so it
	// sits far higher on the exposure curve than the room scenes. Pulling it down
	// keeps the letter cores from clipping to white and lets the wash hold its hue
	// and fall off into dark corners.
	// Per-scene display scale. The bounce-lit room needs pulling down as well: a
	// closed box with reflective walls converges to roughly 1/(1-albedo) times the
	// direct light, which clips at the room scenes' default exposure.
	g_exposure = 1.0f;
	g_bounce_boost = 1.0f;
	if (!CF_STRCMP(scene_name(), "maze")) { g_exposure = 2.5f; g_bounce_boost = 2.0f; }
	if (!CF_STRCMP(scene_name(), "text"))  g_exposure = 0.10f;
	if (!CF_STRCMP(scene_name(), "orbit")) g_exposure = 2.0f;

	if (!CF_STRCMP(scene_name(), "bloom")) {
		g_exposure = 1.15f;
		bloom_draw();
		return;
	}
	g_abs_zoom = 1.0f;

	// 0 = emission, 1 = absorption, 2 = diffuse albedo (drives the bounce feedback).
	// The projection spans the PADDED canvas while the shapes are authored to
	// VIEW dims, so content lands centred at 1:1 with the HRC_PAD ring empty
	// around it -- the window crop then shows exactly the authored scene.
	for (int channel = 0; channel < 3; channel++) {
		cf_draw_push();
		cf_draw_projection(cf_ortho_2d(0, 0, (float)world_w, (float)world_h));
		push_f16_render_state();
		scene_shapes(channel, W, H);
		cf_draw_pop_render_state();

		cf_render_to(channel == 0 ? hrc.emissivity : channel == 1 ? hrc.absorption : hrc.diffuse, true);
		cf_draw_pop();
	}
}

// Composite the type back over the finished HRC image with the ordinary draw
// path: default shader, default blend, partial alpha. The cascade pass already
// did the lighting -- glow, colour bleed, cast shadows -- but reconstructs it
// through a light field, which is low-frequency by nature and leaves thin
// letterforms soft. Stamping a flat translucent copy on top restores the crisp
// edge while the volumetric result still shows through it.
static void hrc_text_overlay(void)
{
	if (CF_STRCMP(scene_name(), "text")) return; // only the type scene stamps
	{ const char* e = getenv("HRC_OVL_E"); if (e) g_overlay_emis = (float)atof(e); }
	{ const char* o = getenv("HRC_OVL_O"); if (o) g_overlay_occ  = (float)atof(o); }
	{ const char* t = getenv("HRC_OVL_TONE"); if (t) g_overlay_tone = (float)atof(t); }
	{ const char* w = getenv("HRC_OVL_WHITE"); if (w) g_overlay_white = (float)atof(w); }
	if (g_overlay_emis <= 0 && g_overlay_occ <= 0) return;

	cf_draw_push();
	cf_draw_projection(cf_ortho_2d(0, 0, (float)world_w, (float)world_h));
	scene_shapes(3, (float)view_w, (float)view_h); // same layout (VIEW dims, like the lit pass), overlay colours
	cf_render_to(hrc.fluence, false);                // composite, do not clear
	cf_draw_pop();
}

// Headless capture: render the scene, settle, dump display + linear, exit.
void hrc_testscene(void)
{
	// Settle over several frames before reading back. A single frame is not safe:
	// the readback can report ready before the GPU has finished the dispatches, and
	// we capture zeros (reproducible on back-to-back runs). Re-running the compute
	// each frame is idempotent here -- the scene textures don't change.
	//
	// 48 frames, not a handful: each frame folds in one bounce, and the bounce
	// does more than converge a brightness series -- it diffuses light INTO the
	// finite-absorption walls one cell per iteration, which is what makes a wall's
	// face read lit through its thickness. Capturing early shows walls darker than
	// the interactive app ever looks. (HRC_SETTLE overrides, mostly for quick
	// artifact checks where bounce convergence is irrelevant.)
	//
	// hrc_scene_draw opens a frame (cf_app_update), so each pass pairs it with one
	// cf_app_draw_onto_screen to submit. Leaving a frame open no longer loses its GPU
	// work, but pairing keeps the frame accounting obvious.
	int settle = getenv("HRC_SETTLE") ? atoi(getenv("HRC_SETTLE")) : 48;
	if (settle < 2) settle = 2;
	for (int f = 0; f < settle; f++) {
		hrc_scene_draw();   // opens the frame (cf_app_update)
		hrc_compute();
		hrc_text_overlay();    // translucent stamp over the finished image
		cf_app_draw_onto_screen(true);   // submits it
	}

	CF_Readback rb = cf_canvas_readback(hrc.fluence);
	CF_Readback rbl = cf_canvas_readback(hrc.fluence_lin);
	for (int t = 0; t < 240 && (!cf_readback_ready(rb) || !cf_readback_ready(rbl)); t++) { cf_app_update(NULL); cf_app_draw_onto_screen(true); }
	// Dumps land in the working directory. HRC_OUT can point them elsewhere; it is
	// used as a prefix, so a trailing slash gives a directory and anything else
	// prefixes the filename.
	const char* out = getenv("HRC_OUT");
	char path[512];
	if (!out) out = "";
	if (cf_readback_ready(rb)) {
		int sz = cf_readback_size(rb);
		unsigned char* buf = (unsigned char*)cf_alloc(sz);
		cf_readback_data(rb, buf, sz);
		CF_SNPRINTF(path, sizeof(path), "%shrc_shot.raw", out);
		FILE* f = fopen(path, "wb");
		if (f) { fwrite(buf, 1, sz, f); fclose(f); printf("SCENE %s: %dx%d rgba8 -> %s\n", scene_name(), world_w, world_h, path); }
		else    { printf("SCENE %s: failed to open %s for writing\n", scene_name(), path); }
		fflush(stdout);
		cf_free(buf);
	}
	if (cf_readback_ready(rbl)) {
		int sz = cf_readback_size(rbl), N = world_w * world_h;
		unsigned char* buf = (unsigned char*)cf_alloc(sz); cf_readback_data(rbl, buf, sz);
		uint16_t* h = (uint16_t*)buf;
		CF_SNPRINTF(path, sizeof(path), "%shrc_lin.raw", out);
		FILE* f = fopen(path, "wb");
		if (f) {
			for (int i = 0; i < N; i++) { float rgb[3] = { half_to_float(h[i*4+0]), half_to_float(h[i*4+1]), half_to_float(h[i*4+2]) }; fwrite(rgb, 4, 3, f); }
			fclose(f);
			printf("SCENE lin -> %s\n", path);
		} else {
			printf("SCENE lin: failed to open %s for writing\n", path);
		}
		fflush(stdout);
		cf_free(buf);
	}
	exit(0);
}


int main(int argc, char* argv[])
{
	// Window / world resolution. HRC_RES=WxH picks a non-square resolution
	// (rounded to a multiple of 4 so the probe lattice stays integral).
	world_w = GI_REF;
	world_h = GI_REF;
	{
		const char* res = getenv("HRC_RES");
		if (res) {
			int rw = 0, rh = 0;
			if (sscanf(res, "%dx%d", &rw, &rh) == 2 && rw >= 256 && rh >= 256 && rw <= 4096 && rh <= 4096) {
				int q = 4;
				world_w = (rw / q) * q;
				world_h = (rh / q) * q;
			}
		}
	}
	// HRC_PAD=N: extend the lighting world N pixels past the window on every
	// side (default 96, 0 restores the exact viewport-sized cascade). Off-screen
	// emitters in the ring still light the visible frame, so lights no longer
	// pop into existence at the edge; animated scenes also ramp them across the
	// ring. Costs ((view+2*pad)/view)^2 in cascade work.
	view_w = world_w;
	view_h = world_h;
	{
		const char* pd = getenv("HRC_PAD");
		g_pad = pd ? atoi(pd) : 96;
		if (g_pad < 0) g_pad = 0;
		if (g_pad > 512) g_pad = 512;
		g_pad = (g_pad / 4) * 4;
	}
	world_w = view_w + 2 * g_pad;
	world_h = view_h + 2 * g_pad;


	cf_make_app("HRC GI", 0, 0, 0, view_w, view_h, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	cf_clear_color(0, 0, 0, 1);

	{ const char* hs = getenv("HRC_SCALE"); if (hs) { int v = atoi(hs);
		if (v == 1 || v == 2 || v == 4) g_hrc_scale = v; } }
	{ const char* hb = getenv("HRC_BLUR"); if (hb) blur_on = atoi(hb) != 0; }
	{ const char* tl = getenv("HRC_TRACE_LEVELS"); if (tl) { int v = atoi(tl);
		if (v >= 1 && v <= GI_N_MAX) g_trace_levels = v; } }
	{ const char* ml = getenv("HRC_MAX_LEVELS"); if (ml) { int v = atoi(ml);
		if (v >= 2) g_max_levels = v; } }
	{ const char* sk = getenv("HRC_SKIP"); if (sk) g_skip = sk; }

	hrc_init();

	// Headless: render one scene, dump it, exit. This is what regress.py drives.
	scene_select(getenv("HRC_SCENE"));

	// Headless capture is its own switch now that scenes cycle at runtime: HRC_SCENE
	// only chooses where to start, in both modes.
	if (getenv("HRC_SHOT")) hrc_testscene();

	// HRC_PERF=N: time N frames of the full per-frame pipeline (scene draw +
	// bounce + cascade + display + present) after a warmup, print ms/frame, exit.
	// Present mode goes immediate so vsync cannot clamp the number. Combine with
	// HRC_SKIP / HRC_TRACE_LEVELS / HRC_MAX_LEVELS / HRC_SCALE for a timing matrix.
	if (getenv("HRC_PERF")) {
		int N = atoi(getenv("HRC_PERF"));
		if (N < 10) N = 200;
		cf_app_set_present_mode(CF_PRESENT_MODE_IMMEDIATE);
		for (int f = 0; f < 30; f++) {
			hrc_scene_draw();
			hrc_compute();
			cf_app_draw_onto_screen(true);
		}
		uint64_t t0 = cf_get_ticks();
		for (int f = 0; f < N; f++) {
			hrc_scene_draw();
			hrc_compute();
			cf_app_draw_onto_screen(true);
		}
		double ms = (double)(cf_get_ticks() - t0) / (double)cf_get_tick_frequency() * 1000.0 / N;
		printf("PERF %s %dx%d scale=%d trace=%d cap=%d skip='%s' blur=%d bounce=%d: %.3f ms/frame\n",
			scene_name(), world_w, world_h, g_hrc_scale, g_trace_levels, g_max_levels,
			g_skip, blur_on, bounce_on, ms);
		fflush(stdout);
		exit(0);
	}

	// HRC_VIDEO=N: headless video capture -- render N frames stepping the scene
	// clock by HRC_VIDEO_DT seconds each (default 4/30: 4x time compression at a
	// 30fps timeline), append rgba8 fluence frames to hrc_video.raw, exit. Pipe
	// the file through ffmpeg (-f rawvideo -pixel_format rgba) for a shareable
	// clip of the animated scenes.
	if (getenv("HRC_VIDEO")) {
		int N = atoi(getenv("HRC_VIDEO"));
		if (N < 1) N = 240;
		float dt = getenv("HRC_VIDEO_DT") ? (float)atof(getenv("HRC_VIDEO_DT")) : (4.0f / 30.0f);
		const char* out = getenv("HRC_OUT");
		if (!out) out = "";
		char path[512];
		CF_SNPRINTF(path, sizeof(path), "%shrc_video.raw", out);
		FILE* vf = fopen(path, "wb");
		if (!vf) { printf("VIDEO: cannot open %s\n", path); exit(1); }
		size_t frame_bytes = (size_t)world_w * world_h * 4;
		unsigned char* buf = (unsigned char*)cf_alloc(frame_bytes);
		// settle the bounce before frame 0 so the clip doesn't open mid-converge
		for (int f = 0; f < 24; f++) { hrc_scene_draw(); hrc_compute(); cf_app_draw_onto_screen(true); }
		for (int f = 0; f < N; f++) {
			g_scene_time = f * dt;
			hrc_scene_draw();
			hrc_compute();
			cf_app_draw_onto_screen(true);
			CF_Readback rb = cf_canvas_readback(hrc.fluence);
			for (int t2 = 0; t2 < 240 && !cf_readback_ready(rb); t2++) { cf_app_update(NULL); cf_app_draw_onto_screen(true); }
			if (cf_readback_ready(rb)) {
				cf_readback_data(rb, buf, frame_bytes);
				fwrite(buf, 1, frame_bytes, vf);
			}
			cf_destroy_readback(rb);
			if (f % 60 == 0) { printf("VIDEO frame %d/%d\n", f, N); fflush(stdout); }
		}
		fclose(vf);
		cf_free(buf);
		printf("VIDEO: wrote %d frames %dx%d rgba8 -> %s\n", N, world_w, world_h, path);
		fflush(stdout);
		exit(0);
	}

	// Interactive: draw the selected scene and light it every frame.
	while (cf_app_is_running()) {
		cf_app_update(NULL);
		g_scene_time += CF_DELTA_TIME;
		if (cf_key_just_pressed(CF_KEY_RIGHT)) scene_cycle(1);
		if (cf_key_just_pressed(CF_KEY_LEFT)) scene_cycle(-1);
		// Maze interaction: SPACE toggles the cursor probe light, left click
		// drops a coloured light (hue walks the golden angle so neighbours
		// differ), right click removes the nearest one.
		if (!CF_STRCMP(scene_name(), "maze")) {
			g_mz_mx = cf_mouse_x() - view_w * 0.5f;
			g_mz_my = view_h * 0.5f - cf_mouse_y();
			if (cf_key_just_pressed(CF_KEY_SPACE)) g_mz_probe_on = !g_mz_probe_on;
			if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) && g_mz_nlights < MZ_MAX_LIGHTS) {
				g_mz_lights[g_mz_nlights].x = g_mz_mx;
				g_mz_lights[g_mz_nlights].y = g_mz_my;
				g_mz_lights[g_mz_nlights].hue = fmodf(40.0f + g_mz_nlights * 137.5f, 360.0f);
				g_mz_nlights++;
			}
			if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT) && g_mz_nlights > 0) {
				int best = 0;
				float bd = 1e30f;
				for (int i = 0; i < g_mz_nlights; i++) {
					float dx = g_mz_lights[i].x - g_mz_mx, dy = g_mz_lights[i].y - g_mz_my;
					float d = dx * dx + dy * dy;
					if (d < bd) { bd = d; best = i; }
				}
				g_mz_lights[best] = g_mz_lights[--g_mz_nlights];
			}
		}
		// Cascade scale. Storage is sized for scale 1, so this only changes the
		// working extents and the dispatch sizes -- nothing is reallocated.
		if (cf_key_just_pressed(CF_KEY_1)) { g_hrc_scale = 1; hrc_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_2)) { g_hrc_scale = 2; hrc_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_4)) { g_hrc_scale = 4; hrc_apply_scale(); }
		if (cf_key_just_pressed(CF_KEY_B)) bounce_on = !bounce_on;
		if (cf_key_just_pressed(CF_KEY_C)) blur_on = !blur_on;
		hrc_scene_draw();
		hrc_compute();
		hrc_text_overlay();
		cf_draw_canvas(hrc.fluence, cf_v2(0, 0), cf_v2((float)world_w, (float)world_h));

		// Controls, drawn like any other shape. No scene title here: every scene
		// captions itself inside the world, so a HUD copy would just duplicate it.
		// Push/pop are balanced -- these stacks are global and leak across frames
		// otherwise.
		cf_push_font_size(view_h * 0.022f);
		cf_draw_push_color(cf_make_color_rgb_f(0.62f, 0.72f, 0.80f));
		char hud[256];
		CF_SNPRINTF(hud, sizeof(hud), "%s   (%d/%d)      %dx%d  cascade 1/%d",
			scene_name(), g_scene_index + 1, g_scene_count, world_w, world_h, g_hrc_scale);
		cf_draw_text(hud, cf_v2(-view_w * 0.46f, -view_h * 0.415f), -1);
		CF_SNPRINTF(hud, sizeof(hud), "left/right scene    1/2/4 cascade scale    B bounce %s    C blur %s",
			bounce_on ? "on" : "off", blur_on ? "on" : "off");
		cf_draw_text(hud, cf_v2(-view_w * 0.46f, -view_h * 0.45f), -1);
		if (!CF_STRCMP(scene_name(), "maze")) {
			// top of the window: a third line under the controls falls off the
			// bottom edge
			CF_SNPRINTF(hud, sizeof(hud), "space probe light %s    LMB add light    RMB remove    (%d placed)",
				g_mz_probe_on ? "on" : "off", g_mz_nlights);
			cf_draw_text(hud, cf_v2(-view_w * 0.46f, view_h * 0.47f), -1);
		}
		cf_draw_pop_color();
		cf_pop_font_size();

		cf_app_draw_onto_screen(true);
	}

	hrc_shutdown();
	cf_destroy_app();
	return 0;
}
