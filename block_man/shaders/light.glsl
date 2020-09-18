@module light
@ctype mat4 cute::matrix_t
@ctype vec4 cute::color_t
@ctype vec2 cute::v2

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;

	layout(binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(round(in_pos), 0, 1);
		gl_Position = posH;
}
@end

@fs fs
	out vec4 result;

	layout(binding = 0) uniform fs_params {
		vec4 u_color;
	};

	void main()
	{
		vec4 color = u_color;
		result = color;
	}
@end

@program shd vs fs
