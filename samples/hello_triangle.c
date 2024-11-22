#include <cute.h>

#ifndef CF_RUNTIME_SHADER_COMPILATION
#include "hello_triangle_data/tri_fs.h"
#include "hello_triangle_data/tri_vs.h"
#endif

typedef struct Vertex
{
	CF_V2 position;

	// It's possible to use CF_Color if you want to use four floats to store color instead of 4 bytes with CF_Pixel.
	// Make sure to swap `CF_VERTEX_FORMAT_UBYTE4N` to `CF_VERTEX_FORMAT_FLOAT4`, and `cf_pixel_red` -> `cf_color_red`, etc.
	CF_Pixel color;
} Vertex;

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Hello Triangle", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Example program to get going with custom rendering. Most of the time you want to draw sprites or shapes,
	// so CF's draw API (see cute_draw.h) is best. But, if you're looking for low-level graphics access to call
	// almost directly into the underlying driver this is the place. Setup your own meshes, materials, and shaders.
	// Bind them altogether and construct a render pass.

	// Setup triangle mesh covering most of the screen.
	Vertex verts[3];
	verts[0].position = cf_v2(-1,-1);
	verts[0].color = cf_pixel_red();
	verts[1].position = cf_v2(1,-1);
	verts[1].color = cf_pixel_blue();
	verts[2].position = cf_v2(0,1);
	verts[2].color = cf_pixel_green();
	
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, position);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_UBYTE4_NORM;
	attrs[1].offset = CF_OFFSET_OF(Vertex, color);
	CF_Mesh mesh = cf_make_mesh(sizeof(Vertex) * 3, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 3);

	// Create material and shader. The material could potentially hold render state, uniforms, or texture bindings. For this
	// example none of these features are used. The shader simply interpolates color based on colors providing as vertex attributes.
	// Therefor, the material is just empty in this case.
	CF_Material material = cf_make_material();
#ifdef CF_RUNTIME_SHADER_COMPILATION
	char* tri_vs = cf_fs_read_entire_file_to_memory_and_nul_terminate("/hello_triangle_data/tri_vs.shd", NULL);
	char* tri_fs = cf_fs_read_entire_file_to_memory_and_nul_terminate("/hello_triangle_data/tri_fs.shd", NULL);
	CF_Shader shader = cf_make_shader_from_source(tri_vs, tri_fs);
	cf_free(tri_vs);
	cf_free(tri_fs);
#else
	CF_Shader shader = cf_make_shader_from_bytecode(s_tri_vs_bytecode, s_tri_fs_bytecode);
#endif

	while (cf_app_is_running()) {
		cf_app_update(NULL);

		// Whenever we draw with low-level graphics a canvas must be selected to draw upon.
		// Conveniently we can just draw to the app's backbuffer directly.
		CF_Canvas app_canvas = cf_app_get_canvas();
		cf_apply_canvas(app_canvas, true);

		// Draw the triangle.
		cf_apply_mesh(mesh);
		cf_apply_shader(shader, material);
		cf_draw_elements();
		cf_commit();

		cf_app_draw_onto_screen(false);
	}

	cf_destroy_shader(shader);
	cf_destroy_material(material);
	cf_destroy_mesh(mesh);
	cf_destroy_app();

	return 0;
}
