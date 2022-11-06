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
#include <cute_file_system_utils.h>
#include <cute_c_runtime.h>
#include <cute_kv.h>
#include <cute_font.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>
#include <internal/cute_audio_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/cute_dx11.h>
#include <internal/cute_font_internal.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifdef CUTE_WINDOWS
#	include <SDL_syswm.h>
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

#include <shaders/upscale_shader.h>

cf_app_t* cf_app;

using namespace cute;

cf_result_t cf_make_app(const char* window_title, int x, int y, int w, int h, int options, const char* argv0, void* user_allocator_context)
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
	cf_app_t* app = (cf_app_t*)CUTE_ALLOC(sizeof(cf_app_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(app) cf_app_t;
	app->options = options;
	app->window = window;
	app->mem_ctx = user_allocator_context;
	app->w = w;
	app->h = h;
	app->x = x;
	app->y = y;
	app->offscreen_w = w;
	app->offscreen_h = h;
	cf_app = app;

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
			CUTE_FREE(app, user_allocator_context);
			return cf_result_error("Unable to create OpenGL context.");
		}
		CUTE_MEMSET(&app->gfx_ctx_params, 0, sizeof(app->gfx_ctx_params));
		app->gfx_ctx_params.color_format = SG_PIXELFORMAT_RGBA8;
		app->gfx_ctx_params.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
		sg_desc params = { 0 };
		params.context = cf_app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
		cf_font_init();
	}

	if (options & APP_OPTIONS_D3D11_CONTEXT) {
		cf_dx11_init(hwnd, w, h, 1);
		app->gfx_ctx_params = cf_dx11_get_context();
		sg_desc params = { 0 };
		params.context = cf_app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
		cf_font_init();
	}

	int num_threads_to_spawn = cf_core_count() - 1;
	if (num_threads_to_spawn) {
		app->threadpool = cf_make_threadpool(num_threads_to_spawn, user_allocator_context);
	}

	cf_result_t err = cf_file_system_init(argv0);
	if (cf_is_error(err)) {
		CUTE_ASSERT(0);
	} else if (!(options & APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT)) {
		// Put the base directory (the path to the exe) onto the file system search path.
		cf_file_system_mount(cf_file_system_get_base_dir(), "", true);
	}

	app->strpool = cf_make_strpool(NULL);

	return cf_result_success();
}

void cf_destroy_app()
{
	cf_destroy_strpool(cf_app->strpool);
	if (cf_app->using_imgui) {
		ImGui_ImplSDL2_Shutdown();
		simgui_shutdown();
		sg_imgui_discard(&cf_app->sg_imgui);
		cf_app->using_imgui = false;
	}
	if (cf_app->gfx_enabled) {
		sg_shutdown();
		cf_dx11_shutdown();
	}
	if (cf_app->cute_sound) cs_shutdown_context(cf_app->cute_sound);
	SDL_DestroyWindow(cf_app->window);
	SDL_Quit();
	destroy_threadpool(cf_app->threadpool);
	cf_audio_system_destroy(cf_app->audio_system);
	int schema_count = cf_app->entity_parsed_schemas.count();
	cf_kv_t** schemas = cf_app->entity_parsed_schemas.items();
	for (int i = 0; i < schema_count; ++i) cf_destroy_kv(schemas[i]);
	if (cf_app->ase_cache) {
		cf_destroy_aseprite_cache(cf_app->ase_cache);
		cf_destroy_batch(cf_app->ase_batch);
	}
	if (cf_app->png_cache) {
		cf_destroy_png_cache(cf_app->png_cache);
		cf_destroy_batch(cf_app->png_batch);
	}
	if (cf_app->courier_new) {
		cf_font_free((cf_font_t*)cf_app->courier_new);
	}
	cf_app->~cf_app_t();
	CUTE_FREE(cf_app, cf_app->mem_ctx);
	cf_file_system_destroy();
}

bool cf_app_is_running()
{
	return cf_app->running;
}

void cf_app_stop_running()
{
	cf_app->running = 0;
}

void cf_app_update(float dt)
{
	cf_app->dt = dt;
	cf_pump_input_msgs();
	if (cf_app->audio_system) {
		cf_audio_system_update(cf_app->audio_system, dt);
#ifdef CUTE_EMSCRIPTEN
		cf_app_do_mixing(cf_app);
#endif // CUTE_EMSCRIPTEN
	}
	if (cf_app->using_imgui) {
		simgui_new_frame(cf_app->w, cf_app->h, dt);
		ImGui_ImplSDL2_NewFrame(cf_app->window);
	}

	if (cf_app->gfx_enabled) {
		sg_pass_action pass_action = { 0 };
		pass_action.colors[0] = { SG_ACTION_CLEAR, { 0.4f, 0.65f, 0.7f, 1.0f } };
		if (cf_app->offscreen_enabled) {
			sg_begin_pass(cf_app->offscreen_pass, pass_action);
		} else {
			sg_begin_default_pass(pass_action, cf_app->w, cf_app->h);
		}
	}

	if (cf_app->ase_batch) {
		cf_batch_update(cf_app->ase_batch);
	}
}

static void cf_s_imgui_present()
{
	if (cf_app->using_imgui) {
		ImGui::EndFrame();
		ImGui::Render();
		simgui_render();
	}
}

sg_image cf_app_get_offscreen_buffer()
{
	sg_image result = { 0 };
	if (cf_app->offscreen_enabled) {
		if (!cf_app->fetched_offscreen) {
			sg_end_pass();
			cf_app->fetched_offscreen = true;
		}

		result = cf_app->offscreen_color_buffer;
		cf_app->fetched_offscreen = true;
	}
	return result;
}

void cf_app_present(bool draw_offscreen_buffer)
{
	if (cf_app->offscreen_enabled) {
		sg_bindings bind = { 0 };
		bind.vertex_buffers[0] = cf_app->quad;
		bind.fs_images[0] = cf_app_get_offscreen_buffer();

		sg_pass_action clear_to_black = { 0 };
		clear_to_black.colors[0] = { SG_ACTION_CLEAR, { 0.0f, 0.0f, 0.0f, 1.0f } };
		sg_begin_default_pass(&clear_to_black, cf_app->w, cf_app->h);
		if (draw_offscreen_buffer) {
			sg_apply_pipeline(cf_app->offscreen_to_screen_pip);
			sg_apply_bindings(bind);
			upscale_vs_params_t vs_params = { cf_app->upscale };
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE(vs_params));
			upscale_fs_params_t fs_params = { cf_V2((float)cf_app->offscreen_w, (float)cf_app->offscreen_h) };
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE(fs_params));
			sg_draw(0, 6, 1);
		}
		if (cf_app->using_imgui) {
			sg_imgui_draw(&cf_app->sg_imgui);
			cf_s_imgui_present();
		}
		sg_end_pass();
	} else {
		if (cf_app->using_imgui) {
			sg_imgui_draw(&cf_app->sg_imgui);
			cf_s_imgui_present();
		}
		sg_end_pass();
	}

	sg_commit();
	cf_dx11_present();
	if (cf_app->options & APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SwapWindow(cf_app->window);
	}

	cf_app->fetched_offscreen = false;
}

cf_result_t cf_app_init_audio(bool spawn_mix_thread, int max_simultaneous_sounds)
{
	int more_on_emscripten = 1;
#ifdef CUTE_EMSCRIPTEN
	more_on_emscripten = 4;
#endif
	cf_app->cute_sound = cs_make_context(NULL, 44100, 1024 * more_on_emscripten, 0, cf_app->mem_ctx);
	if (cf_app->cute_sound) {
#ifndef CUTE_EMSCRIPTEN
		if (spawn_mix_thread) {
			cs_spawn_mix_thread(cf_app->cute_sound);
			cf_app->spawned_mix_thread = true;
		}
#endif // CUTE_EMSCRIPTEN
		cf_app->audio_system = cf_audio_system_make(max_simultaneous_sounds, cf_app->mem_ctx);
		return cf_result_success();
	} else {
		return cf_result_error(cs_error_reason);
	}
}

void cf_app_do_mixing()
{
#ifdef CUTE_EMSCRIPTEN
	cs_mix(cf_app->cute_sound);
#else
	if (cf_app->spawned_mix_thread) {
		cs_mix(cf_app->cute_sound);
	}
#endif // CUTE_EMSCRIPTEN
}

ImGuiContext* cf_app_init_imgui(bool no_default_font)
{
	if (!cf_app->gfx_enabled) return NULL;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	cf_app->using_imgui = true;

	ImGui::StyleColorsDark();
	
	sg_backend backend = sg_query_backend();
	switch (backend) {
		case SG_BACKEND_GLCORE33: ImGui_ImplSDL2_InitForOpenGL(cf_app->window, NULL); break;
		case SG_BACKEND_GLES2: ImGui_ImplSDL2_InitForOpenGL(cf_app->window, NULL); break;
		case SG_BACKEND_GLES3: ImGui_ImplSDL2_InitForOpenGL(cf_app->window, NULL); break;
		case SG_BACKEND_D3D11: ImGui_ImplSDL2_InitForD3D(cf_app->window); break;
		case SG_BACKEND_METAL_IOS: ImGui_ImplSDL2_InitForMetal(cf_app->window); break;
		case SG_BACKEND_METAL_MACOS: ImGui_ImplSDL2_InitForMetal(cf_app->window); break;
		case SG_BACKEND_METAL_SIMULATOR: ImGui_ImplSDL2_InitForMetal(cf_app->window); break;
		case SG_BACKEND_WGPU: ImGui_ImplSDL2_InitForOpenGL(cf_app->window, NULL); break;
	}
	
	simgui_desc_t imgui_params = { 0 };
	imgui_params.no_default_font = no_default_font;
	imgui_params.ini_filename = "imgui.ini";
	simgui_setup(imgui_params);
	sg_imgui_init(&cf_app->sg_imgui);

	return ::ImGui::GetCurrentContext();
}

sg_imgui_t* cf_app_get_sokol_imgui()
{
	if (!cf_app->using_imgui) return NULL;
	return &cf_app->sg_imgui;
}

cf_strpool_t* cf_app_get_strpool()
{
	return cf_app->strpool;
}

static void cf_s_quad(float x, float y, float sx, float sy, float* out)
{
	struct vertex_t
	{
		float x, y;
		float u, v;
	};

	vertex_t quad[6];

	quad[0].x = -0.5f; quad[0].y = 0.5f;  quad[0].u = 0; quad[0].v = 0;
	quad[1].x = 0.5f;  quad[1].y = -0.5f; quad[1].u = 1; quad[1].v = 1;
	quad[2].x = 0.5f;  quad[2].y = 0.5f;  quad[2].u = 1; quad[2].v = 0;

	quad[3].x = -0.5f; quad[3].y =  0.5f; quad[3].u = 0; quad[3].v = 0;
	quad[4].x = -0.5f; quad[4].y = -0.5f; quad[4].u = 0; quad[4].v = 1;
	quad[5].x = 0.5f;  quad[5].y = -0.5f; quad[5].u = 1; quad[5].v = 1;

	for (int i = 0; i < 6; ++i)
	{
		quad[i].x = quad[i].x * sx + x;
		quad[i].y = quad[i].y * sy + y;
	}

	CUTE_MEMCPY(out, quad, sizeof(quad));
}

static cf_result_t s_create_offscreen_buffers(int w, int h)
{
	sg_image_desc buffer_params = { 0 };
	buffer_params.render_target = true;
	buffer_params.width = w;
	buffer_params.height = h;
	buffer_params.pixel_format = cf_app->gfx_ctx_params.color_format;
	cf_app->offscreen_color_buffer = sg_make_image(buffer_params);
	if (cf_app->offscreen_color_buffer.id == SG_INVALID_ID) return cf_result_error("Unable to create offscreen color buffer.");
	buffer_params.pixel_format = cf_app->gfx_ctx_params.depth_format;
	cf_app->offscreen_depth_buffer = sg_make_image(buffer_params);
	if (cf_app->offscreen_depth_buffer.id == SG_INVALID_ID) return cf_result_error("Unable to create offscreen depth buffer.");

	sg_pass_desc pass_params = { 0 };
	pass_params.color_attachments[0].image = cf_app->offscreen_color_buffer;
	pass_params.depth_stencil_attachment.image = cf_app->offscreen_depth_buffer;
	cf_app->offscreen_pass = sg_make_pass(pass_params);
	if (cf_app->offscreen_pass.id == SG_INVALID_ID) return cf_result_error("Unable to create offscreen pass.");

	return cf_result_success();
}

cf_result_t cf_app_set_offscreen_buffer(int offscreen_w, int offscreen_h)
{
	if (cf_app->offscreen_enabled) {
		if (offscreen_w != cf_app->offscreen_w && offscreen_h != cf_app->offscreen_h) {
			sg_destroy_image(cf_app->offscreen_color_buffer);
			sg_destroy_image(cf_app->offscreen_depth_buffer);
			sg_destroy_pass(cf_app->offscreen_pass);
			cf_result_t err = s_create_offscreen_buffers(offscreen_w, offscreen_h);
			if (cf_is_error(err)) return err;
		}
		return cf_result_success();
	}

	cf_app->offscreen_enabled = true;
	cf_app->offscreen_w = offscreen_w;
	cf_app->offscreen_h = offscreen_h;

	// Create offscreen buffers and pass.
	cf_result_t err = s_create_offscreen_buffers(offscreen_w, offscreen_h);
	if (cf_is_error(err)) return err;

	// Initialize static geometry for the offscreen quad.
	float quad[4 * 6];
	cf_s_quad(0, 0, 2, 2, quad);
	sg_buffer_desc quad_params = { 0 };
	quad_params.size = sizeof(quad);
	quad_params.data = SG_RANGE(quad);
	cf_app->quad = sg_make_buffer(quad_params);
	if (cf_app->quad.id == SG_INVALID_ID) return cf_result_error("Unable create static quad buffer.");

	// Setup upscaling shader, to draw the offscreen buffer onto the screen as a textured quad.
	cf_app->offscreen_shader = sg_make_shader(upscale_shd_shader_desc(sg_query_backend()));
	if (cf_app->offscreen_shader.id == SG_INVALID_ID) return cf_result_error("Unable create offscreen shader.");

	//app->upscale = { (float)app->offscreen_w / (float)app->w, (float)app->offscreen_h / (float)app->h };
	cf_app->upscale = cf_V2(1, 1);

	// Setup offscreen rendering pipeline, to draw the offscreen buffer onto the screen.
	sg_pipeline_desc params = { 0 };
	params.layout.buffers[0].stride = sizeof(cf_v2) * 2;
	params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	params.layout.buffers[0].step_rate = 1;
	params.layout.attrs[0].buffer_index = 0;
	params.layout.attrs[0].offset = 0;
	params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	params.layout.attrs[1].buffer_index = 0;
	params.layout.attrs[1].offset = sizeof(cf_v2);
	params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	params.shader = cf_app->offscreen_shader;
	cf_app->offscreen_to_screen_pip = sg_make_pipeline(params);
	if (cf_app->offscreen_to_screen_pip.id == SG_INVALID_ID) return cf_result_error("Unable create offscreen pipeline.");

	return cf_result_success();
}

void cf_app_offscreen_size(int* offscreen_w, int* offscreen_h)
{
	*offscreen_w = cf_app->offscreen_w;
	*offscreen_h = cf_app->offscreen_h;
}

cf_power_info_t cf_app_power_info()
{
	cf_power_info_t info;
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
