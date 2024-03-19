#include <cute.h>
#include <cimgui.h>
#include <sokol/sokol_gfx_imgui.h>

#include "draw_to_texture_data/blit_shader.h"

typedef struct Offscreen
{
	int w, h;
	CF_Texture backbuffer;
	CF_Texture backbuffer_depth_stencil;
	CF_Canvas canvas;
} Offscreen;

typedef struct Vertex
{
	float x, y;
	float u, v;
} Vertex;

// UV (0,0) is top-left of the screen, while UV (1,1) is bottom right. We flip the y-axis for UVs to make the y-axis point up.
// Coordinate (-1,1) is top left, while (1,-1) is bottom right.
static void s_quad(float x, float y, float sx, float sy, Vertex quad[6])
{
	// Build a quad from (-1.0f,-1.0f) to (1.0f,1.0f).
	quad[0].x = -1.0f; quad[0].y =  1.0f; quad[0].u = 0; quad[0].v = 0;
	quad[1].x =  1.0f; quad[1].y = -1.0f; quad[1].u = 1; quad[1].v = 1;
	quad[2].x =  1.0f; quad[2].y =  1.0f; quad[2].u = 1; quad[2].v = 0;

	quad[3].x = -1.0f; quad[3].y =  1.0f; quad[3].u = 0; quad[3].v = 0;
	quad[4].x = -1.0f; quad[4].y = -1.0f; quad[4].u = 0; quad[4].v = 1;
	quad[5].x =  1.0f; quad[5].y = -1.0f; quad[5].u = 1; quad[5].v = 1;

	// Scale the quad about the origin by (sx,sy), then translate it by (x,y).
	for (int i = 0; i < 6; ++i) {
		quad[i].x = quad[i].x * sx + x;
		quad[i].y = quad[i].y * sy + y;
	}
}

Offscreen make_offscreen(int w, int h)
{
	Offscreen offscreen;
	offscreen.w = w;
	offscreen.h = h;

	CF_TextureParams backbuffer_params = cf_texture_defaults();
	backbuffer_params.width = w;
	backbuffer_params.height = h;
	backbuffer_params.render_target = true;
	offscreen.backbuffer = cf_make_texture(backbuffer_params);

	CF_TextureParams backbuffer_depth_stencil_params = cf_texture_defaults();
	backbuffer_depth_stencil_params.width = w;
	backbuffer_depth_stencil_params.height = h;
	backbuffer_depth_stencil_params.render_target = true;
	backbuffer_depth_stencil_params.pixel_format = CF_PIXELFORMAT_DEPTH_STENCIL;
	offscreen.backbuffer_depth_stencil = cf_make_texture(backbuffer_depth_stencil_params);

	CF_CanvasParams params = cf_canvas_defaults();
	params.target = offscreen.backbuffer;
	params.depth_stencil_target = offscreen.backbuffer_depth_stencil;
	offscreen.canvas = cf_make_canvas(params);

	return offscreen;
}

void destroy_offscreen(Offscreen offscreen)
{
	cf_destroy_texture(offscreen.backbuffer);
	cf_destroy_texture(offscreen.backbuffer_depth_stencil);
	cf_destroy_canvas(offscreen.canvas);
}

int main(int argc, char* argv[])
{
	float w = 640.0f;
	float h = 480.0f;
	int options = CF_APP_OPTIONS_DEFAULT_GFX_CONTEXT | CF_APP_OPTIONS_WINDOW_POS_CENTERED | CF_APP_OPTIONS_RESIZABLE;
	CF_Result result = cf_make_app("Draw to Texture", 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) return -1;

	cf_app_init_imgui(false);
	sg_imgui_t* sg_imgui = cf_app_get_sokol_imgui();

	// Create an offscreen canvas.
	Offscreen offscreen = make_offscreen((int)(w*0.5f), (int)(h*0.5f));

	// Create a quad for left view.
	CF_Mesh left_quad = cf_make_mesh(CF_USAGE_TYPE_IMMUTABLE, sizeof(Vertex) * 6, 0, 0);
	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, x);
	attrs[1].name = "in_uv";
	attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[1].offset = CF_OFFSET_OF(Vertex, u);
	cf_mesh_set_attributes(left_quad, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
	Vertex left_quad_verts[6];
	s_quad(-0.5f, 0, 0.5f, 0.5f, left_quad_verts);
	cf_mesh_update_vertex_data(left_quad, left_quad_verts, 6);

	// Create a quad for the right view.
	CF_Mesh right_quad = cf_make_mesh(CF_USAGE_TYPE_IMMUTABLE, sizeof(Vertex) * 6, 0, 0);
	cf_mesh_set_attributes(right_quad, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
	Vertex right_quad_verts[6];
	s_quad(0.5f, 0, 0.5f, 0.5f, right_quad_verts);
	cf_mesh_update_vertex_data(right_quad, right_quad_verts, 6);

	// Create a quad for the full screen.
	CF_Mesh fullscreen_quad = cf_make_mesh(CF_USAGE_TYPE_IMMUTABLE, sizeof(Vertex) * 6, 0, 0);
	cf_mesh_set_attributes(fullscreen_quad, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
	Vertex fullscreen_quad_verts[6];
	s_quad(0, 0, 1, 1, fullscreen_quad_verts);
	cf_mesh_update_vertex_data(fullscreen_quad, fullscreen_quad_verts, 6);

	// Setup shader + material for drawing the offscreen canvas onto the screen (blit).
	CF_Material blit_material = cf_make_material();
	cf_material_set_texture_fs(blit_material, "u_image", offscreen.backbuffer);
	CF_Shader blit_shader = CF_MAKE_SOKOL_SHADER(blit_shader);

	while (cf_app_is_running())
	{
		cf_app_update(NULL);

		// Draw offscreen.
		cf_draw_circle2(cf_v2(0,0), 100, 5);
		cf_render_to(offscreen.canvas, true);

		// Fetch this each frame, as it's invalidated during window-resizing.
		CF_Canvas app_canvas = cf_app_get_canvas();

		// Draw our offscreen texture onto the app's canvas.
		cf_apply_canvas(app_canvas, false);
		{
			// Draw offscreen texture onto the app's canvas on the left.
			cf_apply_mesh(left_quad);
			cf_apply_shader(blit_shader, blit_material);
			cf_draw_elements();

			// Also draw the offscreen texture onto the app's canvas on the right.
			cf_apply_mesh(right_quad);
			cf_apply_shader(blit_shader, blit_material);
			cf_draw_elements();
		}

		// Show debug views of graphics primitives.
		if (igBeginMainMenuBar()) {
			if (igBeginMenu("sokol-gfx", true)) {
				igMenuItem_BoolPtr("Buffers", NULL, &sg_imgui->buffers.open, true);
				igMenuItem_BoolPtr("Images", NULL, &sg_imgui->images.open, true);
				igMenuItem_BoolPtr("Shaders", NULL, &sg_imgui->shaders.open, true);
				igMenuItem_BoolPtr("Pipelines", NULL, &sg_imgui->pipelines.open, true);
				igMenuItem_BoolPtr("Passes", NULL, &sg_imgui->passes.open, true);
				igMenuItem_BoolPtr("Calls", NULL, &sg_imgui->capture.open, true);
				igEndMenu();
			}
			igEndMainMenuBar();
		}

		// Send the app's canvas to the screen.
		cf_app_draw_onto_screen(false);
	}

	cf_destroy_shader(blit_shader);
	cf_destroy_material(blit_material);
	cf_destroy_mesh(left_quad);
	cf_destroy_mesh(right_quad);
	cf_destroy_mesh(fullscreen_quad);
	destroy_offscreen(offscreen);
	cf_destroy_app();

	return 0;
}
