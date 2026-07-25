layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_view_pos;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_albedo;

layout (set = 3, binding = 0) uniform uniform_block {
	vec4 u_light_direction; // xyz: direction light travels.
	vec4 u_light_color;
	vec4 u_ambient;
	vec4 u_params;          // x: number of shading bands.
};

void main()
{
	vec4 albedo = texture(u_albedo, v_uv);

	// Quantize the Lambert term into a few flat bands. This, rather than the low-resolution
	// canvas, is what makes the shading read as pixel art -- a smooth gradient downsampled to
	// 320x240 still looks like a smooth gradient, just blockier.
	float ndl = max(dot(normalize(v_normal), -u_light_direction.xyz), 0.0);
	float bands = max(u_params.x, 1.0);
	float toon = floor(ndl * bands) / bands;

	// Nudge the darkest band up a little so unlit faces keep some of their albedo.
	toon = mix(0.25, 1.0, toon);

	vec3 lit = albedo.rgb * (u_ambient.rgb + u_light_color.rgb * toon);
	result = vec4(lit, albedo.a);
}
