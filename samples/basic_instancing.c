#include <cute.h>

typedef struct Vertex
{
	CF_V2 position;
	CF_Pixel color;
} Vertex;

typedef struct InstanceData
{
	CF_V2 offset;
} InstanceData;

const char* s_tri_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_col;\n"
"layout (location = 2) in vec2 in_offset;\n"
"\n"
"layout (location = 0) out vec4 v_col;\n"
"\n"
"void main()\n"
"{\n"
"    vec2 pos = in_pos + in_offset;\n"
"    v_col = in_col;\n"
"    gl_Position = vec4(pos, 0, 1);\n"
"}\n";

const char* s_tri_fs =
"layout(location = 0) in vec4 v_col;\n"
"layout(location = 0) out vec4 result;\n"
"\n"
"void main()\n"
"{\n"
"    result = v_col;\n"
"}\n";


int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Instancing", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Vertex data for a single triangle.
	Vertex verts[3];
	verts[0].position = cf_v2(-0.1f, -0.1f);
	verts[0].color = cf_pixel_red();
	verts[1].position = cf_v2( 0.1f, -0.1f);
	verts[1].color = cf_pixel_blue();
	verts[2].position = cf_v2( 0.0f,  0.1f);
	verts[2].color = cf_pixel_green();

	CF_VertexAttribute attrs[3] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, position);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_UBYTE4_NORM;
	attrs[1].offset = CF_OFFSET_OF(Vertex, color);
	attrs[2].name = "in_offset";
	attrs[2].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[2].offset = CF_OFFSET_OF(InstanceData, offset);
	attrs[2].per_instance = true;

	CF_Mesh mesh = cf_make_mesh(sizeof(Vertex) * 3, attrs, 3, sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 3);

	// Instance data: place triangles in a grid.
	InstanceData instances[100];
	int idx = 0;
	for (int y = 0; y < 10; ++y) {
		for (int x = 0; x < 10; ++x) {
			instances[idx++].offset = cf_v2(-0.9f + x * 0.2f, -0.9f + y * 0.2f);
		}
	}

	cf_mesh_set_instance_buffer(mesh, sizeof(instances), sizeof(InstanceData));
	cf_mesh_update_instance_data(mesh, instances, 100);

	CF_Material material = cf_make_material();
	CF_Shader shader = cf_make_shader_from_source(s_tri_vs, s_tri_fs);

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
