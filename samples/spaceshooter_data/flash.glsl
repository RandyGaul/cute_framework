@module flash

@ctype mat4 CF_Matrix4x4
@ctype vec4 CF_Color
@ctype vec2 CF_V2

@block shader_block
vec4 shader(vec4 color, vec2 pos, vec2 atlas_uv, vec2 screen_uv, vec4 params)
{
	return vec4(mix(color.rgb, params.rgb, params.a), color.a);
}
@end

@include ../../include/shaders/draw.glsl
