@module sprite_outline
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
		vec2 u_texel_size;
		float u_use_border;
	};

	vec4 overlay(vec4 base, vec4 blend)
	{
		float opacity = blend.a;
		vec3 rgb = 2 * base.rgb * blend.rgb * opacity + base.rgb * (1.0 - opacity);
		return vec4(rgb, base.a);
	}

	void main()
	{
		// Border detection for pixel outlines.
		float a = texture(u_image, uv + vec2(0,  u_texel_size.y)).a;
		float b = texture(u_image, uv + vec2(0, -u_texel_size.y)).a;
		float c = texture(u_image, uv + vec2( u_texel_size.x,  0)).a;
		float d = texture(u_image, uv + vec2(-u_texel_size.x,  0)).a;
		float e = texture(u_image, uv + vec2(-u_texel_size.x, -u_texel_size.y)).a;
		float f = texture(u_image, uv + vec2(-u_texel_size.x,  u_texel_size.y)).a;
		float g = texture(u_image, uv + vec2( u_texel_size.x, -u_texel_size.y)).a;
		float h = texture(u_image, uv + vec2( u_texel_size.x,  u_texel_size.y)).a;

		vec4 image_color = texture(u_image, uv);
		float image_mask = float(any(notEqual(image_color.rgb, vec3(0.0, 0.0, 0.0))));

		float border = max(a, max(b, max(c, max(d, max(e, max(f, max(g, h))))))) * (1 - image_mask);
		border *= u_use_border;

		// Pick between white border or sprite color.
		vec4 border_color = vec4(1, 1, 1, 1);
		vec4 color = image_color * image_mask + border_color * border;
		color = overlay(color, u_tint);
		color.a = color.a * alpha;
		result = color;
	}
@end

@program shd vs fs
