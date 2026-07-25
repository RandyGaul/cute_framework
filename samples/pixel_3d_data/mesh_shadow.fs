// Shadow map pass. Renders the scene from the light's point of view and writes clip-space depth
// into a colour target.
//
// Depth goes to a colour target rather than being read back out of the depth buffer because
// cf_canvas_get_depth_stencil_target is not sampleable on every backend -- it returns a null
// handle on GLES, where depth lives in a renderbuffer. A float colour target works everywhere.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_world_pos;
layout (location = 3) in vec4 v_light_pos;

layout (location = 0) out vec4 result;

void main()
{
	// The light uses an orthographic projection, so gl_FragCoord.z is already linear in [0, 1].
	result = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
