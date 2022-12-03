@module sprite
@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@include includes/overlay.glsl
@include includes/smooth_uv.glsl

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;
	layout (location = 2) in vec4 in_col;
	layout (location = 3) in vec4 in_params;

	layout (location = 0) out vec2 v_uv;
	layout (location = 1) out vec4 v_col;
	layout (location = 2) out float v_solid;
	layout (location = 3) out float v_alpha;

	layout (binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(in_pos, 0, 1);
		v_uv = in_uv;
		v_col = in_col;
		v_solid = in_params.r;
		v_alpha = in_params.b;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 v_uv;
	layout (location = 1) in vec4 v_col;
	layout (location = 2) in float v_solid;
	layout (location = 3) in float v_alpha;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
		vec4 u_tint;
	};

	@include_block overlay
	@include_block smooth_uv

	void main()
	{
		vec4 color = texture(u_image, smooth_uv(v_uv, u_texture_size));
		color = overlay(color, u_tint);
		color = mix(color, v_col, v_solid);
		color.a = color.a * v_alpha;
		if (color.a == 0) discard;
		result = color;
	}
@end

@program shd vs fs
