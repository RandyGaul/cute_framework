// Shared vertex stage for all three geometry passes: the lit colour pass, the normal/depth
// g-buffer, and the shadow maps. It follows the shader contract in cute_draw3d.h -- per-mesh
// transforms arrive as instance-rate attributes instead of uniforms, so every pass submits
// through cf_draw3d_mesh and the per-object matrix plumbing disappears. Which camera this
// stage sees is just whatever projection/view is pushed when the mesh is submitted: the
// shadow passes push the light's, the colour and g-buffer passes push the orbit camera's.

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

layout (location = 8)  in vec4 in_model0;
layout (location = 9)  in vec4 in_model1;
layout (location = 10) in vec4 in_model2;
layout (location = 12) in vec4 in_nmat0;
layout (location = 13) in vec4 in_nmat1;
layout (location = 14) in vec4 in_nmat2;

layout (location = 0) out vec3 v_normal;      // World space, for lighting.
layout (location = 1) out vec3 v_view_normal; // View space, for the g-buffer.
layout (location = 2) out vec2 v_uv;
layout (location = 3) out vec3 v_world_pos;
layout (location = 4) out vec4 v_light_pos;
layout (location = 5) out vec4 v_spot_pos;

layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_view_projection;
	mat4 u_view;
	mat4 u_light_vp;
	mat4 u_spot_vp;
};

void main()
{
	vec4 p = vec4(in_pos, 1.0);
	vec3 world = vec3(dot(in_model0, p), dot(in_model1, p), dot(in_model2, p));
	vec3 n = normalize(vec3(dot(in_nmat0.xyz, in_normal),
	                        dot(in_nmat1.xyz, in_normal),
	                        dot(in_nmat2.xyz, in_normal)));
	v_normal = n;
	// three.js's normal buffer is view-space (MeshNormalMaterial), and the edge shader's
	// vec3(1,1,1) bias is written against that -- world normals would make which crease
	// brightens depend on world orientation, so highlights swim as the camera orbits. A w of
	// 0 drops the view translation, leaving exactly the rotation normals want.
	v_view_normal = normalize((u_view * vec4(n, 0.0)).xyz);
	v_uv = in_uv;
	v_world_pos = world;
	v_light_pos = u_light_vp * vec4(world, 1.0);
	v_spot_pos = u_spot_vp * vec4(world, 1.0);
	gl_Position = u_view_projection * vec4(world, 1.0);
}
