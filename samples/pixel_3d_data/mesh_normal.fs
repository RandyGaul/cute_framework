// The g-buffer pass. Writes the world normal and depth so the composite pass can find
// silhouettes and interior creases -- edge detection on the colour image alone would trace the
// checker texture instead of the geometry.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_world_pos;
layout (location = 3) in vec4 v_light_pos;

layout (location = 0) out vec4 result;

void main()
{
	// Normals are encoded to [0,1] and depth goes in alpha, so one colour target carries what
	// three.js splits across its tNormal and tDepth. The composite pass decodes with rgb*2-1,
	// exactly as three.js's getNormal does.
	//
	// The camera is orthographic, so gl_FragCoord.z is already linear in [0,1] -- which is what
	// lets the edge thresholds ported from three.js work unchanged.
	result = vec4(normalize(v_normal) * 0.5 + 0.5, gl_FragCoord.z);
}
