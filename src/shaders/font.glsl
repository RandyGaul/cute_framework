@module font
@ctype mat4 cute::matrix_t
@ctype vec2 cute::v2

@vs vs
@glsl_options flip_vert_y
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;

	layout (location = 0) out vec2 uv;

	layout(binding = 0) uniform vs_params {
		mat4 u_mvp;
	};

	void main()
	{
		vec4 posH = u_mvp * vec4(round(in_pos), 0, 1);
		uv = in_uv;
		gl_Position = posH;
}
@end

@fs fs
	layout (location = 0) in vec2 uv;

	out vec4 result;

	layout(binding = 0) uniform sampler2D u_image;
	layout (location = 0)uniform vec4 u_text_color;
	layout (location = 1)uniform vec4 u_border_color;
	layout (location = 2)uniform vec2 u_texel_size;
	layout (location = 3)uniform float u_use_border;

	void main()
	{
		// Border detection for pixel outlines.
		float a = texture(u_image, uv + vec2(0,  u_texel_size.y)).r;
		float b = texture(u_image, uv + vec2(0, -u_texel_size.y)).r;
		float c = texture(u_image, uv + vec2(u_texel_size.x,  0)).r;
		float d = texture(u_image, uv + vec2(-u_texel_size.x, 0)).r;
		float e = texture(u_image, uv + vec2(-u_texel_size.x, -u_texel_size.y)).r;
		float f = texture(u_image, uv + vec2(-u_texel_size.x,  u_texel_size.y)).r;
		float g = texture(u_image, uv + vec2( u_texel_size.x, -u_texel_size.y)).r;
		float h = texture(u_image, uv + vec2( u_texel_size.x,  u_texel_size.y)).r;
		float i = texture(u_image, uv).r;
		float border = max(a, max(b, max(c, max(d, max(e, max(f, max(g, h))))))) * (1 - i);
		border *= u_use_border;

		// Pick black for font and white for border.
		vec4 border_color = u_border_color;
		vec4 text_color = u_text_color * i;
		vec4 result = mix(text_color, border_color, border);
	}
@end

@program font vs fs
