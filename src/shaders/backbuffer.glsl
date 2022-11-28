@module backbuffer
@ctype vec2 cf_v2

@include includes/smooth_uv.glsl

@vs vs
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;

	layout (location = 0) out vec2 uv;

	void main() {
		vec4 posH = vec4(in_pos, 0, 1);
		uv = in_uv;
		gl_Position = posH;
	}
@end

@fs fs
	layout (location = 0) in vec2 uv;

	out vec4 result;

	layout (binding = 0) uniform sampler2D u_image;

	layout (binding = 0) uniform fs_params {
		vec2 u_texture_size;
	};

	@include_block smooth_uv

	void main() {
		vec4 color = texture(u_image, smooth_uv(uv, u_texture_size));
		result = color;
	}
@end

@program shd vs fs
