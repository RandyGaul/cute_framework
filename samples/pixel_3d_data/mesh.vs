layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec3 v_view_pos;

layout (set = 1, binding = 0) uniform uniform_block {
	mat4 u_mvp;
	mat4 u_mv;
	mat4 u_normal_matrix;
};

void main()
{
	// The normal matrix is a mat4 because Metal rejects mat3 inside a uniform block. A w of 0
	// drops the translation column, leaving exactly the upper-3x3 transform normals want.
	v_normal = normalize((u_normal_matrix * vec4(in_normal, 0.0)).xyz);
	v_uv = in_uv;
	v_view_pos = (u_mv * vec4(in_pos, 1.0)).xyz;
	gl_Position = u_mvp * vec4(in_pos, 1.0);
}
