/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

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
#include <cute_concurrency.h>
#include <cute_file_system.h>
#include <cute_c_runtime.h>
#include <cute_kv.h>
#include <cute_draw.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_dx11.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifdef CUTE_WINDOWS
#	include <SDL_syswm.h>
#endif

#ifdef CUTE_WINDOWS
#	pragma comment (lib, "dxgi.lib")
#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "dxguid.lib")
#endif

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#ifdef CUTE_USE_CIMGUI
#include <cimgui.h>
#endif // CUTE_FRAMEWORK_CIMGUI

#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
#ifdef SOKOL_D3D11
#	define D3D11_NO_HELPERS
#endif

#include <sokol/sokol_gfx.h>

#define SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include <internal/imgui/sokol_imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <sokol/sokol_gfx_imgui.h>

#include <shaders/backbuffer_shader.h>

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

CF_Result cf_make_app(const char* window_title, int x, int y, int w, int h, int options, const char* argv0)
{
	SDL_SetMainReady();

#ifdef CUTE_EMSCRIPTEN
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER;
#else
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
	bool needs_video = options & (APP_OPTIONS_OPENGL_CONTEXT | APP_OPTIONS_OPENGLES_CONTEXT | APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_DEFAULT_GFX_CONTEXT);
	if (!needs_video) {
		sdl_options &= ~SDL_INIT_VIDEO;
	}
#endif
	if (SDL_Init(sdl_options)) {
		return cf_result_error("SDL_Init failed");
	}

	if (options & APP_OPTIONS_DEFAULT_GFX_CONTEXT) {
#ifdef CUTE_WINDOWS
		options |= APP_OPTIONS_D3D11_CONTEXT;
#elif CUTE_EMSCRIPTEN
		options |= APP_OPTIONS_OPENGLES_CONTEXT;
#else
		options |= APP_OPTIONS_OPENGL_CONTEXT;
#endif
	}

	if (options & (APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_OPENGLES_CONTEXT | APP_OPTIONS_OPENGL_CONTEXT)) {
		// D3D11 crashes if w/h are not positive.
		w = w <= 0 ? 1 : w;
		h = h <= 0 ? 1 : h;
	}

	Uint32 flags = 0;
	if (options & APP_OPTIONS_OPENGL_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & APP_OPTIONS_OPENGLES_CONTEXT) flags |= SDL_WINDOW_OPENGL;
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
	CF_App* app = (CF_App*)CUTE_ALLOC(sizeof(CF_App));
	CUTE_PLACEMENT_NEW(app) CF_App;
	app->options = options;
	app->window = window;
	app->w = w;
	app->h = h;
	app->x = x;
	app->y = y;
	list_init(&app->joypads);
	::app = app;

#ifdef CUTE_WINDOWS
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	app->platform_handle = hwnd;
#else
	void* hwnd = NULL;
#endif

	if ((options & APP_OPTIONS_OPENGL_CONTEXT) | (options & APP_OPTIONS_OPENGLES_CONTEXT)) {
		SDL_GL_SetSwapInterval(0);
		SDL_GLContext ctx = SDL_GL_CreateContext(window);
		if (!ctx) {
			CUTE_FREE(app);
			return cf_result_error("Unable to create OpenGL context.");
		}
		CUTE_MEMSET(&app->gfx_ctx_params, 0, sizeof(app->gfx_ctx_params));
		app->gfx_ctx_params.color_format = SG_PIXELFORMAT_RGBA8;
		app->gfx_ctx_params.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
		sg_desc params = { 0 };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	if (options & APP_OPTIONS_D3D11_CONTEXT) {
		cf_dx11_init(hwnd, w, h, 1);
		app->gfx_ctx_params = cf_dx11_get_context();
		sg_desc params = { 0 };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	if (app->gfx_enabled) {
		{
			CF_TextureParams params = cf_texture_defaults();
			params.width = app->w;
			params.height = app->h;
			params.render_target = true;
			app->backbuffer = cf_make_texture(params);
		}
		{
			CF_TextureParams params = cf_texture_defaults();
			params.width = app->w;
			params.height = app->h;
			params.render_target = true;
			params.pixel_format = CF_PIXELFORMAT_DEPTH_STENCIL;
			app->backbuffer_depth_stencil = cf_make_texture(params);
		}
		{
			CF_CanvasParams params = cf_canvas_defaults();
			params.target = app->backbuffer;
			params.depth_stencil_target = app->backbuffer_depth_stencil;
			app->offscreen_canvas = cf_make_canvas(params);
		}
		{
			CF_CanvasParams params = cf_canvas_defaults();
			params.clear_settings.color = cf_color_black();
			app->backbuffer_canvas = cf_make_canvas(params);
		}
		{
			app->backbuffer_quad = cf_make_mesh(CF_USAGE_TYPE_IMMUTABLE, sizeof(Vertex) * 6, 0, 0);
			CF_VertexAttribute attrs[2] = { };
			attrs[0].name = "in_pos";
			attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
			attrs[0].offset = CUTE_OFFSET_OF(Vertex, x);
			attrs[1].name = "in_uv";
			attrs[1].format = CF_VERTEX_FORMAT_FLOAT2;
			attrs[1].offset = CUTE_OFFSET_OF(Vertex, u);
			cf_mesh_set_attributes(app->backbuffer_quad, attrs, CUTE_ARRAY_SIZE(attrs), sizeof(Vertex), 0);
			Vertex quad[6];
			s_quad(0, 0, 2, 2, quad);
			cf_mesh_update_vertex_data(app->backbuffer_quad, quad, 6);
		}
		{
			app->backbuffer_material = cf_make_material();
			cf_material_set_texture_fs(app->backbuffer_material, "u_image", app->backbuffer);
			v2 u_texture_size = V2((float)app->w, (float)app->h);
			cf_material_set_uniform_fs(app->backbuffer_material, "fs_params", "u_texture_size", &u_texture_size, CF_UNIFORM_TYPE_FLOAT2, 1);
			app->backbuffer_shader = CF_MAKE_SOKOL_SHADER(backbuffer_shd);
		}
		cf_make_draw();
		cf_make_aseprite_cache();
		cf_make_png_cache();
	}

	if (!(options & APP_OPTIONS_NO_AUDIO)) {
		int more_on_emscripten = 1;
	#ifdef CUTE_EMSCRIPTEN
		more_on_emscripten = 4;
	#endif
		cs_error_t err = cs_init(NULL, 44100, 1024 * more_on_emscripten, NULL);
		if (err == CUTE_SOUND_ERROR_NONE) {
	#ifndef CUTE_EMSCRIPTEN
			cs_spawn_mix_thread();
			app->spawned_mix_thread = true;
	#endif // CUTE_EMSCRIPTEN
		} else {
			CUTE_ASSERT(false);
		}
	}

	int num_threads_to_spawn = cf_core_count() - 1;
	if (num_threads_to_spawn) {
		app->threadpool = cf_make_threadpool(num_threads_to_spawn);
	}

	CF_Result err = cf_fs_init(argv0);
	if (cf_is_error(err)) {
		CUTE_ASSERT(0);
	} else if (!(options & APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT)) {
		// Put the base directory (the path to the exe) onto the file system search path.
		cf_fs_mount(cf_fs_get_base_directory(), "", true);
	}

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
		cf_destroy_draw();
		cf_destroy_aseprite_cache();
		cf_destroy_png_cache();
		cf_destroy_texture(app->backbuffer);
		cf_destroy_canvas(app->backbuffer_canvas);
		cf_destroy_mesh(app->backbuffer_quad);
		cf_destroy_shader(app->backbuffer_shader);
		cf_destroy_material(app->backbuffer_material);
		cf_destroy_graphics();
		sg_shutdown();
		cf_dx11_shutdown();
	}
	cs_shutdown();
	SDL_DestroyWindow(app->window);
	SDL_Quit();
	destroy_threadpool(app->threadpool);
	cs_shutdown();
	int schema_count = app->entity_parsed_schemas.count();
	CF_KeyValue** schemas = app->entity_parsed_schemas.items();
	for (int i = 0; i < schema_count; ++i) cf_kv_destroy(schemas[i]);
	app->~CF_App();
	CUTE_FREE(app);
	cf_fs_destroy();
}

bool cf_app_is_running()
{
	return app->running;
}

void cf_app_stop_running()
{
	app->running = 0;
}

void cf_app_update(float dt)
{
	app->dt = dt;
	cf_pump_input_msgs();
	if (app->audio_system) {
		cs_update(dt);
#ifdef CUTE_EMSCRIPTEN
		cf_app_do_mixing(cf_app);
#endif // CUTE_EMSCRIPTEN
	}

	if (app->gfx_enabled) {
		if (app->using_imgui) {
			simgui_new_frame(app->w, app->h, dt);
			ImGui_ImplSDL2_NewFrame(app->window);
		}
	}
}

void cf_app_size(int* w, int* h)
{
	if (w) *w = app->w;
	if (h) *h = app->h;
}

void cf_app_position(int* x, int* y)
{
	if (x) *x = app->x;
	if (y) *y = app->y;
}

bool cf_app_was_size_changed()
{
	return app->window_state.resized;
}

bool cf_app_was_moved()
{
	return app->window_state.moved;
}

bool cf_app_keyboard_lost_focus()
{
	return !app->window_state.has_keyboard_focus && app->window_state_prev.has_keyboard_focus;
}

bool cf_app_keyboard_gained_focus()
{
	return app->window_state.has_keyboard_focus && !app->window_state_prev.has_keyboard_focus;
}

bool cf_app_keyboard_has_focus()
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

bool cf_app_is_minimized()
{
	return app->window_state.minimized;
}

bool cf_app_is_maximized()
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

static void s_imgui_present()
{
	if (app->using_imgui) {
		ImGui::EndFrame();
		ImGui::Render();
		simgui_render();
	}
}

CF_Canvas cf_app_get_canvas()
{
	return app->offscreen_canvas;
}

int cf_app_get_canvas_width()
{
	return app->w;
}

int cf_app_get_canvas_height()
{
	return app->h;
}

int cf_app_present()
{
	cf_render_to(app->offscreen_canvas);
	cf_apply_canvas(app->backbuffer_canvas);
	cf_apply_mesh(app->backbuffer_quad);
	cf_apply_shader(app->backbuffer_shader, app->backbuffer_material);
	cf_draw_elements();
	if (app->using_imgui) {
		sg_imgui_draw(&app->sg_imgui);
		s_imgui_present();
	}

	cf_commit();
	cf_dx11_present();
	if (app->options & APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SwapWindow(app->window);
	}

	int draw_call_count = app->draw_call_count;
	app->draw_call_count = 0;
	return draw_call_count;
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
		case SG_BACKEND_GLES2: ImGui_ImplSDL2_InitForOpenGL(app->window, NULL); break;
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
	sg_imgui_init(&app->sg_imgui);

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

void cf_sleep(int milliseconds)
{
	SDL_Delay((Uint32)milliseconds);
}
