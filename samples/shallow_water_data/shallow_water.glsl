@module shallow_water

@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@block shader_block
uniform sampler2D wavelets_tex;
uniform sampler2D noise_tex;
uniform sampler2D scene_tex;

uniform shader_uniforms {
	float show_normals;
	float show_noise;
};

vec2 normal_from_heightmap(sampler2D tex, vec2 uv)
{
	float ha = textureOffset(tex, uv, ivec2(-1, 1)).r;
	float hb = textureOffset(tex, uv, ivec2( 1, 1)).r;
	float hc = textureOffset(tex, uv, ivec2( 0,-1)).r;
	vec2 n = vec2(ha-hc, hb-hc);
	return n;
}

vec4 normal_to_color(vec2 n)
{
	return vec4(n * 0.5 + 0.5, 1.0, 1.0);
}

vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	vec2 uv = screen_uv;
	vec2 dim = vec2(1.0/160.0,1.0/120.0);
	vec2 n = normal_from_heightmap(noise_tex, uv);
	vec2 w = normal_from_heightmap(wavelets_tex, uv+n*dim*10.0);
	vec4 c = mix(normal_to_color(n), normal_to_color(w), 0.25);
	c = texture(scene_tex, uv+(n+w)*dim*10.0);
	c = mix(c, vec4(1), length(n+w) > 0.2 ? 0.1 : 0.0);

	c = show_normals > 0.0 ? mix(normal_to_color(n), normal_to_color(w), 0.25) : c;

	c = show_noise > 0.0 ? texture(noise_tex, uv) : c;

	return c;
}
@end

@include ../../include/shaders/draw.glsl
