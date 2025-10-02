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
#include <internal/cute_png_cache_internal.h>
#include <internal/cute_aseprite_cache_internal.h>
#include <internal/cute_imgui_internal.h>

#include <data/fonts/calibri.h>

#include <SDL3/SDL.h>

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

CF_STATIC_ASSERT(sizeof(uint64_t) >= sizeof(void*), "Must be equal for opaque id implementations throughout CF.");

static void s_init_video()
{
	static bool init = false;
	if (init) return;
	init = SDL_Init(SDL_INIT_VIDEO);
}

CF_DisplayID cf_default_display()
{
	s_init_video();
	return SDL_GetPrimaryDisplay();
}

int cf_display_count()
{
	s_init_video();
	int count = 0;
	SDL_GetDisplays(&count);
	return count;
}

CF_DisplayID* cf_get_display_list()
{
	s_init_video();
	int count = 0;
	SDL_DisplayID* ids = SDL_GetDisplays(&count);
	return (CF_DisplayID*)ids;
}

void cf_free_display_list(CF_DisplayID* display_list)
{
	SDL_free(display_list);
}

int cf_display_x(CF_DisplayID display_id)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_id, &rect);
	return rect.x;
}

int cf_display_y(CF_DisplayID display_id)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_id, &rect);
	return rect.y;
}

int cf_display_width(CF_DisplayID display_id)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_id, &rect);
	return rect.w;
}

int cf_display_height(CF_DisplayID display_id)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_id, &rect);
	return rect.h;
}

float cf_display_refresh_rate(CF_DisplayID display_id)
{
	s_init_video();
	const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display_id);
	return mode->refresh_rate;
}

CF_Rect cf_display_bounds(CF_DisplayID display_id)
{
	s_init_video();
	SDL_Rect rect;
	SDL_GetDisplayBounds(display_id, &rect);
	CF_Rect result = { rect.x, rect.y, rect.w, rect.h };
	return result;
}

const char* cf_display_name(CF_DisplayID display_id)
{
	s_init_video();
	return SDL_GetDisplayName(display_id);
}

CF_DisplayOrientation cf_display_orientation(CF_DisplayID display_id)
{
	s_init_video();
	SDL_DisplayOrientation orientation = SDL_GetCurrentDisplayOrientation(display_id);
	switch (orientation) {
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
	CF_CanvasParams params = cf_canvas_defaults(w, h);
	params.target.filter = CF_FILTER_LINEAR;
	if (app->offscreen_canvas.id) {
		cf_destroy_canvas(app->offscreen_canvas);
	}
	params.depth_stencil_enable = true;
	params.sample_count = (CF_SampleCount)cf_clamp_int((app->sample_count >> 1), 0, 3);
	app->offscreen_canvas = cf_make_canvas(params);
	app->canvas_w = w;
	app->canvas_h = h;
}

CF_Result cf_make_app(const char* window_title, CF_DisplayID display_id, int x, int y, int w, int h, CF_AppOptionFlags options, const char* argv0)
{
	bool use_dx11 = options & CF_APP_OPTIONS_GFX_D3D11_BIT;
	bool use_dx12 = options & CF_APP_OPTIONS_GFX_D3D12_BIT;
	bool use_metal = options & CF_APP_OPTIONS_GFX_METAL_BIT;
	bool use_vulkan = options & CF_APP_OPTIONS_GFX_VULKAN_BIT;
	bool use_opengl = options & CF_APP_OPTIONS_GFX_OPENGL_BIT;
	bool use_gfx = !(options & CF_APP_OPTIONS_NO_GFX_BIT);

#ifdef CF_EMSCRIPTEN
	// This is the only supported backend as of now
	use_opengl = true;
#endif

	// Ensure the user selected only one backend, if they selected one at all.
	if (use_dx11) {
		CF_ASSERT(!use_dx12);
		CF_ASSERT(!use_metal);
		CF_ASSERT(!use_vulkan);
		CF_ASSERT(!use_opengl);
	}
	if (use_dx12) {
		CF_ASSERT(!use_dx11);
		CF_ASSERT(!use_metal);
		CF_ASSERT(!use_vulkan);
		CF_ASSERT(!use_opengl);
	}
	if (use_metal) {
		CF_ASSERT(!use_dx11);
		CF_ASSERT(!use_dx12);
		CF_ASSERT(!use_vulkan);
		CF_ASSERT(!use_opengl);
	}
	if (use_vulkan) {
		CF_ASSERT(!use_dx11);
		CF_ASSERT(!use_dx12);
		CF_ASSERT(!use_metal);
		CF_ASSERT(!use_opengl);
	}
	if (use_opengl) {
		CF_ASSERT(!use_dx11);
		CF_ASSERT(!use_dx12);
		CF_ASSERT(!use_metal);
		CF_ASSERT(!use_vulkan);
	}

	Uint32 sdl_options = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC;
	if (options & CF_APP_OPTIONS_NO_GFX_BIT) {
		sdl_options &= ~SDL_INIT_VIDEO;
	}
	if (!(options & CF_APP_OPTIONS_NO_AUDIO_BIT)) {
		SDL_Init(SDL_INIT_AUDIO);
	}

	if (!SDL_Init(sdl_options)) {
		return cf_result_error("SDL_Init failed");
	}

	CF_BackendType gfx_backend_type = CF_BACKEND_TYPE_INVALID;
	if (use_gfx) {
		// Some backends don't support window size of zero.
		w = w <= 0 ? 1 : w;
		h = h <= 0 ? 1 : h;

		CF_Result init_gfx_result = {  };
		bool debug = options & CF_APP_OPTIONS_GFX_DEBUG_BIT;

		// Create the SDL GPU device.
		if (!use_opengl) {
#ifndef CF_EMSCRIPTEN
			const char* device_name = NULL;
			if (use_dx11) {
				device_name = "direct3d11";
			} else if (use_dx12) {
				device_name = "direct3d12";
			} else if (use_metal) {
				device_name = "metal";
			} else if (use_vulkan) {
				device_name = "vulkan";
			}

			init_gfx_result = cf_sdlgpu_init(device_name, debug, &gfx_backend_type);
#endif
		} else {
			init_gfx_result = cf_gles_init(debug);
			gfx_backend_type = CF_BACKEND_TYPE_GLES3;
		}

		if (cf_is_error(init_gfx_result)) {
			gfx_backend_type = CF_BACKEND_TYPE_INVALID;
			return init_gfx_result;
		}
	}

	Uint32 flags = 0;
	flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY; // Turn on high DPI support for all platforms.
	if (use_opengl) flags |= SDL_WINDOW_OPENGL;
	if (use_metal) flags |= SDL_WINDOW_METAL;
	if (options & CF_APP_OPTIONS_FULLSCREEN_BIT) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & CF_APP_OPTIONS_RESIZABLE_BIT) flags |= SDL_WINDOW_RESIZABLE;
	if (options & CF_APP_OPTIONS_HIDDEN_BIT) flags |= (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);

	SDL_Window* window;
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_title);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, w);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, h);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);
	if (options & CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT) {
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(display_id));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(display_id));
	} else {
		if (display_id == 0) display_id = SDL_GetPrimaryDisplay();
		int x_offset = display_x(display_id);
		int y_offset = display_y(display_id);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x_offset+x);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y_offset+y);
	}
	window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);

	CF_App* app = (CF_App*)CF_ALLOC(sizeof(CF_App));
	CF_PLACEMENT_NEW(app) CF_App;
	app->options = options;
	app->gfx_backend_type = gfx_backend_type;
	app->window = window;
	app->w = w;
	app->h = h;
	SDL_GetWindowPosition(app->window, &app->x, &app->y);
	app->dpi_scale = SDL_GetWindowDisplayScale(app->window);
	app->dpi_scale_prev = app->dpi_scale;
	::app = app;
	cf_make_aseprite_cache();
	cf_make_png_cache();

	if (use_gfx) {
		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
			cf_gles_attach(app->window);
		} else {
#ifndef CF_EMSCRIPTEN
			cf_sdlgpu_attach(app->window);
#endif
		}

		cf_load_internal_shaders();
		cf_make_draw();

		s_canvas(app->w, app->h);

		// Load up a default image of 1x1 white pixel.
		// Used in various places as a placeholder or default.
		CF_Png img;
		cf_png_cache_load_from_memory("cf_default_png", default_png_data, (size_t)default_png_sz, &img);
		app->default_image_id = img.id;
		CF_ASSERT(app->default_image_id == CF_PNG_ID_RANGE_LO);

		// Create the default font.
		make_font_from_memory(calibri_data, calibri_sz, "Calibri");

		// Ensure initial resources are uploaded
		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
			cf_gles_flush();
		} else {
#ifndef CF_EMSCRIPTEN
			cf_sdlgpu_flush();
#endif
		}
	}

#ifdef CF_WINDOWS
	app->platform_handle = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

	app->gfx_enabled = use_gfx;

	if (!(options & CF_APP_OPTIONS_NO_AUDIO_BIT)) {
		int more_on_emscripten = 1;
#ifdef CF_EMSCRIPTEN
		more_on_emscripten = 4;
#endif
		cs_error_t err = cs_init(NULL, 44100, 1024 * more_on_emscripten, NULL);
		if (err == CUTE_SOUND_ERROR_NONE) {
#ifndef CF_EMSCRIPTEN
			cs_spawn_mix_thread();
			app->spawned_mix_thread = true;
#endif
			app->audio_needs_updates = true;
			//cs_cull_duplicates(true); -- https://github.com/RandyGaul/cute_framework/issues/172
		} else {
			CF_Result result;
			result.code = -1;
			result.details = cs_error_as_string(err);
			return result;
		}
	}

	CF_Result err = cf_fs_init(argv0);
	if (cf_is_error(err)) {
		CF_ASSERT(0);
	} else if (!(options & CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT_BIT)) {
		// Put the base directory (the path to the exe) onto the file system search path.
		cf_fs_mount(cf_fs_get_base_directory(), "", true);
	}

	return cf_result_success();
}

void cf_destroy_app()
{
	if (app->using_imgui) {
		cf_imgui_shutdown();
		app->using_imgui = false;
	}
	if (app->gfx_enabled) {
		cf_destroy_canvas(app->offscreen_canvas);
		cf_destroy_draw();
		cf_unload_internal_shaders();

		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
			cf_gles_cleanup();
		} else {
#ifndef CF_EMSCRIPTEN
			cf_sdlgpu_cleanup();
#endif
		}
	}
	cf_destroy_aseprite_cache();
	cf_destroy_png_cache();
	cs_shutdown();
	destroy_mutex(&app->on_sound_finish_mutex);
	SDL_DestroyWindow(app->window);
	SDL_Quit();
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
		cs_update(CF_DELTA_TIME);
		if (app->on_sound_finish_single_threaded) {
			mutex_lock(&app->on_sound_finish_mutex);
			Array<CF_Sound> on_finish = app->on_sound_finish_queue;
			app->on_sound_finish_queue.clear();
			mutex_unlock(&app->on_sound_finish_mutex);
			for (int i = 0; i < on_finish.size(); ++i) {
				app->on_sound_finish(on_finish[i], app->on_sound_finish_udata);
			}
			if (app->on_music_finish && app->on_music_finish_signal) {
				app->on_music_finish_signal = false;
				app->on_music_finish(app->on_music_finish_udata);
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
			if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
				ImGui_ImplOpenGL3_NewFrame();
			} else {
				ImGui_ImplSDLGPU3_NewFrame();
			}
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();
		}

		if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
			cf_gles_begin_frame();
		} else {
#ifndef CF_EMSCRIPTEN
			cf_sdlgpu_begin_frame();
#endif
		}
		cf_shader_watch();
	}
	app->user_on_update = on_update;
	cf_begin_frame_input();
	cf_update_time(s_on_update);
}

int cf_app_draw_onto_screen(bool clear)
{
	if (app->sync_window) {
		app->sync_window = false;
		SDL_SyncWindow(app->window);
	}

	// Update lifetime of all text effects.
	uint64_t* effect_keys = app->text_effect_states.keys();
	CF_TextEffectState* effect_states = app->text_effect_states.items();
	int count = app->text_effect_states.count();
	for (int i = 0; i < count;) {
		if (!effect_states[i].alive) {
			app->text_effect_states.remove(effect_keys[i]);
			--count;
		} else {
			effect_states[i].alive = false;
			++i;
		}
	}

	// Do the same for text states.
	uint64_t* text_keys = app->parsed_text_states.keys();
	CF_ParsedTextState* text_states = app->parsed_text_states.items();
	count = app->parsed_text_states.count();
	for (int i = 0; i < count;) {
		if (!text_states[i].alive) {
			app->parsed_text_states.remove(text_keys[i]);
			--count;
		} else {
			text_states[i].alive = false;
			++i;
		}
	}

	// Update the spritebatch itself.
	// This does atlas management internally.
	// All references to backend texture id's are now invalid (fetch_image or cf_texture_handle).
	if (!draw->delay_defrag) {
		spritebatch_tick(&draw->sb);
		spritebatch_defrag(&draw->sb);
	}

	// Render any remaining geometry in the draw API.
	cf_render_to(app->offscreen_canvas, clear);

	// Draw the app canvas
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		cf_gles_blit_canvas(app->offscreen_canvas);
	} else {
#ifndef CF_EMSCRIPTEN
		cf_sdlgpu_blit_canvas(app->offscreen_canvas);
#endif
	}

	// Dear ImGui draw.
	if (app->using_imgui) {
		ImGui::EndFrame();
		ImGui::Render();
		cf_imgui_draw();
	}

	// Do defrag down here after rendering ImGui to avoid thrashing any texture IDs. Generally we want to defrag
	// before doing final rendering to reduce draw call count, but in the case where ImGui is rendered it's acceptable
	// to have the perf-hit and delay until next frame.
	if (draw->delay_defrag) {
		spritebatch_tick(&draw->sb);
		spritebatch_defrag(&draw->sb);
		draw->delay_defrag = false;
	}

	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		cf_gles_end_frame();
	} else {
#ifndef CF_EMSCRIPTEN
		cf_sdlgpu_end_frame();
#endif
	}

	// Clear all pushed draw parameters.
	draw->alpha_discards.set_count(1);
	draw->colors.set_count(1);
	draw->antialias.set_count(1);
	draw->antialias_scale.set_count(1);
	draw->render_states.set_count(1);
	draw->scissors.set_count(1);
	draw->viewports.set_count(1);
	draw->layers.set_count(1);
	draw->reset_cam();
	draw->font_sizes.set_count(1);
	draw->fonts.set_count(1);
	draw->blurs.set_count(1);
	draw->text_wrap_widths.set_count(1);
	draw->vertical.set_count(1);
	draw->text_ids.set_count(1);
	draw->user_params.set_count(1);
	draw->shaders.set_count(1);
	draw->verts.clear();
	draw->draw_item_order = 0;
	draw->cmds.clear();
	draw->add_cmd();

	// Report the number of draw calls.
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

void cf_app_show_window()
{
	SDL_ShowWindow(app->window);
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
	app->sync_window = true;
}

void cf_app_get_position(int* x, int* y)
{
	if (x) *x = app->x;
	if (y) *y = app->y;
}

void cf_app_set_position(int x, int y)
{
	SDL_SetWindowPosition(app->window, x, y);
	app->sync_window = true;
}

void cf_app_center_window()
{
	SDL_SetWindowPosition(app->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	app->sync_window = true;
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

bool cf_app_set_msaa(int sample_count)
{
	bool supported = false;
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		supported = cf_gles_supports_msaa(sample_count);
	} else {
#ifndef CF_EMSCRIPTEN
		supported = cf_sdlgpu_supports_msaa(sample_count);
#endif
	}

	if (supported && app->sample_count != sample_count) {
		app->sample_count = sample_count;
		s_canvas(app->w, app->h);
	}

	return supported;
}

CF_Canvas cf_app_get_canvas()
{
	return app->offscreen_canvas;
}

void cf_app_set_canvas_size(int w, int h)
{
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
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		cf_gles_set_vsync(true_turn_on_vsync);
	} else {
#ifndef CF_EMSCRIPTEN
		cf_sdlgpu_set_vsync(true_turn_on_vsync);
#endif
	}
}

void cf_app_set_vsync_mailbox(bool true_turn_on_mailbox)
{
	app->vsync = true_turn_on_mailbox;
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		CF_ASSERT(!"This is only implemented on SDL_Gpu backends (Metal/DX11/DX12/Vulkan).");
	} else {
#ifndef CF_EMSCRIPTEN
		cf_sdlgpu_set_vsync_mailbox(true_turn_on_mailbox);
#endif
	}
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
	SDL_SetWindowFullscreen(app->window, true);
}

void cf_app_set_fullscreen_mode()
{
	SDL_SetWindowFullscreen(app->window, SDL_WINDOW_FULLSCREEN);
}

void cf_app_set_title(const char* title)
{
	SDL_SetWindowTitle(app->window, title);
}

// Here for easy porting from SDL2.
SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	return SDL_CreateSurfaceFrom(width, height, SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask), pixels, pitch);
}

void cf_app_set_icon(const char* virtual_path_to_png)
{
	CF_Image img;
	if (is_error(cf_image_load_png(virtual_path_to_png, &img))) {
		fprintf(stderr, "Unable to open icon png file %s.", virtual_path_to_png);
		return;
	}
	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(img.pix, img.w, img.h, 32, img.w * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	SDL_SetWindowIcon(app->window, icon);
	SDL_DestroySurface(icon);
	cf_image_free(&img);
}

float cf_app_get_framerate()
{
	return 1.0f / CF_DELTA_TIME;
}

#ifndef CF_FRAMERATE_SMOOTHING
#define CF_FRAMERATE_SMOOTHING 60.0f
#endif

float cf_app_get_smoothed_framerate()
{
	static float fps = 0;
	fps = cf_lerp(fps, 1.0f / CF_DELTA_TIME, 1 / CF_FRAMERATE_SMOOTHING);
	return fps;
}

void* cf_app_init_imgui()
{
	if (!app->gfx_enabled) return NULL;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	app->using_imgui = true;

	cf_imgui_init();

	return (void*)::ImGui::GetCurrentContext();
}

CF_PowerInfo cf_app_power_info()
{
	CF_PowerInfo info;
	SDL_PowerState state = SDL_GetPowerInfo(&info.seconds_left, &info.percentage_left);
	switch (state) {
	case SDL_POWERSTATE_ERROR: info.state = CF_POWER_STATE_ERROR;
	case SDL_POWERSTATE_UNKNOWN: info.state = CF_POWER_STATE_UNKNOWN;
	case SDL_POWERSTATE_ON_BATTERY: info.state = CF_POWER_STATE_ON_BATTERY;
	case SDL_POWERSTATE_NO_BATTERY: info.state = CF_POWER_STATE_NO_BATTERY;
	case SDL_POWERSTATE_CHARGING: info.state = CF_POWER_STATE_CHARGING;
	case SDL_POWERSTATE_CHARGED: info.state = CF_POWER_STATE_CHARGED;
	}
	return info;
}

void cf_default_assert(bool expr, const char* message, const char* file, int line)
{
	if (!expr) {
		fprintf(stderr, "CF_ASSERT(%s) : %s, line %d\n", message, file, line);
#ifdef _MSC_VER
		__debugbreak();
#endif
	}
}

cf_assert_fn* g_assert_fn = cf_default_assert;

void cf_set_assert_handler(cf_assert_fn* assert_fn)
{
	g_assert_fn = assert_fn;
}
