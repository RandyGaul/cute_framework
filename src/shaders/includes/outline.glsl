@block outline
	vec2 texel(vec2 t)
	{
		return vec2(clamp(t.x, 0.0, 1.0), clamp(t.y, 0.0, 1.0));
	}

	float sides(sampler2D img, vec2 texel_size, float image_mask)
	{
		float a = texture(img, texel(uv + vec2( 0,  texel_size.y))).a;
		float b = texture(img, texel(uv + vec2( 0, -texel_size.y))).a;
		float c = texture(img, texel(uv + vec2( texel_size.x,  0))).a;
		float d = texture(img, texel(uv + vec2(-texel_size.x,  0))).a;
		return float(max(a, max(b, max(c, d))) != 0) * 1 - image_mask;
	}

	float corners(sampler2D img, vec2 texel_size, float image_mask)
	{
		float e = texture(img, texel(uv + vec2(-texel_size.x, -texel_size.y))).a;
		float f = texture(img, texel(uv + vec2(-texel_size.x,  texel_size.y))).a;
		float g = texture(img, texel(uv + vec2( texel_size.x, -texel_size.y))).a;
		float h = texture(img, texel(uv + vec2( texel_size.x,  texel_size.y))).a;
		return float(max(e, max(f, max(g, h))) != 0) * 1 - image_mask;
	}

	float outline(sampler2D img, vec2 texel_size, float image_mask, float use_border, float use_corner)
	{
		float side = sides(img, texel_size, image_mask) * use_border;
		float corner = corners(img, texel_size, image_mask) * use_border * use_corner;
		return side + corner;
	}
@end
