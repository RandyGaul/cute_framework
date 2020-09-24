@block overlay
	vec4 overlay(vec4 base, vec4 blend)
	{
		float opacity = blend.a;
		vec3 rgb = 2 * base.rgb * blend.rgb * opacity + base.rgb * (1.0 - opacity);
		return vec4(rgb, base.a);
	}
@end
