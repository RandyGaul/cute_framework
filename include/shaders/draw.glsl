@block blend

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

@end

//--------------------------------------------------------------------------------------------------

@block gamma

vec4 gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);
}

vec4 de_gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);
}

@end

//--------------------------------------------------------------------------------------------------

@block smooth_uv

vec2 smooth_uv(vec2 uv, vec2 texture_size)
{
	vec2 pixel = uv * texture_size;
	vec2 seam = floor(pixel + 0.5);
	pixel = seam + clamp((pixel - seam) / fwidth(pixel), -0.5, 0.5);
	return pixel / texture_size;
}

@end

//--------------------------------------------------------------------------------------------------

@block distance

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

@end

//--------------------------------------------------------------------------------------------------

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_posH;
	layout (location = 2) in vec2 in_a;
	layout (location = 3) in vec2 in_b;
	layout (location = 4) in vec2 in_c;
	layout (location = 5) in vec2 in_uv;
	layout (location = 6) in vec4 in_col;
	layout (location = 7) in float in_radius;
	layout (location = 8) in float in_stroke;
	layout (location = 9) in float in_aa;
	layout (location = 10) in vec4 in_params;
	layout (location = 11) in vec4 in_user_params;

	layout (location = 0) out vec2 v_pos;
	layout (location = 1) out vec2 v_a;
	layout (location = 2) out vec2 v_b;
	layout (location = 3) out vec2 v_c;
	layout (location = 4) out vec2 v_uv;
	layout (location = 5) out vec4 v_col;
	layout (location = 6) out float v_radius;
	layout (location = 7) out float v_stroke;
	layout (location = 8) out float v_aa;
	layout (location = 9) out float v_type;
	layout (location = 10) out float v_alpha;
	layout (location = 11) out float v_fill;
	layout (location = 12) out vec2 v_posH;
	layout (location = 13) out vec4 v_user;

	layout (binding = 0) uniform vs_params {
		vec2 u_cam_pos;
		vec2 u_cam_scale;
		float u_cam_angle;
	};

	void main()
	{
		v_pos = in_pos;
		v_a = in_a;
		v_b = in_b;
		v_c = in_c;
		v_uv = in_uv;
		v_col = in_col;
		v_radius = in_radius;
		v_stroke = in_stroke;
		v_aa = in_aa;
		v_type = in_params.r;
		v_alpha = in_params.g;
		v_fill = in_params.b;
		// = in_params.a;

		vec4 posH = vec4(in_posH, 0, 1);
		gl_Position = posH;
		v_posH = in_posH;
		v_user = in_user_params;
}
@end

@fs fs
	layout (location = 0) in vec2 v_pos;
	layout (location = 1) in vec2 v_a;
	layout (location = 2) in vec2 v_b;
	layout (location = 3) in vec2 v_c;
	layout (location = 4) in vec2 v_uv;
	layout (location = 5) in vec4 v_col;
	layout (location = 6) in float v_radius;
	layout (location = 7) in float v_stroke;
	layout (location = 8) in float v_aa;
	layout (location = 9) in float v_type;
	layout (location = 10) in float v_alpha;
	layout (location = 11) in float v_fill;
	layout (location = 12) in vec2 v_posH;
	layout (location = 13) in vec4 v_user;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
	};

	@include_block blend
	@include_block gamma
	@include_block smooth_uv
	@include_block distance
	@include_block shader_block

	void main()
	{
		bool is_sprite    = v_type >= (0.0/255.0) && v_type < (0.5/255.0);
		bool is_text      = v_type >  (0.5/255.0) && v_type < (1.5/255.0);
		bool is_box       = v_type >  (1.5/255.0) && v_type < (2.5/255.0);
		bool is_seg       = v_type >  (2.5/255.0) && v_type < (3.5/255.0);
		bool is_tri       = v_type >  (3.5/255.0) && v_type < (4.5/255.0);
		bool is_tri_sdf   = v_type >  (4.5/255.0) && v_type < (5.5/255.0);

		// Traditional sprite/text/tri cases.
		vec4 c = vec4(0);
		c = !(is_sprite && is_text) ? de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size))) : c;
		c = is_sprite ? gamma(overlay(c, v_col)) : c;
		c = is_text ? v_col * c.a : c;
		c = is_tri ? v_col : c;

		// SDF cases.
		float d = 0;
		if (is_box) {
			d = distance_box(v_pos, v_a, v_b, v_c);
		} else if (is_seg) {
			d = distance_segment(v_pos, v_a, v_b);
			d = min(d, distance_segment(v_pos, v_b, v_c));
		} else if (is_tri_sdf) {
			d = distance_triangle(v_pos, v_a, v_b, v_c);
		}
		c = (!is_sprite && !is_text && !is_tri) ? sdf(c, v_col, d - v_radius) : c;

		c *= v_alpha;
		vec2 screen_position = (v_posH + vec2(1,1)) * 0.5 * vec2(1,-1);
		c = shader(c, v_pos, v_uv, screen_position, v_user);
		if (c.a == 0) discard;
		result = c;
	}
@end

@program shader vs fs
