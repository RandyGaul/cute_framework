@module color_fill
@ctype vec2 CF_V2
@ctype vec4 CF_Pixel

@vs vs
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec4 in_col;

	layout (location = 0) out vec4 col;

	void main() {
		vec4 posH = vec4(in_pos, 0, 1);
		col = in_col;
		gl_Position = posH;
	}
@end

@fs fs
	layout (location = 0) in vec4 col;

	out vec4 result;

	void main() {
		result = col;
	}
@end

@program shader vs fs
