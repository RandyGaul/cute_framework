// Shadow map pass. Renders the scene from the light's point of view and writes clip-space depth
// into a colour target, sampled manually with PCF in mesh_lit.fs.
//
// This is the other way to build shadow maps in CF -- the draw3d sample uses the alternative:
// a canvas depth target with `compare_enable`, sampled through sampler2DShadow for hardware
// comparisons. A float colour target like this one trades that for full control over the
// filter kernel, and needs no depth-target sampling support at all.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec3 v_view_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in vec3 v_world_pos;
layout (location = 4) in vec4 v_light_pos;
layout (location = 5) in vec4 v_spot_pos;

layout (location = 0) out vec4 result;

void main()
{
	// gl_FragCoord.z is the post-divide depth in [0, 1] either way -- linear for the directional
	// light's orthographic projection, non-linear for the spot's perspective one. The lookup
	// compares against the same quantity, so both work without special-casing.
	result = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
