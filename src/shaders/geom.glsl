@module geom
@ctype mat4 cute::matrix_t
@ctype vec4 cute::color_t
@ctype vec2 cute::v2

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec4 in_col;

	layout (location = 0) out vec4 col;

	layout (binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(round(in_pos), 0, 1);
		col = in_col;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec4 col;

	out vec4 result;

	void main()
	{
		result = col;
	}
@end

@program shd vs fs
