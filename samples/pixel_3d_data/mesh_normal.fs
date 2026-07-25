// The g-buffer pass. Writes the world normal and a linear view depth so the composite pass can
// find silhouettes and interior creases -- edge detection on the colour image alone would trace
// texture detail instead of geometry.

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_view_pos;

layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_albedo;

layout (set = 3, binding = 0) uniform uniform_block {
	vec4 u_params; // x: znear, y: zfar.
};

void main()
{
	// Right-handed view space: the camera looks down -z, so distance in front is -z.
	float depth = (-v_view_pos.z - u_params.x) / max(u_params.y - u_params.x, 0.0001);
	result = vec4(normalize(v_normal), clamp(depth, 0.0, 1.0));
}
