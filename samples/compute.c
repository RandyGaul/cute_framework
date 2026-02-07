// Compute shader sample.
//
// A compute shader writes an animated procedural pattern to a texture each frame.
// A fullscreen quad then displays the texture. If the compute dispatch works, you
// see a colorful moving gradient. If it fails, the screen stays black.

#include <cute.h>

#define TEX_W 256
#define TEX_H 256

#define STR(X) #X

// Compute shader: writes RGBA pixels to a storage texture.
// Each invocation handles one pixel. The pattern is a simple color wash
// based on pixel position and a time uniform.
const char* s_compute = STR(
	layout (set = 1, binding = 0, rgba8) uniform writeonly image2D u_output;

	layout (set = 2, binding = 0) uniform uniform_block {
		float u_time;
	};

	layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

	void main()
	{
		ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
		ivec2 size = imageSize(u_output);
		if (coord.x >= size.x || coord.y >= size.y) return;

		vec2 uv = vec2(coord) / vec2(size);

		// Animated diagonal gradient with shifting hue.
		float t = u_time * 0.5;
		float r = 0.5 + 0.5 * sin(uv.x * 6.28 + t);
		float g = 0.5 + 0.5 * sin(uv.y * 6.28 + t * 1.3 + 2.0);
		float b = 0.5 + 0.5 * sin((uv.x + uv.y) * 3.14 + t * 0.7 + 4.0);

		imageStore(u_output, coord, vec4(r, g, b, 1.0));
	}
);

// Vertex shader: takes position + UV from mesh.
const char* s_vs = STR(
	layout (location = 0) in vec2 in_pos;
	layout (location = 1) in vec2 in_uv;

	layout (location = 0) out vec2 v_uv;

	void main()
	{
		v_uv = in_uv;
		gl_Position = vec4(in_pos, 0.0, 1.0);
	}
);

// Fragment shader: sample the compute output texture.
const char* s_fs = STR(
	layout (location = 0) in vec2 v_uv;
	layout (location = 0) out vec4 result;

	layout (set = 2, binding = 0) uniform sampler2D u_image;

	void main()
	{
		result = texture(u_image, v_uv);
	}
);

typedef struct Vertex
{
	CF_V2 pos;
	CF_V2 uv;
} Vertex;

int main(int argc, char* argv[])
{
	CF_Result result = cf_make_app("Compute Sample", 0, 0, 0, 512, 512, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (cf_is_error(result)) return -1;

	// Create the compute output texture with both sampler and compute storage write usage.
	CF_TextureParams tex_params = cf_texture_defaults(TEX_W, TEX_H);
	tex_params.usage = CF_TEXTURE_USAGE_SAMPLER_BIT | CF_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE_BIT;
	tex_params.filter = CF_FILTER_LINEAR;
	CF_Texture compute_tex = cf_make_texture(tex_params);

	// Create the compute shader.
	CF_ComputeShader compute_shader = cf_make_compute_shader_from_source(s_compute);

	// Create a material for compute uniforms.
	CF_Material compute_material = cf_make_material();

	// Create the graphics shader for display.
	CF_Shader display_shader = cf_make_shader_from_source(s_vs, s_fs);

	// Create a material for the display shader (holds the texture binding).
	CF_Material display_material = cf_make_material();
	cf_material_set_texture_fs(display_material, "u_image", compute_tex);

	// Fullscreen quad as two triangles.
	Vertex verts[6] = {
		{ cf_v2(-1, -1), cf_v2(0, 1) },
		{ cf_v2( 1, -1), cf_v2(1, 1) },
		{ cf_v2( 1,  1), cf_v2(1, 0) },
		{ cf_v2(-1, -1), cf_v2(0, 1) },
		{ cf_v2( 1,  1), cf_v2(1, 0) },
		{ cf_v2(-1,  1), cf_v2(0, 0) },
	};

	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, uv);
	CF_Mesh mesh = cf_make_mesh(sizeof(verts), attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex));
	cf_mesh_update_vertex_data(mesh, verts, 6);

	float time = 0.0f;

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		time += CF_DELTA_TIME;

		// --- Compute pass: write the procedural pattern into compute_tex. ---
		cf_material_set_uniform_cs(compute_material, "u_time", &time, CF_UNIFORM_TYPE_FLOAT, 1);

		CF_ComputeDispatch dispatch = cf_compute_dispatch_defaults(TEX_W / 16, TEX_H / 16, 1);
		dispatch.rw_textures = &compute_tex;
		dispatch.rw_texture_count = 1;
		cf_dispatch_compute(compute_shader, compute_material, dispatch);

		// --- Graphics pass: display the texture as a fullscreen quad. ---
		cf_apply_canvas(cf_app_get_canvas(), true);
		cf_apply_mesh(mesh);
		cf_apply_shader(display_shader, display_material);
		cf_draw_elements();

		cf_app_draw_onto_screen(false);
	}

	cf_destroy_compute_shader(compute_shader);
	cf_destroy_shader(display_shader);
	cf_destroy_material(compute_material);
	cf_destroy_material(display_material);
	cf_destroy_mesh(mesh);
	cf_destroy_texture(compute_tex);
	cf_destroy_app();

	return 0;
}
