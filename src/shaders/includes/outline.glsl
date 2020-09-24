@block outline
	float sides(sampler2D img, vec2 texel_size)
	{
		float a = texture(img, uv + vec2( 0,  texel_size.y)).a;
		float b = texture(img, uv + vec2( 0, -texel_size.y)).a;
		float c = texture(img, uv + vec2( texel_size.x,  0)).a;
		float d = texture(img, uv + vec2(-texel_size.x,  0)).a;
		return max(a, max(b, max(c, d)));
	}

	float corners(sampler2D img, vec2 texel_size)
	{
		float e = texture(img, uv + vec2(-texel_size.x, -texel_size.y)).a;
		float f = texture(img, uv + vec2(-texel_size.x,  texel_size.y)).a;
		float g = texture(img, uv + vec2( texel_size.x, -texel_size.y)).a;
		float h = texture(img, uv + vec2( texel_size.x,  texel_size.y)).a;
		return max(e, max(f, max(g, h)));
	}
@end
