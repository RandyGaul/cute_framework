#ifndef CF_BUILTIN_SHADERS_H
#define CF_BUILTIN_SHADERS_H

#include "cute_shader.h"

//--------------------------------------------------------------------------------------------------
// Builtin shaders. These get cross-compiled at runtime.

// Render as white -- for testing.
static const char* s_basic_vs = R"(
layout (location = 0) in vec2 in_posH;

void main()
{
	gl_Position = vec4(in_posH, 0, 1);
}
)";
static const char* s_basic_fs = R"(
layout(location = 0) out vec4 result;

void main()
{
	result = vec4(1);
}
)";

// Copy the app's offscreen canvas onto the swapchain texture (backfbuffer).
static const char* s_backbuffer_vs = R"(
layout (location = 0) in vec2 in_posH;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 uv;

void main()
{
	uv = in_uv;
	gl_Position = vec4(in_posH, 0, 1);
}
)";
static const char* s_backbuffer_fs = R"(
layout (location = 0) in vec2 uv;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
};

#include "smooth_uv.shd"

void main()
{
	vec4 color = texture(u_image, smooth_uv(uv, u_texture_size));
	result = color;
}
)";

//--------------------------------------------------------------------------------------------------
// Utility shaders (included into other shaders).

static const char* s_gamma = R"(
vec4 gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);
}

vec4 de_gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);
}
)";

static const char* s_blend = R"(
// HSV <-> RGB from : http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// And https://www.shadertoy.com/view/MsS3Wc

vec3 rgb_to_hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
	vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv_to_rgb(vec3 c)
{
	vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
	rgb = rgb*rgb*(3.0-2.0*rgb);
	return c.z * mix(vec3(1.0), rgb, c.y);
}

vec3 hue(vec3 base, vec3 tint)
{
	base = rgb_to_hsv(base);
	tint = rgb_to_hsv(tint);
	return hsv_to_rgb(vec3(tint.r, base.gb));
}

vec4 hue(vec4 base, vec4 tint)
{
	return vec4(hue(base.rgb, tint.rgb), base.a);
}

float overlay(float base, float blend)
{
	return (base <= 0.5) ? 2*base * blend : 1-2*(1-base) * (1-blend);
}

vec3 overlay(vec3 base, vec3 blend)
{
	return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));
}

vec4 overlay(vec4 base, vec4 blend)
{
	return vec4(overlay(base.rgb, blend.rgb), base.a);
}

float softlight(float base, float blend)
{
	if (blend <= 0.5) return base - (1-2*blend)*base*(1-base);
	else return base + (2.0 * blend - 1) * (((base <= 0.25) ? ((16.0 * base - 12.0) * base + 4.0) * base : sqrt(base)) - base);
}

vec3 softlight(vec3 base, vec3 blend)
{
	return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));
}

vec4 softlight(vec4 base, vec4 blend)
{
	return vec4(softlight(base.rgb, blend.rgb), base.a);
}
)";

static const char* s_distance = R"(
float safe_div(float a, float b)
{
	return b == 0.0 ? 0.0 : a / b;
}

float safe_len(vec2 v)
{
	float d = dot(v,v);
	return d == 0.0 ? 0.0 : sqrt(d);
}

vec2 safe_norm(vec2 v, float l)
{
	return mix(vec2(0), v / l, l == 0.0 ? 0.0 : 1.0);
}

vec2 skew(vec2 v)
{
	return vec2(-v.y, v.x);
}

float det2(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

float sdf_stroke(float d)
{
	return abs(d) - v_stroke;
}

float sdf_intersect(float a, float b)
{
	return max(a, b);
}

float sdf_union(float a, float b)
{
	return min(a, b);
}

float sdf_subtract(float d0, float d1)
{
	return max(d0, -d1);
}

float dd(float d)
{
	return length(vec2(dFdx(d), dFdy(d)));
}

// Given two colors a and b, and a distance to the isosurface of a shape,
// apply antialiasing, fill, and surface stroke FX.
vec4 sdf(vec4 a, vec4 b, float d)
{
	float wire_d = sdf_stroke(d);
	vec4 stroke_aa = mix(b, a, smoothstep(0.0, v_aa, wire_d));
	vec4 stroke_no_aa = wire_d <= 0.0 ? b : a;

	vec4 fill_aa = mix(b, a, smoothstep(0.0, v_aa, d));
	vec4 fill_no_aa = clamp(d, -1.0, 1.0) <= 0.0 ? b : a;

	vec4 stroke = mix(stroke_aa, stroke_aa, v_aa > 0.0 ? 1.0 : 0.0);
	vec4 fill = mix(fill_no_aa, fill_aa, v_aa > 0.0 ? 1.0 : 0.0);

	result = mix(stroke, fill, v_fill);
	return result;
}

float distance_aabb(vec2 p, vec2 he)
{
	vec2 d = abs(p) - he;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float distance_box(vec2 p, vec2 c, vec2 he, vec2 u)
{
	mat2 m = transpose(mat2(u, skew(u)));
	p = p - c;
	p = m * p;
	return distance_aabb(p, he);
}

// Referenced from: https://www.shadertoy.com/view/3tdSDj
float distance_segment(vec2 p, vec2 a, vec2 b)
{
	vec2 n = b - a;
	vec2 pa = p - a;
	float d = safe_div(dot(pa,n), dot(n,n));
	float h = clamp(d, 0.0, 1.0);
	return safe_len(pa - h * n);
}

// Referenced from: https://www.shadertoy.com/view/XsXSz4
float distance_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
	vec2 e0 = b - a;
	vec2 e1 = c - b;
	vec2 e2 = a - c;

	vec2 v0 = p - a;
	vec2 v1 = p - b;
	vec2 v2 = p - c;

	vec2 pq0 = v0 - e0 * clamp(safe_div(dot(v0, e0), dot(e0, e0)), 0.0, 1.0);
	vec2 pq1 = v1 - e1 * clamp(safe_div(dot(v1, e1), dot(e1, e1)), 0.0, 1.0);
	vec2 pq2 = v2 - e2 * clamp(safe_div(dot(v2, e2), dot(e2, e2)), 0.0, 1.0);

	float s = det2(e0, e2);
	vec2 d = min(min(vec2(dot(pq0, pq0), s * det2(v0, e0)),
	                 vec2(dot(pq1, pq1), s * det2(v1, e1))),
	                 vec2(dot(pq2, pq2), s * det2(v2, e2)));

	return -sqrt(d.x) * sign(d.y);
}

// Referenced from: https://www.shadertoy.com/view/wdBXRW
float distance_polygon(vec2 p, vec2[8] v, int N)
{
	float d = dot(p-v[0], p-v[0]);
	float s = 1.0;
	for (int i=0, j=N-1; i<N; j=i, i++) {
		vec2 e = v[j] - v[i];
		vec2 w = p - v[i];
		vec2 b = w - e * clamp(dot(w,e)/dot(e,e), 0.0, 1.0);
		d = min(d, dot(b,b));

		bvec3 cond = bvec3(p.y     >= v[i].y,
		                   p.y     <  v[j].y,
		                   e.x*w.y >  e.y*w.x);
		if (all(cond) || all(not(cond))) {
			s =- s;
		}
	}

	return s * sqrt(d);
}
)";

static const char* s_smooth_uv = R"(
vec2 smooth_uv(vec2 uv, vec2 texture_size)
{
	vec2 pixel = uv * texture_size;
	vec2 seam = floor(pixel + 0.5);
	pixel = seam + clamp((pixel - seam) / fwidth(pixel), -0.5, 0.5);
	return pixel / texture_size;
}
)";

// Stub function. This gets replaced by injected user-shader code via #include.
static const char* s_shader_stub = R"(
vec4 shader(vec4 color, ShaderParams params)
{
	return color;
}
)";

//--------------------------------------------------------------------------------------------------
// Primary cute_draw.h shader.

static const char* s_draw_vs = R"(
layout (location = 0) in vec4 in_pos_uv;       // pos.xy, uv.zw
layout (location = 1) in int in_n;
layout (location = 2) in vec4 in_ab;
layout (location = 3) in vec4 in_cd;
layout (location = 4) in vec4 in_ef;
layout (location = 5) in vec4 in_gh;
layout (location = 6) in vec4 in_col;
layout (location = 7) in vec4 in_shape;         // radius, stroke, aa, type
layout (location = 8) in vec4 in_blend_posH;    // alpha, fill, posH.xy
layout (location = 9) in vec4 in_user;
layout (location = 10) in vec4 in_uv_bounds;    // uv_min.xy, uv_max.zw

layout (location = 0) out vec4 v_pos_uv;
layout (location = 1) out int v_n;
layout (location = 2) out vec4 v_ab;
layout (location = 3) out vec4 v_cd;
layout (location = 4) out vec4 v_ef;
layout (location = 5) out vec4 v_gh;
layout (location = 6) out vec4 v_col;
layout (location = 7) out vec4 v_shape;
layout (location = 8) out vec4 v_blend_posH;
layout (location = 9) out vec4 v_user;
layout (location = 10) out vec4 v_uv_bounds;

void main()
{
	v_pos_uv = in_pos_uv;
	v_n = in_n;
	v_ab = in_ab;
	v_cd = in_cd;
	v_ef = in_ef;
	v_gh = in_gh;
	v_col = in_col;
	v_shape = vec4(in_shape.x, in_shape.y * 0.5, in_shape.zw);
	v_blend_posH = in_blend_posH;
	v_user = in_user;
	v_uv_bounds = in_uv_bounds;

	gl_Position = vec4(in_blend_posH.zw, 0, 1);
}
)";

static const char* s_draw_fs = R"(
layout (location = 0) in vec4 v_pos_uv;
layout (location = 1) in flat int v_n;
layout (location = 2) in vec4 v_ab;
layout (location = 3) in vec4 v_cd;
layout (location = 4) in vec4 v_ef;
layout (location = 5) in vec4 v_gh;
layout (location = 6) in vec4 v_col;
layout (location = 7) in vec4 v_shape;
layout (location = 8) in vec4 v_blend_posH;
layout (location = 9) in vec4 v_user;
layout (location = 10) in vec4 v_uv_bounds;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	int u_alpha_discard;
	int u_use_smooth_uv;
};

// Used only for polygon SDF.
vec2 pts[8];

// Backwards-compat globals.
// Here for convenience for any legacy user draw shaders that reference them.
vec2 v_pos;
vec2 v_uv;
vec2 v_posH;
float v_radius;
float v_stroke;
float v_aa;
float v_type;
float v_alpha;
float v_fill;
vec2 pos;
vec2 screen_uv;

#include "blend.shd"
#include "gamma.shd"
#include "smooth_uv.shd"
#include "distance.shd"

struct ShaderParams
{
	vec2 view_pos;
	vec2 uv;
	vec2 uv_min;
	vec2 uv_max;
	vec2 screen_uv;
	vec4 attributes;
};

#include "shader_stub.shd"

void main()
{
	// Unpack into backwards-compat globals.
	v_pos    = v_pos_uv.xy;
	v_uv     = v_pos_uv.zw;
	v_radius = v_shape.x;
	v_stroke = v_shape.y;
	v_aa     = v_shape.z;
	v_type   = v_shape.w;
	v_alpha  = v_blend_posH.x;
	v_fill   = v_blend_posH.y;
	v_posH   = v_blend_posH.zw;

	bool is_sprite  = v_type >= 0.0 && v_type < 0.5;
	bool is_text    = v_type >  0.5 && v_type < 1.5;
	bool is_box     = v_type >  1.5 && v_type < 2.5;
	bool is_seg     = v_type >  2.5 && v_type < 3.5;
	bool is_tri     = v_type >  3.5 && v_type < 4.5;
	bool is_tri_sdf = v_type >  4.5 && v_type < 5.5;
	bool is_poly    = v_type >  5.5 && v_type < 6.5;

	// Traditional sprite/text/tri cases.
	vec4 c = vec4(0);
	vec2 uv = u_use_smooth_uv == 0 ? smooth_uv(v_uv, u_texture_size) : v_uv;
	vec4 tex_c = de_gamma(texture(u_image, uv));
	c = is_sprite ? gamma(tex_c) : c;
	c = is_text ? v_col * tex_c.a : c;
	c = is_tri ? v_col : c;

	// SDF cases.
	float d = 0;
	if (is_box) {
		d = distance_box(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_seg) {
		d = distance_segment(v_pos, v_ab.xy, v_ab.zw);
		d = min(d, distance_segment(v_pos, v_ab.zw, v_cd.xy));
	} else if (is_tri_sdf) {
		d = distance_triangle(v_pos, v_ab.xy, v_ab.zw, v_cd.xy);
	} else if (is_poly) {
		pts[0] = v_ab.xy;
		pts[1] = v_ab.zw;
		pts[2] = v_cd.xy;
		pts[3] = v_cd.zw;
		pts[4] = v_ef.xy;
		pts[5] = v_ef.zw;
		pts[6] = v_gh.xy;
		pts[7] = v_gh.zw;
		d = distance_polygon(v_pos, pts, v_n);
	}
	c = (!is_sprite && !is_text && !is_tri) ? sdf(c, v_col, d - v_radius) : c;

	c *= v_alpha;
	pos = v_pos;
	screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	ShaderParams sp;
	sp.view_pos = pos;
	sp.uv = v_uv;
	sp.uv_min = v_uv_bounds.xy;
	sp.uv_max = v_uv_bounds.zw;
	sp.screen_uv = screen_uv;
	sp.attributes = v_user;
	c = shader(c, sp);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	result = c;
}
)";

//--------------------------------------------------------------------------------------------------
// Primary blit shader.

static const char* s_blit_vs = R"(
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_posH;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec4 in_params;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec2 v_pos;
layout (location = 2) out vec2 v_posH;
layout (location = 3) out vec4 v_params;

void main() {
	v_uv = in_uv;
	v_pos = in_pos;
	v_posH = in_posH;
	v_params = in_params;
	gl_Position = vec4(in_posH, 0, 1);
}
)";

static const char* s_blit_fs = R"(
layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec2 v_pos;
layout (location = 2) in vec2 v_posH;
layout (location = 3) in vec4 v_params;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

layout (set = 3, binding = 0) uniform uniform_block {
	vec2 u_texture_size;
	int u_alpha_discard;
};

#include "smooth_uv.shd"

vec2 pos;
vec2 screen_uv;

struct ShaderParams
{
	vec2 view_pos;
	vec2 uv;
	vec2 uv_min;
	vec2 uv_max;
	vec2 screen_uv;
	vec4 attributes;
};

#include "shader_stub.shd"

void main() {
	vec4 color = texture(u_image, smooth_uv(v_uv, u_texture_size));
	pos = v_pos;
	screen_uv = (v_posH + vec2(1,-1)) * 0.5 * vec2(1,-1);
	ShaderParams sp;
	sp.view_pos = pos;
	sp.uv = v_uv;
	sp.uv_min = vec2(0);
	sp.uv_max = vec2(0);
	sp.screen_uv = screen_uv;
	sp.attributes = v_params;
	vec4 c = shader(color, sp);
	if (u_alpha_discard != 0 && c.a == 0) discard;
	result = c;
}
)";

typedef struct CF_BuiltinShaderSource {
	const char* name;
	const char* vertex;
	const char* fragment;
} CF_BuiltinShaderSource;

static CF_ShaderCompilerFile s_builtin_includes[] = {
	{ "gamma.shd", s_gamma },
	{ "distance.shd", s_distance },
	{ "smooth_uv.shd", s_smooth_uv },
	{ "blend.shd", s_blend },
};

static CF_BuiltinShaderSource s_builtin_shader_sources[] = {
	{ "s_draw", s_draw_vs, s_draw_fs },
	{ "s_basic", s_basic_vs, s_basic_fs },
	{ "s_backbuffer", s_backbuffer_vs, s_backbuffer_fs },
	{ "s_blit", s_blit_vs, s_blit_fs },
};

#endif
