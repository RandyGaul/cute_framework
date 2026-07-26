// The colour pass: Phong shading with a shadow-mapped directional light, a warm spot light, and
// a flat ambient term -- the light rig from three.js's webgl_postprocessing_pixel.
//
// Deliberately NOT toon-quantized. The pixel-art read comes from the low-resolution buffer and
// the edge pass, not from banded shading. Quantizing here actively fights the edge pass, since
// every band boundary becomes another normal edge for it to find.
//
// Shading happens in LINEAR space and is gamma-encoded at the very end, as three.js does. Doing
// it directly in sRGB crushes shadowed ground to black and blows lit surfaces past white -- and
// blown highlights are what turn the edge pass's 1.3x brightening into solid white halos.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_world_pos;
layout (location = 3) in vec4 v_light_pos;
layout (location = 4) in vec4 v_spot_pos;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_albedo;
layout (set = 2, binding = 1) uniform sampler2D u_shadow_map;
layout (set = 2, binding = 2) uniform sampler2D u_spot_shadow_map;

layout (set = 3, binding = 0) uniform uniform_block {
#include "uniform_members.shd"
};

// Blinn-Phong's normalization factor. The light colours arrive pre-divided by PI (the Lambert
// BRDF's 1/PI, folded in on the CPU side), which is right for diffuse but wrong for a specular
// lobe -- undoing it here is what lets a highlight actually reach white and read as a reflection
// rather than a slightly paler patch of surface.
float specular_lobe(vec3 n, vec3 to_light, vec3 view_dir, float shininess)
{
	vec3 halfway = normalize(to_light + view_dir);
	float s = max(shininess, 1.0);
	return ((s + 2.0) / 8.0) * 3.14159265 * pow(max(dot(n, halfway), 0.0), s);
}

// Percentage-closer filtering over a 3x3 kernel. A single tap gives hard, stair-stepped shadow
// edges, which at this resolution read as noise rather than as a shadow.
float pcf(sampler2D map, vec4 light_pos, float bias, float texel)
{
	vec3 proj = light_pos.xyz / light_pos.w;
	// Clip-space xy is [-1,1] with y up, while texture v runs downward.
	vec2 uv = vec2(proj.x * 0.5 + 0.5, 0.5 - proj.y * 0.5);
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0) return 1.0;

	float lit = 0.0;
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			float closest = texture(map, uv + vec2(float(x), float(y)) * texel).r;
			lit += (proj.z - bias) > closest ? 0.0 : 1.0;
		}
	}
	return lit / 9.0;
}

// Slope-scaled bias: a surface nearly edge-on to the light spans far more depth per shadow texel
// and needs much more slack before it stops shadowing itself.
float shadow_factor(vec3 n, vec3 to_light)
{
	if (u_material.z <= 0.0) return 1.0;
	float ndl = max(dot(n, to_light), 0.0);
	return pcf(u_shadow_map, v_light_pos, mix(0.0035, 0.0004, ndl), u_material.z);
}

// The spot's projection is perspective, so its depth is non-linear and needs a looser bias than
// the directional light's.
float spot_shadow_factor(vec3 n, vec3 to_spot)
{
	if (u_material.w <= 0.0) return 1.0;
	float ndl = max(dot(n, to_spot), 0.0);
	return pcf(u_spot_shadow_map, v_spot_pos, mix(0.004, 0.0008, ndl), u_material.w);
}

void spot_contribution(vec3 n, vec3 view_dir, float shininess, out vec3 diffuse, out vec3 specular)
{
	diffuse = vec3(0.0);
	specular = vec3(0.0);
	vec3 to_spot = u_spot_pos.xyz - v_world_pos;
	float dist = length(to_spot);
	if (dist < 0.0001) return;
	vec3 l = to_spot / dist;

	// Cone falloff; u_spot_params.y (penumbra) widens the soft edge inward from the cone angle.
	float cos_angle = dot(-l, normalize(u_spot_dir.xyz));
	float cos_cutoff = u_spot_params.x;
	float cos_inner = mix(1.0, cos_cutoff, 1.0 - u_spot_params.y);
	float cone = smoothstep(cos_cutoff, cos_inner, cos_angle);
	if (cone <= 0.0) return;

	// three.js's getDistanceAttenuation: inverse-square by decay, then a smooth cut so the light
	// reaches exactly zero at its range instead of being clipped mid-falloff.
	float range = u_spot_params.z;
	float atten = 1.0 / max(pow(dist, u_spot_params.w), 0.01);
	float edge = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
	atten *= edge * edge;

	float ndl = max(dot(n, l), 0.0);
	float shadow = spot_shadow_factor(n, l);
	vec3 energy = u_spot_color.rgb * (u_spot_color.a * cone * atten * shadow);

	diffuse = energy * ndl;
	// The spot carries a highlight of its own, which is most of what sells the crystal as shiny:
	// it sits directly in the pool, so this is the light it actually reflects toward the camera.
	specular = energy * u_specular.rgb * specular_lobe(n, l, view_dir, shininess);
}

void main()
{
	vec3 n = normalize(v_normal);
	vec3 to_light = -normalize(u_light_direction.xyz);

	// The checker is authored in sRGB, so bring it into linear before it meets the lighting.
	vec4 tex = texture(u_albedo, v_uv);
	vec3 tex_linear = pow(tex.rgb, vec3(2.2));
	vec3 albedo = mix(vec3(1.0), tex_linear, u_material.x) * pow(u_base_color.rgb, vec3(2.2));

	float ndl = max(dot(n, to_light), 0.0);
	float shadow = shadow_factor(n, to_light);

	// Blinn-Phong specular. Specular is not multiplied by albedo -- a reflection takes the light's
	// colour, not the surface's, which is what makes a white highlight read as shiny.
	vec3 view_dir = normalize(u_eye.xyz - v_world_pos);
	float spec = specular_lobe(n, to_light, view_dir, u_material.y);

	vec3 spot_diffuse, spot_specular;
	spot_contribution(n, view_dir, u_material.y, spot_diffuse, spot_specular);

	vec3 lit = albedo * u_ambient.rgb;
	lit += albedo * u_light_color.rgb * ndl * shadow;
	lit += u_light_color.rgb * u_specular.rgb * spec * shadow;
	lit += albedo * spot_diffuse + spot_specular;
	lit += pow(u_emissive.rgb, vec3(2.2));

	// Linear -> sRGB on the way out. Everything above is linear light.
	result = vec4(pow(max(lit, vec3(0.0)), vec3(1.0 / 2.2)), 1.0);
}
