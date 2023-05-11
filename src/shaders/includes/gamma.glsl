@block gamma

vec4 gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(1.0/2.2)), c.a);
}

vec4 de_gamma(vec4 c)
{
	return vec4(pow(abs(c.rgb), vec3(2.2)), c.a);
}

@end
