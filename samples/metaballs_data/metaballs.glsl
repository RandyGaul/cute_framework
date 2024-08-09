@module metaballs

@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@block shader_block
uniform sampler2D tex;

vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	float d = texture(tex, screen_uv).x;
	d = d > 0.5 ? 1.0 : 0.0;
	return vec4(vec3(screen_uv.xy, d), 1);
}
@end

@include ../../include/shaders/draw.glsl
