@module sprite
@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 cf_v2

@include includes/overlay.glsl
@include includes/smooth_uv.glsl

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;
	layout (location = 2) in vec4 in_col;
	layout (location = 3) in vec4 in_params;

	layout (location = 0) out vec2 uv;
	layout (location = 1) out float alpha;

	layout (binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(in_pos, 0, 1);
		uv = in_uv;
		alpha = in_params.b;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 uv;
	layout (location = 1) in float alpha;

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
		vec4 color = texture(u_image, smooth_uv(uv, u_texture_size));
		color = overlay(color, u_tint);
		color.a = color.a * alpha;
		if (color.a == 0) discard;
		result = color;
	}
@end

@program shd vs fs
