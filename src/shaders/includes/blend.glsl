@block blend

// HSV <-> RGB from : http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// And https://www.shadertoy.com/view/MsS3Wc

vec3 rgb_to_hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
	vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv_to_rgb(vec3 c)
{
	vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
	rgb = rgb*rgb*(3.0-2.0*rgb);
	return c.z * mix(vec3(1.0), rgb, c.y);
}

vec3 hue(vec3 base, vec3 tint)
{
	base = rgb_to_hsv(base);
	tint = rgb_to_hsv(tint);
	return hsv_to_rgb(vec3(tint.r, base.gb));
}

vec4 hue(vec4 base, vec4 tint)
{
	return vec4(hue(base.rgb, tint.rgb), base.a);
}

float overlay(float base, float blend)
{
	return (base <= 0.5) ? 2*base * blend : 1-2*(1-base) * (1-blend);
}

vec3 overlay(vec3 base, vec3 blend)
{
	return vec3(overlay(base.r, blend.r), overlay(base.g, blend.g), overlay(base.b, blend.b));
}

vec4 overlay(vec4 base, vec4 blend)
{
	return vec4(overlay(base.rgb, blend.rgb), base.a);
}

float softlight(float base, float blend)
{
	if (blend <= 0.5) return base - (1-2*blend)*base*(1-base);
	else return base + (2.0 * blend - 1) * (((base <= 0.25) ? ((16.0 * base - 12.0) * base + 4.0) * base : sqrt(base)) - base);
}

vec3 softlight(vec3 base, vec3 blend)
{
	return vec3(softlight(base.r, blend.r), softlight(base.g, blend.g), softlight(base.b, blend.b));
}

vec4 softlight(vec4 base, vec4 blend)
{
	return vec4(softlight(base.rgb, blend.rgb), base.a);
}

@end
