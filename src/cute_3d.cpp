/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_3d.h>

#include <internal/cute_alloc_internal.h>

// Binding layout for a low-level (two-file) shader, per docs/topics/shader_compilation.md:
// vertex uniforms at set 1, fragment samplers at set 2, fragment uniforms at set 3, all at
// binding 0. Unlike a draw shader there is no built-in stage occupying binding 0 already. Both
// uniform blocks must be named uniform_block -- that is the only name cf_material_set_uniform_*
// looks up.
static const char* s_lit_vs =
"layout (location = 0) in vec3 in_pos;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_uv;\n"
"\n"
"layout (location = 0) out vec3 v_normal;\n"
"layout (location = 1) out vec2 v_uv;\n"
"\n"
"layout (set = 1, binding = 0) uniform uniform_block {\n"
"    mat4 u_mvp;\n"
"    mat4 u_model;\n"
"    mat4 u_normal_matrix;\n"
"};\n"
"\n"
"void main()\n"
"{\n"
// The normal matrix is a mat4 because Metal rejects mat3 inside a uniform block. Multiplying
// with w = 0 drops the translation column, which is exactly the upper-3x3 transform we want,
// and avoids needing a mat3 constructor at all.
"    v_normal = normalize((u_normal_matrix * vec4(in_normal, 0.0)).xyz);\n"
"    v_uv = in_uv;\n"
"    gl_Position = u_mvp * vec4(in_pos, 1.0);\n"
"}\n";

static const char* s_lit_fs =
"layout (location = 0) in vec3 v_normal;\n"
"layout (location = 1) in vec2 v_uv;\n"
"\n"
"layout (location = 0) out vec4 result;\n"
"\n"
"layout (set = 2, binding = 0) uniform sampler2D u_albedo;\n"
"\n"
"layout (set = 3, binding = 0) uniform uniform_block {\n"
"    vec4 u_light_direction;\n"
"    vec4 u_light_color;\n"
"    vec4 u_ambient;\n"
"};\n"
"\n"
"void main()\n"
"{\n"
"    vec4 albedo = texture(u_albedo, v_uv);\n"
// u_light_direction is the direction light travels, so the vector toward the light is its negation.
"    float ndl = max(dot(normalize(v_normal), -u_light_direction.xyz), 0.0);\n"
"    vec3 lit = albedo.rgb * (u_ambient.rgb + u_light_color.rgb * ndl);\n"
"    result = vec4(lit, albedo.a);\n"
"}\n";

CF_Mesh cf_make_mesh_3d(const CF_Vertex3D* vertices, int vertex_count, const uint32_t* indices, int index_count)
{
	CF_VertexAttribute attrs[3] = { };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[0].offset = CF_OFFSET_OF(CF_Vertex3D, pos);
	attrs[1].name = "in_normal";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT3;
	attrs[1].offset = CF_OFFSET_OF(CF_Vertex3D, normal);
	attrs[2].name = "in_uv";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(CF_Vertex3D, uv);

	CF_Mesh mesh = cf_make_mesh(vertex_count * (int)sizeof(CF_Vertex3D), attrs, 3, (int)sizeof(CF_Vertex3D));
	cf_mesh_update_vertex_data(mesh, (void*)vertices, vertex_count);

	if (indices && index_count > 0) {
		cf_mesh_set_index_buffer(mesh, index_count * (int)sizeof(uint32_t), 32);
		cf_mesh_update_index_data(mesh, (void*)indices, index_count);
	}

	return mesh;
}

CF_Shader cf_make_lit_shader_3d()
{
	return cf_make_shader_from_source(s_lit_vs, s_lit_fs);
}

void cf_lit_shader_3d_set_transform(CF_Material material, CF_M4x4 mvp, CF_M4x4 model)
{
	CF_M4x4 normal_matrix = cf_m4_normal_matrix(model);
	cf_material_set_uniform_vs(material, "u_mvp", &mvp, CF_UNIFORM_TYPE_MAT4, 1);
	cf_material_set_uniform_vs(material, "u_model", &model, CF_UNIFORM_TYPE_MAT4, 1);
	cf_material_set_uniform_vs(material, "u_normal_matrix", &normal_matrix, CF_UNIFORM_TYPE_MAT4, 1);
}

void cf_lit_shader_3d_set_light(CF_Material material, CF_V3 direction, CF_Color color, CF_Color ambient)
{
	CF_V3 d = cf_safe_norm_v3(direction);
	CF_V4 dir4 = cf_v4(d.x, d.y, d.z, 0.0f);
	cf_material_set_uniform_fs(material, "u_light_direction", &dir4, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(material, "u_light_color", &color, CF_UNIFORM_TYPE_FLOAT4, 1);
	cf_material_set_uniform_fs(material, "u_ambient", &ambient, CF_UNIFORM_TYPE_FLOAT4, 1);
}

void cf_lit_shader_3d_set_albedo(CF_Material material, CF_Texture texture)
{
	cf_material_set_texture_fs(material, "u_albedo", texture);
}
