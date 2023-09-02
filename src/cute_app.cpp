/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <cute_app.h>
#include <cute_alloc.h>
#include <cute_audio.h>
#include <cute_multithreading.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>
#include <cute_draw.h>
#include <cute_time.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_dx11.h>
#include <internal/cute_metal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

#include <SDL.h>

#ifdef CF_WINDOWS
#	include <SDL_syswm.h>
#endif

#ifdef CF_WINDOWS
#	pragma comment (lib, "dxgi.lib")
#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "dxguid.lib")
#endif

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

#define SOKOL_IMGUI_NO_SOKOL_APP
#include <internal/imgui/sokol_imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <sokol/sokol_gfx_imgui.h>

#include <shaders/backbuffer_shader.h>

CF_STATIC_ASSERT(sizeof(uint64_t) >= sizeof(void*), "Must be equal for opaque id implementations throughout CF.");

CF_App* app;

using namespace Cute;

struct Vertex
{
	float x, y;
	float u, v;
};

static void s_quad(float x, float y, float sx, float sy, Vertex quad[6])
{
	quad[0].x = -0.5f; quad[0].y =  0.5f; quad[0].u = 0; quad[0].v = 0;
	quad[1].x =  0.5f; quad[1].y = -0.5f; quad[1].u = 1; quad[1].v = 1;
	quad[2].x =  0.5f; quad[2].y =  0.5f; quad[2].u = 1; quad[2].v = 0;

	quad[3].x = -0.5f; quad[3].y =  0.5f; quad[3].u = 0; quad[3].v = 0;
	quad[4].x = -0.5f; quad[4].y = -0.5f; quad[4].u = 0; quad[4].v = 1;
	quad[5].x =  0.5f; quad[5].y = -0.5f; quad[5].u = 1; quad[5].v = 1;

	for (int i = 0; i < 6; ++i) {
		quad[i].x = quad[i].x * sx + x;
		quad[i].y = quad[i].y * sy + y;
	}
}

// Embedded white_pixel
int default_png_sz = 81;
unsigned char default_png_data[81] = {
	0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
	0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x06,0x00,0x00,0x00,0x1f,0x15,0xc4,
	0x89,0x00,0x00,0x00,0x01,0x73,0x52,0x47,0x42,0x00,0xae,0xce,0x1c,0xe9,0x00,0x00,
	0x00,0x0b,0x49,0x44,0x41,0x54,0x08,0x99,0x63,0xf8,0x0f,0x04,0x00,0x09,0xfb,0x03,
	0xfd,0xe3,0x55,0xf2,0x9c,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,
	0x82
};

static void s_canvas(int w, int h)
{
	{
		CF_TextureParams params = cf_texture_defaults();
		params.width = w;
		params.height = h;
		params.render_target = true;
		if (app->backbuffer.id) {
			cf_destroy_texture(app->backbuffer);
		}
		app->backbuffer = cf_make_texture(params);
	}
	{
		CF_TextureParams params = cf_texture_defaults();
		params.width = w;
		params.height = h;
		params.render_target = true;
		params.pixel_format = CF_PIXELFORMAT_DEPTH_STENCIL;
		if (app->backbuffer_depth_stencil.id) {
			cf_destroy_texture(app->backbuffer_depth_stencil);
		}
		app->backbuffer_depth_stencil = cf_make_texture(params);
	}
	{
		CF_CanvasParams params = cf_canvas_defaults();
		params.target = app->backbuffer;
		params.depth_stencil_target = app->backbuffer_depth_stencil;
		if (app->offscreen_canvas.id) {
			cf_destroy_canvas(app->offscreen_canvas);
		}
		app->offscreen_canvas = cf_make_canvas(params);
	}
	{
		CF_CanvasParams params = cf_canvas_defaults();
		if (app->backbuffer_canvas.id) {
			cf_destroy_canvas(app->backbuffer_canvas);
		}
		app->backbuffer_canvas = cf_make_canvas(params);
	}
	app->canvas_w = w;
	app->canvas_h = h;
	cf_material_set_texture_fs(app->backbuffer_material, "u_image", app->backbuffer);
}

CF_Result cf_make_app(const char* window_title, int x, int y, int w, int h, int options, const char* argv0)
{
	SDL_SetMainReady();

#ifdef CF_EMSCRIPTEN
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER;
#else
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
	bool needs_video = options & (APP_OPTIONS_OPENGL_CONTEXT | APP_OPTIONS_OPENGLES_CONTEXT | APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_METAL_CONTEXT | APP_OPTIONS_DEFAULT_GFX_CONTEXT);
	if (!needs_video || options & APP_OPTIONS_NO_GFX) {
		sdl_options &= ~SDL_INIT_VIDEO;
	}
#endif

	// Turn on high DPI support for all platforms.
	options |= SDL_WINDOW_ALLOW_HIGHDPI;
	
	if (SDL_Init(sdl_options)) {
		return cf_result_error("SDL_Init failed");
	}

	if (options & APP_OPTIONS_DEFAULT_GFX_CONTEXT && !(options & APP_OPTIONS_NO_GFX)) {
#ifdef CF_WINDOWS
		options |= APP_OPTIONS_D3D11_CONTEXT;
#elif CF_EMSCRIPTEN
		options |= APP_OPTIONS_OPENGLES_CONTEXT;
#elif CF_APPLE
		options |= APP_OPTIONS_METAL_CONTEXT;
#else
		options |= APP_OPTIONS_OPENGL_CONTEXT;
#endif
	}

	if (options & (APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_OPENGLES_CONTEXT | APP_OPTIONS_OPENGL_CONTEXT | APP_OPTIONS_METAL_CONTEXT)) {
		// Some backends don't support window size of zero.
		w = w <= 0 ? 1 : w;
		h = h <= 0 ? 1 : h;
	}

	Uint32 flags = 0;
	if (options & APP_OPTIONS_OPENGL_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & APP_OPTIONS_OPENGLES_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & APP_OPTIONS_METAL_CONTEXT) flags |= SDL_WINDOW_METAL;
	if (options & APP_OPTIONS_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & APP_OPTIONS_RESIZABLE) flags |= SDL_WINDOW_RESIZABLE;
	if (options & APP_OPTIONS_HIDDEN) flags |= (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);

	if (options & APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	if (options & APP_OPTIONS_OPENGLES_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	}

	SDL_Window* window;
	if (options & APP_OPTIONS_WINDOW_POS_CENTERED) {
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	} else {
		window = SDL_CreateWindow(window_title, x, y, w, h, flags);
	}
	CF_App* app = (CF_App*)CF_ALLOC(sizeof(CF_App));
	CF_PLACEMENT_NEW(app) CF_App;
	app->options = options;
	app->window = window;
	app->w = w;
	app->h = h;
	app->x = x;
	app->y = y;
	list_init(&app->joypads);
	::app = app;

#ifdef CF_WINDOWS
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	app->platform_handle = hwnd;
#else
	void* hwnd = NULL;
#endif

	if ((options & APP_OPTIONS_OPENGL_CONTEXT) | (options & APP_OPTIONS_OPENGLES_CONTEXT) && !(options & APP_OPTIONS_NO_GFX)) {
		SDL_GL_SetSwapInterval(app->vsync);
		SDL_GLContext ctx = SDL_GL_CreateContext(window);
		if (!ctx) {
			CF_FREE(app);
			return cf_result_error("Unable to create OpenGL context.");
		}
		CF_MEMSET(&app->gfx_ctx_params, 0, sizeof(app->gfx_ctx_params));
		app->gfx_ctx_params.color_format = SG_PIXELFORMAT_RGBA8;
		app->gfx_ctx_params.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
		sg_desc params = { };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	if (options & APP_OPTIONS_D3D11_CONTEXT && !(options & APP_OPTIONS_NO_GFX)) {
		cf_dx11_init(hwnd, w, h, 1);
		app->gfx_ctx_params = cf_dx11_get_context();
		sg_desc params = { };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
	}
	
	if (options & APP_OPTIONS_METAL_CONTEXT && !(options & APP_OPTIONS_NO_GFX)) {
		cf_metal_init(window, w, h, 1);
		app->gfx_ctx_params = cf_metal_get_context();
		sg_desc params = { };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	cf_make_aseprite_cache();
	cf_make_png_cache();

	if (app->gfx_enabled) {
		{
			app->backbuffer_quad = cf_make_mesh(CF_USAGE_TYPE_IMMUTABLE, sizeof(Vertex) * 6, 0, 0);
			CF_VertexAttribute attrs[2] = { };
			attrs[0].name = "in_pos";
			attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
			attrs[0].offset = CF_OFFSET_OF(Vertex, x);
			attrs[1].name = "in_uv";
			attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
			attrs[1].offset = CF_OFFSET_OF(Vertex, u);
			cf_mesh_set_attributes(app->backbuffer_quad, attrs, CF_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
			Vertex quad[6];
			s_quad(0, 0, 2, 2, quad);
			cf_mesh_update_vertex_data(app->backbuffer_quad, quad, 6);
		}
		{
			app->backbuffer_material = cf_make_material();
			app->backbuffer_shader = CF_MAKE_SOKOL_SHADER(backbuffer_shd);
		}
		s_canvas(app->w, app->h);
		cf_make_draw();

		// Load up a default image of 1x1 white pixel.
		// Used in various places as a placeholder or default.
		CF_Png img;
		cf_png_cache_load_from_memory("cf_default_png", default_png_data, (size_t)default_png_sz, &img);
		app->default_image_id = img.id;
		CF_ASSERT(app->default_image_id == CF_PNG_ID_RANGE_LO);

		// This will clear the initial offscreen canvas to prevent the first frame from starting off
		// with a black background.
		cf_apply_canvas(app->offscreen_canvas, true);
	}

	if (!(options & APP_OPTIONS_NO_AUDIO)) {
		int more_on_emscripten = 1;
	#ifdef CF_EMSCRIPTEN
		more_on_emscripten = 4;
	#endif
		cs_error_t err = cs_init(NULL, 44100, 1024 * more_on_emscripten, NULL);
		if (err == CUTE_SOUND_ERROR_NONE) {
	#ifndef CF_EMSCRIPTEN
			cs_spawn_mix_thread();
			app->spawned_mix_thread = true;
	#endif // CF_EMSCRIPTEN
			app->audio_needs_updates = true;
		} else {
			CF_Result result;
			result.code = -1;
			result.details = cs_error_as_string(err);
			return result;
		}
	}

	int num_threads_to_spawn = cf_core_count() - 1;
	if (num_threads_to_spawn) {
		app->threadpool = cf_make_threadpool(num_threads_to_spawn);
	}

	CF_Result err = cf_fs_init(argv0);
	if (cf_is_error(err)) {
		CF_ASSERT(0);
	} else if (!(options & APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT)) {
		// Put the base directory (the path to the exe) onto the file system search path.
		cf_fs_mount(cf_fs_get_base_directory(), "", true);
	}

	// Initialize a default ECS world.
	app->world = cf_make_world();
	app->worlds.add(app->world);

	return cf_result_success();
}

void cf_destroy_app()
{
	if (app->using_imgui) {
		ImGui_ImplSDL2_Shutdown();
		simgui_shutdown();
		sg_imgui_discard(&app->sg_imgui);
		app->using_imgui = false;
	}
	if (app->gfx_enabled) {
		sg_end_pass();
		cf_destroy_draw();
		cf_destroy_texture(app->backbuffer);
		cf_destroy_canvas(app->offscreen_canvas);
		cf_destroy_canvas(app->backbuffer_canvas);
		cf_destroy_mesh(app->backbuffer_quad);
		cf_destroy_shader(app->backbuffer_shader);
		cf_destroy_material(app->backbuffer_material);
		cf_destroy_graphics();
		sg_shutdown();
		cf_dx11_shutdown();
		cf_clear_graphics_static_pointers();
	}
	cf_destroy_aseprite_cache();
	cf_destroy_png_cache();
	// Mainly here to cleanup the default world, but, as a convenience we can just clean them all up.
	for (int i = 0; i < app->worlds.count(); ++i) {
		cf_destroy_world(app->worlds[i]);
	}
	cs_shutdown();
	SDL_DestroyWindow(app->window);
	SDL_Quit();
	destroy_threadpool(app->threadpool);
	cs_shutdown();
	CF_Image* easy_sprites = app->easy_sprites.items();
	for (int i = 0; i < app->easy_sprites.count(); ++i) {
		cf_image_free(&easy_sprites[i]);
	}
	app->~CF_App();
	CF_FREE(app);
	cf_fs_destroy();
}

bool cf_app_is_running()
{
	return app->running;
}

void cf_app_signal_shutdown()
{
	app->running = 0;
}

static void s_on_update(void* udata)
{
	cf_pump_input_msgs();
	if (app->audio_needs_updates) {
		cs_update(DELTA_TIME);
	}
	if (app->user_on_update) app->user_on_update(udata);
}

void cf_app_update(CF_OnUpdateFn* on_update)
{
	if (app->gfx_enabled) {
		if (app->using_imgui) {
			simgui_frame_desc_t desc = { };
			desc.delta_time = DELTA_TIME;
			desc.width = app->w;
			desc.height = app->h;
			desc.dpi_scale; // TODO.
			ImGui_ImplSDL2_NewFrame(app->window);
			simgui_new_frame(&desc);
		}
	}
	app->user_on_update = on_update;
	cf_update_time(s_on_update);
}

static void s_imgui_present()
{
	if (app->using_imgui) {
		ImGui::EndFrame();
		ImGui::Render();
		simgui_render();
	}
}

int cf_app_draw_onto_screen(bool clear)
{
	// Update lifteime of all text effects.
	const char** keys = app->text_effect_states.keys();
	CF_TextEffectState* effect_states = app->text_effect_states.items();
	int count = app->text_effect_states.count();
	for (int i = 0; i < count;) {
		if (!effect_states[i].alive) {
			app->text_effect_states.remove(keys[i]);
			--count;
		} else {
			effect_states[i].alive = false;
			++i;
		}
	}

	// Update the spritebatch itself.
	// This does atlas management internally.
	// All references to backend texture id's are now invalid (fetch_image or canvas_get_backend_target_handle).
	spritebatch_tick(&draw->sb);
	spritebatch_defrag(&draw->sb);

	// Render any remaining geometry in the draw API.
	cf_render_to(app->offscreen_canvas, clear);

	// Stretch the app canvas onto the backbuffer canvas.
	cf_apply_canvas(app->backbuffer_canvas, true);
	{
		cf_apply_mesh(app->backbuffer_quad);
		cf_apply_shader(app->backbuffer_shader, app->backbuffer_material);
		v2 u_texture_size = V2((float)app->w, (float)app->h);
		cf_material_set_uniform_fs(app->backbuffer_material, "fs_params", "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
		cf_draw_elements();
	}

	// Dear ImGui draw.
	if (app->using_imgui) {
		sg_imgui_draw(&app->sg_imgui);
		s_imgui_present();
	}

	// Flip to screen.
	cf_commit();
	cf_dx11_present(app->vsync);
	cf_metal_present(app->vsync);
	if (app->options & APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SwapWindow(app->window);
	}

	// Clear all pushed draw parameters.
	draw->colors.set_count(1);
	draw->tints.set_count(1);
	draw->antialias.set_count(1);
	draw->antialias_scale.set_count(1);
	draw->render_states.set_count(1);
	draw->scissors.set_count(1);
	draw->viewports.set_count(1);
	draw->layers.set_count(1);
	draw->layers.set_count(1);
	draw->cam_stack.clear();
	cf_camera_pop();
	draw->font_sizes.set_count(1);
	draw->fonts.set_count(1);
	draw->blurs.set_count(1);
	draw->text_wrap_widths.set_count(1);
	draw->text_clip_boxes.set_count(1);
	draw->vertical.set_count(1);
	draw->user_params.set_count(1);
	draw->shaders.set_count(1);

	// Report the number of draw calls.
	// This is always user draw call count +1.
	int draw_call_count = app->draw_call_count;
	app->draw_call_count = 0;
	return draw_call_count;
}

void cf_app_get_size(int* w, int* h)
{
	if (w) *w = app->w;
	if (h) *h = app->h;
}

int cf_app_get_width()
{
	return app->w;
}

int cf_app_get_height()
{
	return app->h;
}

void cf_app_set_size(int w, int h)
{
	SDL_SetWindowSize(app->window, w, h);
}

void cf_app_get_position(int* x, int* y)
{
	if (x) *x = app->x;
	if (y) *y = app->y;
}

void cf_app_set_position(int x, int y)
{
	SDL_SetWindowPosition(app->window, x, y);
}

bool cf_app_was_resized()
{
	return app->window_state.resized;
}

bool cf_app_was_moved()
{
	return app->window_state.moved;
}

bool cf_app_lost_focus()
{
	return !app->window_state.has_keyboard_focus && app->window_state_prev.has_keyboard_focus;
}

bool cf_app_gained_focus()
{
	return app->window_state.has_keyboard_focus && !app->window_state_prev.has_keyboard_focus;
}

bool cf_app_has_focus()
{
	return app->window_state.has_keyboard_focus;
}

bool cf_app_was_minimized()
{
	return app->window_state.minimized && !app->window_state_prev.minimized;
}

bool cf_app_was_maximized()
{
	return app->window_state.maximized && !app->window_state_prev.maximized;
}

bool cf_app_minimized()
{
	return app->window_state.minimized;
}

bool cf_app_maximized()
{
	return app->window_state.maximized;
}

bool cf_app_was_restored()
{
	return app->window_state.restored && !app->window_state_prev.restored;
}

bool cf_app_mouse_entered()
{
	return app->window_state.mouse_inside_window && !app->window_state_prev.mouse_inside_window;
}

bool cf_app_mouse_exited()
{
	return !app->window_state.mouse_inside_window && app->window_state_prev.mouse_inside_window;
}

bool cf_app_mouse_inside()
{
	return app->window_state.mouse_inside_window;
}

CF_Canvas cf_app_get_canvas()
{
	return app->offscreen_canvas;
}

void cf_app_set_canvas_size(int w, int h)
{
	unapply_canvas();
	s_canvas(w, h);
}

int cf_app_get_canvas_width()
{
	return app->canvas_w;
}

int cf_app_get_canvas_height()
{
	return app->canvas_h;
}

void cf_app_set_vsync(bool true_turn_on_vsync)
{
	app->vsync = true_turn_on_vsync;
}

bool cf_app_get_vsync()
{
	return app->vsync;
}

ImGuiContext* cf_app_init_imgui(bool no_default_font)
{
	if (!app->gfx_enabled) return NULL;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	app->using_imgui = true;

	ImGui::StyleColorsDark();
	
	sg_backend backend = sg_query_backend();
	switch (backend) {
		case SG_BACKEND_GLCORE33: ImGui_ImplSDL2_InitForOpenGL(app->window, NULL); break;
		case SG_BACKEND_GLES3: ImGui_ImplSDL2_InitForOpenGL(app->window, NULL); break;
		case SG_BACKEND_D3D11: ImGui_ImplSDL2_InitForD3D(app->window); break;
		case SG_BACKEND_METAL_IOS: ImGui_ImplSDL2_InitForMetal(app->window); break;
		case SG_BACKEND_METAL_MACOS: ImGui_ImplSDL2_InitForMetal(app->window); break;
		case SG_BACKEND_METAL_SIMULATOR: ImGui_ImplSDL2_InitForMetal(app->window); break;
		case SG_BACKEND_WGPU: ImGui_ImplSDL2_InitForOpenGL(app->window, NULL); break;
	}
	
	simgui_desc_t imgui_params = { 0 };
	imgui_params.no_default_font = no_default_font;
	imgui_params.ini_filename = "imgui.ini";
	simgui_setup(imgui_params);
	sg_imgui_desc_t sg_imgui_desc = { };
	sg_imgui_init(&app->sg_imgui, &sg_imgui_desc);

	return ::ImGui::GetCurrentContext();
}

sg_imgui_t* cf_app_get_sokol_imgui()
{
	if (!app->using_imgui) return NULL;
	return &app->sg_imgui;
}

CF_PowerInfo cf_app_power_info()
{
	CF_PowerInfo info;
	SDL_PowerState state = SDL_GetPowerInfo(&info.seconds_left, &info.percentage_left);
	switch (state) {
	case SDL_POWERSTATE_UNKNOWN: info.state = CF_POWER_STATE_UNKNOWN;
	case SDL_POWERSTATE_ON_BATTERY: info.state = CF_POWER_STATE_ON_BATTERY;
	case SDL_POWERSTATE_NO_BATTERY: info.state = CF_POWER_STATE_NO_BATTERY;
	case SDL_POWERSTATE_CHARGING: info.state = CF_POWER_STATE_CHARGING;
	case SDL_POWERSTATE_CHARGED: info.state = CF_POWER_STATE_CHARGED;
	}
	return info;
}
