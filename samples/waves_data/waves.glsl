@module waves

@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@block shader_block
uniform sampler2D water_tex;
uniform sampler2D noise_tex;

uniform shader_uniforms {
	float amplitude;
	float time;
	float show_noise;
};

vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	vec2 noise_uv = screen_uv;
	noise_uv.x *= 640.0 / 128.0;
	noise_uv.y *= 480.0 / 128.0;
	vec4 noise_c = vec4(texture(noise_tex, noise_uv * 0.5 + time).r - 0.5);
	vec2 offset = vec2(1.0/640.0, 1.0/480.0) * noise_c.r * amplitude;
	vec4 c = texture(water_tex, screen_uv + offset);
	c = show_noise > 0 ? noise_c + 0.5 : c;
	return c;
}
@end

@include ../../include/shaders/draw.glsl
