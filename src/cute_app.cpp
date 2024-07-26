/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
#include <internal/cute_input_internal.h>
#include <internal/cute_graphics_internal.h>
#include <internal/cute_draw_internal.h>
#include <internal/cute_dx11.h>
#include <internal/cute_metal.h>
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>

#include <data/fonts/calibri.h>

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

#define SOKOL_LOG_IMPL
#include <sokol/sokol_log.h>

#include <shaders/backbuffer_shader.h>

CF_STATIC_ASSERT(sizeof(uint64_t) >= sizeof(void*), "Must be equal for opaque id implementations throughout CF.");

static void s_init_video()
{
	static bool init = false;
	if (init) return;
	init = true;
	SDL_SetMainReady();
	SDL_Init(SDL_INIT_VIDEO);
}

int cf_display_count()
{
	s_init_video();
	return SDL_GetNumVideoDisplays();
}

int cf_display_x(int display_index)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_index, &rect);
	return rect.x;
}

int cf_display_y(int display_index)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_index, &rect);
	return rect.y;
}

int cf_display_width(int display_index)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_index, &rect);
	return rect.w;
}

int cf_display_height(int display_index)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_index, &rect);
	return rect.h;
}

int cf_display_refresh_rate(int display_index)
{
	s_init_video();
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(display_index, &mode);
	return mode.refresh_rate;
}

CF_Rect cf_display_bounds(int display_index)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_index, &rect);
	CF_Rect result = { rect.x, rect.y, rect.w, rect.h };
	return result;
}

const char* cf_display_name(int display_index)
{
	s_init_video();
	return SDL_GetDisplayName(display_index);
}

CF_DisplayOrientation cf_display_orientation(int display_index)
{
	s_init_video();
	SDL_DisplayOrientation orientation = SDL_GetDisplayOrientation(display_index);
	switch (orientation)
	{
	default:
	case SDL_ORIENTATION_UNKNOWN: return CF_DISPLAY_ORIENTATION_UNKNOWN;
	case SDL_ORIENTATION_LANDSCAPE: return CF_DISPLAY_ORIENTATION_LANDSCAPE;
	case SDL_ORIENTATION_LANDSCAPE_FLIPPED: return CF_DISPLAY_ORIENTATION_LANDSCAPE_FLIPPED;
	case SDL_ORIENTATION_PORTRAIT: return CF_DISPLAY_ORIENTATION_PORTRAIT;
	case SDL_ORIENTATION_PORTRAIT_FLIPPED: return CF_DISPLAY_ORIENTATION_PORTRAIT_FLIPPED;
	}
}

CF_GLOBAL CF_App* app;

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
		CF_CanvasParams params = cf_canvas_defaults(w, h);
		if (app->offscreen_canvas.id) {
			cf_destroy_canvas(app->offscreen_canvas);
		}
		app->offscreen_canvas = cf_make_canvas(params);
	}
	{
		// Size (0,0) is a hidden feature to use sokol's default canvas. This let's us
		// get pixels to the actual screen as a final pass.
		CF_CanvasParams params = cf_canvas_defaults(0, 0);
		if (app->backbuffer_canvas.id) {
			cf_destroy_canvas(app->backbuffer_canvas);
		}
		app->backbuffer_canvas = cf_make_canvas(params);
	}
	app->canvas_w = w;
	app->canvas_h = h;
	cf_material_set_texture_fs(app->backbuffer_material, "u_image", cf_canvas_get_target(app->offscreen_canvas));
}

CF_Result cf_make_app(const char* window_title, int display_index, int x, int y, int w, int h, int options, const char* argv0)
{
	SDL_SetMainReady();

#ifdef CF_EMSCRIPTEN
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER;
#else
	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;
	if (options & APP_OPTIONS_NO_GFX) {
		sdl_options &= ~SDL_INIT_VIDEO;
	} else {
		bool specific_gfx_context = options & (APP_OPTIONS_OPENGL_CONTEXT | APP_OPTIONS_OPENGLES_CONTEXT | APP_OPTIONS_D3D11_CONTEXT | APP_OPTIONS_METAL_CONTEXT);
		if (!specific_gfx_context) {
			options |= APP_OPTIONS_DEFAULT_GFX_CONTEXT;
		}
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
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED_DISPLAY(display_index), SDL_WINDOWPOS_CENTERED_DISPLAY(display_index), w, h, flags);
	} else {
		int x_offset = display_x(display_index);
		int y_offset = display_y(display_index);
		window = SDL_CreateWindow(window_title, x_offset+x, y_offset+y, w, h, flags);
	}
	CF_App* app = (CF_App*)CF_ALLOC(sizeof(CF_App));
	CF_PLACEMENT_NEW(app) CF_App;
	app->options = options;
	app->window = window;
	app->w = w;
	app->h = h;
	SDL_GetWindowPosition(app->window, &app->x, &app->y);
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
		params.logger.func = slog_func;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	if (options & APP_OPTIONS_D3D11_CONTEXT && !(options & APP_OPTIONS_NO_GFX)) {
		cf_dx11_init(hwnd, w, h, 1);
		app->gfx_ctx_params = cf_dx11_get_context();
		sg_desc params = { };
		params.context = app->gfx_ctx_params;
		params.logger.func = slog_func;
		sg_setup(params);
		app->gfx_enabled = true;
	}
	
	if (options & APP_OPTIONS_METAL_CONTEXT && !(options & APP_OPTIONS_NO_GFX)) {
		cf_metal_init(window, w, h, 1);
		app->gfx_ctx_params = cf_metal_get_context();
		sg_desc params = { };
		params.context = app->gfx_ctx_params;
		params.logger.func = slog_func;
		sg_setup(params);
		app->gfx_enabled = true;
	}

	cf_make_aseprite_cache();
	cf_make_png_cache();

	if (app->gfx_enabled) {
		// Setup the backbuffer fullscreen mesh and canvas.
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

		// Create a default font.
		make_font_from_memory(calibri_data, calibri_sz, "Calibri");
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
			//cs_cull_duplicates(true); -- https://github.com/RandyGaul/cute_framework/issues/172
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
		cf_destroy_canvas(app->offscreen_canvas);
		cf_destroy_canvas(app->backbuffer_canvas);
		cf_destroy_mesh(app->backbuffer_quad);
		cf_destroy_shader(app->backbuffer_shader);
		cf_destroy_material(app->backbuffer_material);
		if (app->canvas_blit_init) {
			cf_destroy_mesh(app->blit_mesh);
			cf_destroy_material(app->blit_material);
			cf_destroy_shader(app->blit_shader);
		}
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
	destroy_mutex(&app->on_sound_finish_mutex);
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
		if (app->on_sound_finish_single_threaded) {
			mutex_lock(&app->on_sound_finish_mutex);
			Array<CF_Sound> on_finish = app->on_sound_finish_queue;
			app->on_sound_finish_queue.clear();
			mutex_unlock(&app->on_sound_finish_mutex);
			for (int i = 0; i < on_finish.size(); ++i) {
				app->on_sound_finish(on_finish[i], app->on_sound_finish_udata);
			}
		}
	}
	if (app->user_on_update) app->user_on_update(udata);
}

void cf_app_update(CF_OnUpdateFn* on_update)
{
	if (app->gfx_enabled) {
		// Deal with DPI scaling.
		int pw = 0, ph = 0;
		SDL_GetWindowSizeInPixels(app->window, &pw, &ph);
		app->dpi_scale = (float)ph / (float)app->h;
		app->dpi_scale_was_changed = false;
		if (app->dpi_scale != app->dpi_scale_prev) {
			app->dpi_scale_was_changed = true;
			app->dpi_scale_prev = app->dpi_scale;
		}

		if (app->using_imgui) {
			simgui_frame_desc_t desc = { };
			desc.delta_time = DELTA_TIME;
			desc.width = app->w;
			desc.height = app->h;
			desc.dpi_scale = app->dpi_scale;
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
	if (!draw->delay_defrag) {
		spritebatch_tick(&draw->sb);
		spritebatch_defrag(&draw->sb);
	}

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

	// Do defrag down here after rendering ImGui to avoid thrashing any texture IDs. Generally we want to defrag
	// before doing final rendering to reduce draw call count, but in the case where ImGui is rendered it's acceptable
	// to have the perf-hit and delay until next frame.
	if (draw->delay_defrag) {
		spritebatch_tick(&draw->sb);
		spritebatch_defrag(&draw->sb);
		draw->delay_defrag = false;
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
	draw->reset_cam();
	draw->font_sizes.set_count(1);
	draw->fonts.set_count(1);
	draw->blurs.set_count(1);
	draw->text_wrap_widths.set_count(1);
	draw->text_clip_boxes.set_count(1);
	draw->vertical.set_count(1);
	draw->user_params.set_count(1);
	draw->shaders.set_count(1);
	material_clear_textures(draw->material);
	material_clear_uniforms(draw->material);

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

float cf_app_get_dpi_scale()
{
	return app->dpi_scale;
}

bool cf_app_dpi_scale_was_changed()
{
	return app->dpi_scale_was_changed;
}

void cf_app_set_size(int w, int h)
{
	SDL_SetWindowSize(app->window, w, h);
	app->w = w;
	app->h = h;
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

void cf_app_request_attention()
{
	SDL_FlashWindow(app->window, SDL_FLASH_BRIEFLY);
}

void cf_app_request_attention_continuously()
{
	SDL_FlashWindow(app->window, SDL_FLASH_UNTIL_FOCUSED);
}

void cf_app_request_attention_cancel()
{
	SDL_FlashWindow(app->window, SDL_FLASH_CANCEL);
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

void cf_app_set_windowed_mode()
{
	SDL_SetWindowFullscreen(app->window, 0);
}

void cf_app_set_borderless_fullscreen_mode()
{
	SDL_SetWindowFullscreen(app->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void cf_app_set_fullscreen_mode()
{
	SDL_SetWindowFullscreen(app->window, SDL_WINDOW_FULLSCREEN);
}

void cf_app_set_title(const char* title)
{
	SDL_SetWindowTitle(app->window, title);
}

void cf_app_set_icon(const char* virtual_path_to_png)
{
	CF_Image img;
	if (is_error(cf_image_load_png(virtual_path_to_png, &img))) {
		fprintf(stderr, "Unable to open icon png file %s.", virtual_path_to_png);
		return;
	}
	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(img.pix, img.w, img.h, 32, img.w * 4, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_SetWindowIcon(app->window, icon);
	SDL_FreeSurface(icon);
	cf_image_free(&img);
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
