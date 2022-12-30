@block distance

float safe_div(float a, float b)
{
	return b == 0.0 ? 0.0 : a / b;
}

float safe_len(vec2 v)
{
	float d = dot(v,v);
	return d == 0.0 ? 0.0 : sqrt(d);
}

vec2 safe_norm(vec2 v, float l)
{
	return mix(vec2(0), v / l, l == 0.0 ? 0.0 : 1.0);
}

vec2 skew(vec2 v)
{
	return vec2(-v.y, v.x);
}

float det2(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

//--------------------------------------------------------------------------------------------------

float distance_aabb(vec2 p, vec2 he, float r)
{
	vec2 d = abs(p) - he;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

// Referenced from: https://www.shadertoy.com/view/stcfzn
float distance_box(vec2 p, vec2 a, vec2 b, float thickness, float r)
{
	vec2 d = b - a;
	float l = length(d);
	d = safe_norm(d, l);
	vec2 L = p - (a + b) * 0.5;
	L = mat2(skew(d), d) * L; // This (and prior) can be done in vertex shader.
	return distance_aabb(L, vec2(l * 0.5, thickness), r);
}

// Referenced from: https://www.shadertoy.com/view/3tdSDj
float distance_segment(vec2 p, vec2 a, vec2 b, float r)
{
	vec2 n = b - a;
	vec2 pa = p - a;
	float d = safe_div(dot(pa,n), dot(n,n));
	float h = clamp(d, 0.0, 1.0);
	return safe_len(pa - h * n) - r;
}

// Referenced from: https://www.shadertoy.com/view/XsXSz4
float distance_triangle(vec2 p, vec2 a, vec2 b, vec2 c, float r)
{
	vec2 e0 = b - a;
	vec2 e1 = c - b;
	vec2 e2 = a - c;

	vec2 v0 = p - a;
	vec2 v1 = p - b;
	vec2 v2 = p - c;

	vec2 pq0 = v0 - e0 * clamp(safe_div(dot(v0, e0), dot(e0, e0)), 0.0, 1.0);
	vec2 pq1 = v1 - e1 * clamp(safe_div(dot(v1, e1), dot(e1, e1)), 0.0, 1.0);
	vec2 pq2 = v2 - e2 * clamp(safe_div(dot(v2, e2), dot(e2, e2)), 0.0, 1.0);

	float s = det2(e0, e2);
	vec2 d = min(min(vec2(dot(pq0, pq0), s * det2(v0, e0)),
	                 vec2(dot(pq1, pq1), s * det2(v1, e1))),
	                 vec2(dot(pq2, pq2), s * det2(v2, e2)));

	return -sqrt(d.x) * sign(d.y) - r;
}

@end
