@module sprite
@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@include includes/blend.glsl
@include includes/gamma.glsl
@include includes/smooth_uv.glsl

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;
	layout (location = 2) in vec4 in_col;
	layout (location = 3) in vec4 in_params;

	layout (location = 0) out vec2 v_uv;
	layout (location = 1) out vec4 v_col;
	layout (location = 2) out float v_alpha;
	layout (location = 3) out float v_type;

	void main()
	{
		vec4 posH = vec4(in_pos, 0, 1);
		v_uv = in_uv;
		v_col = in_col;
		v_alpha = in_params.r;
		v_type = in_params.g;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 v_uv;
	layout (location = 1) in vec4 v_col;
	layout (location = 2) in float v_alpha;
	layout (location = 3) in float v_type;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
	};

	@include_block blend
	@include_block gamma
	@include_block smooth_uv

	void main()
	{
		float is_sprite = v_type == (0.0/255.0) ? 1.0 : 0.0;
		float is_shape  = v_type == (1.0/255.0) ? 1.0 : 0.0;
		float is_text   = v_type == (2.0/255.0) ? 1.0 : 0.0;
		vec4 c = de_gamma(texture(u_image, smooth_uv(v_uv, u_texture_size)));
		c.a *= v_alpha;
		c = mix(c, gamma(overlay(c, v_col)), is_sprite);
		c = mix(c, v_col, is_shape);
		c = mix(c, v_col * c.a, is_text);
		if (c.a == 0) discard;
		result = c;
	}
@end

@program shd vs fs
