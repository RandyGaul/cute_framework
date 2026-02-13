#include <cute.h>

typedef struct Vertex
{
	CF_V2 position;
	CF_Pixel color;
} Vertex;

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "basic_indexed_rendering_data/basic_indexed_rendering_vs_shd.h"
#include "basic_indexed_rendering_data/basic_indexed_rendering_fs_shd.h"
#endif

void mount_content_directory_as(const char* dir)
{
	const char* path = cf_fs_get_base_directory();
	path = cf_path_normalize(path);
	path = cf_string_append(path, "/basic_indexed_rendering_data");
	cf_fs_mount(path, dir, false);
	cf_string_free(path);
}

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Indexed Mesh", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;
	mount_content_directory_as("/");
	cf_shader_directory("/");

	// Define a square with two triangles (4 vertices, 6 indices).
	Vertex verts[4];
	verts[0].position = cf_v2(-0.5f, -0.5f); verts[0].color = cf_pixel_red();
	verts[1].position = cf_v2( 0.5f, -0.5f); verts[1].color = cf_pixel_green();
	verts[2].position = cf_v2( 0.5f,  0.5f); verts[2].color = cf_pixel_blue();
	verts[3].position = cf_v2(-0.5f,  0.5f); verts[3].color = cf_pixel_yellow();

	uint16_t indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, position);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_UBYTE4_NORM;
	attrs[1].offset = CF_OFFSET_OF(Vertex, color);

	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 4);
	cf_mesh_set_index_buffer(mesh, sizeof(indices), 16);
	cf_mesh_update_index_data(mesh, indices, 6);

	CF_Material material = cf_make_material();
#ifdef CF_RUNTIME_SHADER_COMPILATION
	CF_Shader shader = cf_make_shader("basic_indexed_rendering_vs.shd", "basic_indexed_rendering_fs.shd");
#else
	CF_Shader shader = cf_make_shader_from_bytecode(s_basic_indexed_rendering_vs_bytecode, s_basic_indexed_rendering_fs_bytecode);
#endif

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_apply_canvas(cf_app_get_canvas(), true);
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
		cf_app_draw_onto_screen(false);
	}

	cf_destroy_shader(shader);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_app();
	return 0;
}
