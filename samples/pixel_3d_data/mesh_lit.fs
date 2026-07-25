// The colour pass: Phong shading with a shadow-mapped directional light, a warm spot light, and
// a flat ambient term -- the light rig from three.js's webgl_postprocessing_pixel.
//
// Deliberately NOT toon-quantized. The pixel-art read comes from the low-resolution buffer and
// the edge pass, not from banded shading. Quantizing here actively fights the edge pass, since
// every band boundary becomes another normal edge for it to find.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_world_pos;
layout (location = 3) in vec4 v_light_pos;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_albedo;
layout (set = 2, binding = 1) uniform sampler2D u_shadow_map;

layout (set = 3, binding = 0) uniform uniform_block {
	vec4 u_light_direction; // xyz: direction the light travels
	vec4 u_light_color;
	vec4 u_ambient;
	vec4 u_spot_pos;        // xyz: world position
	vec4 u_spot_dir;        // xyz: direction the cone points
	vec4 u_spot_color;      // rgb, a: intensity
	vec4 u_spot_params;     // x: cos(angle), y: penumbra, z: range, w: decay
	vec4 u_base_color;      // per-object tint
	vec4 u_emissive;        // per-object emissive
	vec4 u_material;        // x: 1 to sample u_albedo, y: shininess, z: shadow texel size
	vec4 u_eye;             // xyz: camera world position, for the specular term
};

// Percentage-closer filtering over a 3x3 kernel. A single tap gives hard, stair-stepped shadow
// edges, which at this resolution read as noise rather than as a shadow.
float shadow_factor(vec3 n, vec3 to_light)
{
	vec3 proj = v_light_pos.xyz / v_light_pos.w;
	// Clip-space xy is [-1,1] with y up, while texture v runs downward.
	vec2 uv = vec2(proj.x * 0.5 + 0.5, 0.5 - proj.y * 0.5);
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0) return 1.0;

	// Slope-scaled bias: a surface nearly edge-on to the light spans far more depth per shadow
	// texel and needs much more slack before it stops shadowing itself.
	float ndl = max(dot(n, to_light), 0.0);
	float bias = mix(0.0035, 0.0004, ndl);

	float texel = u_material.z;
	float lit = 0.0;
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			float closest = texture(u_shadow_map, uv + vec2(float(x), float(y)) * texel).r;
			lit += (proj.z - bias) > closest ? 0.0 : 1.0;
		}
	}
	return lit / 9.0;
}

vec3 spot_contribution(vec3 n)
{
	vec3 to_spot = u_spot_pos.xyz - v_world_pos;
	float dist = length(to_spot);
	if (dist < 0.0001) return vec3(0.0);
	vec3 l = to_spot / dist;

	// Cone falloff; u_spot_params.y widens the soft edge inward from the cone angle.
	float cos_angle = dot(-l, normalize(u_spot_dir.xyz));
	float cos_cutoff = u_spot_params.x;
	float cos_inner = mix(1.0, cos_cutoff, 1.0 - u_spot_params.y);
	float cone = smoothstep(cos_cutoff, cos_inner, cos_angle);
	if (cone <= 0.0) return vec3(0.0);

	float atten = pow(clamp(1.0 - dist / u_spot_params.z, 0.0, 1.0), u_spot_params.w);
	float ndl = max(dot(n, l), 0.0);
	return u_spot_color.rgb * (u_spot_color.a * cone * atten * ndl);
}

void main()
{
	vec3 n = normalize(v_normal);
	vec3 to_light = -normalize(u_light_direction.xyz);

	vec4 tex = texture(u_albedo, v_uv);
	vec3 albedo = mix(vec3(1.0), tex.rgb, u_material.x) * u_base_color.rgb;

	float ndl = max(dot(n, to_light), 0.0);
	float shadow = shadow_factor(n, to_light);

	// Blinn-Phong specular, applied only where the surface is actually lit.
	vec3 view_dir = normalize(u_eye.xyz - v_world_pos);
	vec3 halfway = normalize(to_light + view_dir);
	float spec = pow(max(dot(n, halfway), 0.0), max(u_material.y, 1.0));

	vec3 lit = albedo * u_ambient.rgb;
	lit += albedo * u_light_color.rgb * ndl * shadow;
	lit += u_light_color.rgb * spec * shadow * 0.25;
	lit += albedo * spot_contribution(n);
	lit += u_emissive.rgb;

	result = vec4(lit, 1.0);
}
