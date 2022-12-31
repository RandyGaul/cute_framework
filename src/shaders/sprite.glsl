@module sprite
@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@include includes/blend.glsl
@include includes/gamma.glsl
@include includes/smooth_uv.glsl
@include includes/distance.glsl

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_a;
	layout (location = 2) in vec2 in_b;
	layout (location = 3) in vec2 in_c;
	layout (location = 4) in vec2 in_d;
	layout (location = 5) in vec2 in_uv;
	layout (location = 6) in vec4 in_col;
	layout (location = 7) in float in_radius;
	layout (location = 8) in float in_stroke;
	layout (location = 9) in vec4 in_params;

	layout (location = 0) out vec2 v_pos;
	layout (location = 1) out vec2 v_a;
	layout (location = 2) out vec2 v_b;
	layout (location = 3) out vec2 v_c;
	layout (location = 4) out vec2 v_d;
	layout (location = 5) out vec2 v_uv;
	layout (location = 6) out vec4 v_col;
	layout (location = 7) out float v_radius;
	layout (location = 8) out float v_stroke;
	layout (location = 9) out float v_type;
	layout (location = 10) out float v_alpha;
	layout (location = 11) out float v_fill;
	layout (location = 12) out float v_aa;

	void main()
	{
		vec2 p = in_pos;
		p.x *= 640.0*0.5f;
		p.y *= 480.0*0.5f;
		v_pos = p;
		v_a = in_a;
		v_b = in_b;
		v_c = in_c;
		v_d = in_d;
		v_uv = in_uv;
		v_col = in_col;
		v_radius = in_radius;
		v_stroke = in_stroke;
		v_type = in_params.r;
		v_alpha = in_params.g;
		v_fill = in_params.b;
		v_aa = in_params.a;
		vec4 posH = vec4(in_pos, 0, 1);
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 v_pos;
	layout (location = 1) in vec2 v_a;
	layout (location = 2) in vec2 v_b;
	layout (location = 3) in vec2 v_c;
	layout (location = 4) in vec2 v_d;
	layout (location = 5) in vec2 v_uv;
	layout (location = 6) in vec4 v_col;
	layout (location = 7) in float v_radius;
	layout (location = 8) in float v_stroke;
	layout (location = 9) in float v_type;
	layout (location = 10) in float v_alpha;
	layout (location = 11) in float v_fill;
	layout (location = 12) in float v_aa;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
	};

	@include_block blend
	@include_block gamma
	@include_block smooth_uv
	@include_block distance

	float sdf_stroke(float d)
	{
		return abs(d) - v_stroke;
	}

	vec4 sdf(vec4 a, vec4 b, float d)
	{
		vec4 stroke_aa = mix(a, b, 1.0 - mix(0.0, 1.0, sdf_stroke(d)));
		vec4 stroke_no_aa = sdf_stroke(d) <= 0.0 ? b : a;

		vec4 fill_aa = mix(a, b, 1.0 - mix(0.0, 1.0, d));
		vec4 fill_no_aa = d <= 0.0 ? b : a;

		vec4 stroke = mix(stroke_no_aa, stroke_aa, v_aa);
		vec4 fill = mix(fill_no_aa, fill_aa, v_aa);

		result = mix(stroke, fill, v_fill);
		return result;
	}

	void main()
	{
		bool is_sprite  = v_type == (0.0/255.0);
		bool is_text    = v_type == (1.0/255.0);
		bool is_box     = v_type == (2.0/255.0);
		bool is_seg     = v_type == (3.0/255.0);
		bool is_seg_beg = v_type == (4.0/255.0);
		bool is_seg_mid = v_type == (5.0/255.0);
		bool is_seg_end = v_type == (6.0/255.0);
		bool is_tri     = v_type == (7.0/255.0);
		bool is_tri_sdf = v_type == (8.0/255.0);

		vec4 c = vec4(0);
		float d = 0;
		if (is_sprite) {
			c = de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size)));
			c = gamma(overlay(c, v_col));
		} else if (is_text) {
			c = de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size)));
			c = v_col * c.a;
		} else if (is_box) {
			d = distance_box(v_pos, v_a, v_b, v_c);
			c = sdf(c, v_col, d - v_radius);
		} else if (is_seg) {
			d = distance_segment(v_pos, v_a, v_b);
			c = sdf(c, v_col, d - v_radius);
		} else if (is_seg_beg) {
		} else if (is_seg_mid) {
		} else if (is_seg_end) {
		} else if (is_tri) {
			d = -1.0;
			c = sdf(c, v_col, d - v_radius);
		} else if (is_tri_sdf) {
			d = distance_triangle(v_pos, v_a, v_b, v_c);
			c = sdf(c, v_col, d - v_radius);
		}
		c.a *= v_alpha;

		if (c.a == 0) discard;
		result = c;
	}
@end

@program shd vs fs
