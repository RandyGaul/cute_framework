@module sprite_default
@ctype mat4 cute::matrix_t
@ctype vec4 cute::color_t
@ctype vec2 cute::v2

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;
	layout (location = 2) in float in_alpha;

	layout (location = 0) out vec2 uv;
	layout (location = 1) out float alpha;

	layout (binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(round(in_pos), 0, 1);
		uv = in_uv;
		alpha = in_alpha;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 uv;
	layout (location = 1) in float alpha;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec4 u_tint;
	};

	vec4 overlay(vec4 base, vec4 blend)
	{
		float opacity = blend.a;
		vec3 rgb = 2 * base.rgb * blend.rgb * opacity + base.rgb * (1.0 - opacity);
		return vec4(rgb, base.a);
	}

	void main()
	{
		vec4 color = texture(u_image, uv);
		color = overlay(color, u_tint);
		color.a = color.a * alpha;
		if (color.a < 0.00001) discard;
		result = color;
	}
@end

@program shd vs fs
