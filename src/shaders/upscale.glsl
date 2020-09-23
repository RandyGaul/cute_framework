@module upscale
@ctype vec2 cute::v2

@vs vs
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;

	layout (location = 0) out vec2 uv;

	layout(binding = 0) uniform vs_params {
		vec2 u_scale;
	};

	void main()
	{
		vec2 pos = in_pos;
		pos.x *= u_scale.x;
		pos.y *= u_scale.y;
		vec4 posH = vec4(round(pos), 0, 1);
		uv = in_uv;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 uv;

	out vec4 result;

	layout(binding = 0) uniform sampler2D u_image;

	void main()
	{
		vec4 color = texture(u_image, uv);
		result = color;
	}
@end

@program shd vs fs
