@module default
@ctype mat4 cute::matrix_t
@ctype vec2 cute::v2

@vs vs
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;
	layout (location = 2) in float in_alpha;

	layout (location = 0) out vec2 uv;
	layout (location = 1) out float alpha;

	layout(binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		uv = in_uv;
		alpha = in_alpha;
		gl_Position = u_mvp * vec4(ceil(in_pos), 0, 1);
}
@end

@fs fs
	layout (location = 0) in vec2 uv;
	layout (location = 1) in float alpha;

	out vec4 result;

	layout(binding = 0) uniform sampler2D u_image;

	void main()
	{
		vec4 color = texture(u_image, uv);
		color.a = color.a * alpha;
		if (color.a < 0.00001) discard;
		result = color;
	}
@end

@program sprite vs fs