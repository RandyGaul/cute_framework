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
#include <cute_crypto_utils.h>
#include <cute_file_system.h>
#include <cute_file_system_utils.h>
#include <cute_net.h>
#include <cute_c_runtime.h>
#include <cute_kv.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>
#include <internal/cute_net_internal.h>
#include <internal/cute_crypto_internal.h>
#include <internal/cute_audio_internal.h>
#include <internal/cute_input_internal.h>
#include <internal/cute_dx11.h>
#include <internal/cute_font_internal.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#ifdef CUTE_WINDOWS
#	include <SDL2/SDL_syswm.h>
#endif

#define CUTE_SOUND_FORCE_SDL
#include <cute/cute_sound.h>

#include <imgui/imgui.h>

#ifdef SOKOL_GLCORE33
#	include <glad/glad.h>
#endif

#define SOKOL_IMPL
#ifdef SOKOL_D3D11
#	define SOKOL_D3D11
#	define D3D11_NO_HELPERS
#endif
#include <sokol/sokol_gfx.h>

#define SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include <internal/imgui/sokol_imgui.h>
#include <internal/imgui/imgui_impl_sdl.h>

#include <shaders/upscale_shader.h>

namespace cute
{

// TODO: Refactor to use error_t reporting.

app_t* app_make(const char* window_title, int x, int y, int w, int h, uint32_t options, const char* argv0, void* user_allocator_context)
{
	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER)) {
		return NULL;
	}

	Uint32 flags = 0;
	if (options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT) flags |= SDL_WINDOW_OPENGL;
	if (options & CUTE_APP_OPTIONS_FULLSCREEN) flags |= SDL_WINDOW_FULLSCREEN;
	if (options & CUTE_APP_OPTIONS_RESIZABLE) flags |= SDL_WINDOW_RESIZABLE;
	if (options & CUTE_APP_OPTIONS_HIDDEN) flags |= (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);

	SDL_Window* window;
	if (options & CUTE_APP_OPTIONS_WINDOW_POS_CENTERED) {
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	} else {
		window = SDL_CreateWindow(window_title, x, y, w, h, flags);
	}
	app_t* app = (app_t*)CUTE_ALLOC(sizeof(app_t), user_allocator_context);
	CUTE_PLACEMENT_NEW(app) app_t;
	app->options = options;
	app->window = window;
	app->mem_ctx = user_allocator_context;
	app->w = w;
	app->h = h;
	app->x = x;
	app->y = y;
	app->offscreen_w = w;
	app->offscreen_h = h;

#ifdef CUTE_WINDOWS
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	app->platform_handle = hwnd;
#else
	void* hwnd = NULL;
#endif

	if (options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		app->gfx_enabled = true;
	}

	if (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		app->gfx_enabled = true;
	}

	if ((options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) | (options & CUTE_APP_OPTIONS_OPENG_GL_ES_CONTEXT)) {
		SDL_GL_SetSwapInterval(0);
		SDL_GL_CreateContext(window);
#ifdef __glad_h_
		gladLoadGLLoader(SDL_GL_GetProcAddress);
#endif
		CUTE_MEMSET(&app->gfx_ctx_params, 0, sizeof(app->gfx_ctx_params));
		app->gfx_ctx_params.color_format = SG_PIXELFORMAT_RGBA8;
		app->gfx_ctx_params.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
		sg_desc params = { 0 };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
		font_init(app);
	}

	if (options & CUTE_APP_OPTIONS_D3D11_CONTEXT) {
		dx11_init(hwnd, w, h, 1);
		app->gfx_ctx_params = dx11_get_context();
		sg_desc params = { 0 };
		params.context = app->gfx_ctx_params;
		sg_setup(params);
		app->gfx_enabled = true;
		font_init(app);
	}

	int num_threads_to_spawn = core_count() - 1;
	if (num_threads_to_spawn) {
		app->threadpool = threadpool_create(num_threads_to_spawn, user_allocator_context);
	}

	if (file_system_init(argv0).is_error()) {
		CUTE_ASSERT(0);
	}

	strpool_config_t strpool_config = strpool_default_config;
	strpool_config.memctx = user_allocator_context;
	strpool_init(&app->strpool_instance, &strpool_config);
	app->strpool = &app->strpool_instance;

	return app;
}

void app_destroy(app_t* app)
{
	strpool_term(app->strpool);
	if (app->using_imgui) {
		simgui_shutdown();
		ImGui_ImplSDL2_Shutdown();
		app->using_imgui = false;
	}
	if (app->gfx_enabled) {
		sg_shutdown();
		dx11_shutdown();
	}
	if (app->cute_sound) cs_shutdown_context(app->cute_sound);
	SDL_DestroyWindow(app->window);
	SDL_Quit();
	cute_threadpool_destroy(app->threadpool);
	audio_system_destroy(app->audio_system);
	int schema_count = app->entity_parsed_schemas.count();
	kv_t** schemas = app->entity_parsed_schemas.items();
	for (int i = 0; i < schema_count; ++i) kv_destroy(schemas[i]);
	app->~app_t();
	CUTE_FREE(app, app->mem_ctx);
	file_system_destroy();
}

bool app_is_running(app_t* app)
{
	return app->running;
}

void app_stop_running(app_t* app)
{
	app->running = 0;
}

void app_update(app_t* app, float dt)
{
	app->dt = dt;
	pump_input_msgs(app);
	if (app->audio_system) audio_system_update(app->audio_system, dt);
	if (app->using_imgui) {
		simgui_new_frame(app->w, app->h, dt);
		ImGui_ImplSDL2_NewFrame(app->window);
	}

	sg_pass_action pass_action = { 0 };
	pass_action.colors[0] = { SG_ACTION_CLEAR, { 0.4f, 0.65f, 0.7f, 1.0f } };
	if (app->offscreen_enabled) {
		sg_begin_pass(app->offscreen_pass, pass_action);
	} else {
		sg_begin_default_pass(pass_action, app->w, app->h);
	}
}

static void s_imgui_present(app_t* app)
{
	if (app->using_imgui) {
		ImGui::EndFrame();
		ImGui::Render();
		simgui_render();
	}
}

void app_present(app_t* app)
{
	if (app->offscreen_enabled) {
		sg_end_pass();

		sg_bindings bind = { 0 };
		bind.vertex_buffers[0] = app->quad;
		bind.fs_images[0] = app->offscreen_color_buffer;

		sg_pass_action clear_to_black = { 0 };
		clear_to_black.colors[0] = { SG_ACTION_CLEAR, { 0.0f, 0.0f, 0.0f, 1.0f } };
		sg_begin_default_pass(&clear_to_black, app->w, app->h);
		sg_apply_pipeline(app->offscreen_to_screen_pip);
		sg_apply_bindings(bind);
		upscale_vs_params_t vs_params = { app->upscale };
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
		sg_draw(0, 6, 1);
		s_imgui_present(app);
		sg_end_pass();
	} else {
		s_imgui_present(app);
		sg_end_pass();
	}

	sg_commit();
	dx11_present();
	if (app->options & CUTE_APP_OPTIONS_OPENGL_CONTEXT) {
		SDL_GL_SwapWindow(app->window);
	}

	// Triple buffering on the font vertices.
	app->font_buffer.advance();
}

// TODO - Move these init functions into audio/net headers.

error_t app_init_net(app_t* app)
{
	error_t err = crypto_init();
	if (err.is_error()) return err;
	return net_init();
}

error_t app_init_audio(app_t* app, int max_simultaneous_sounds)
{
	app->cute_sound = cs_make_context(NULL, 44100, 1024, 0, app->mem_ctx);
	if (app->cute_sound) {
		cs_spawn_mix_thread(app->cute_sound);
		app->audio_system = audio_system_make(max_simultaneous_sounds, app->mem_ctx);
		return error_success();
	} else {
		return error_failure(cs_error_reason);
	}
}

ImGuiContext* app_init_imgui(app_t* app)
{
	if (!app->gfx_enabled) return NULL;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	app->using_imgui = true;

	ImGui::StyleColorsDark();
	ImGui_SDL2_Init(app->window);
	simgui_desc_t imgui_params = { 0 };
	simgui_setup(imgui_params);

	return ::ImGui::GetCurrentContext();
}

static void s_quad(float x, float y, float sx, float sy, float* out)
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

static float s_max_scaling_factor(app_t* app)
{
	float scale = 1.0f;
	int i = 0;
	while (1)
	{
		float new_scale = scale + 1;
		int can_scale_x = new_scale * app->offscreen_w <= app->w;
		int can_scale_y = new_scale * app->offscreen_h <= app->h;
		if (can_scale_x && can_scale_y) {
			++i;
			scale = new_scale;
		} else {
			break;
		}
	}
	return scale;
}

static float s_enforce_scale(upscale_t upscaling, float scale)
{
	switch (upscaling) {
	case UPSCALE_PIXEL_PERFECT_AT_LEAST_2X: return max(scale, 2.0f);
	case UPSCALE_PIXEL_PERFECT_AT_LEAST_3X: return max(scale, 3.0f);
	case UPSCALE_PIXEL_PERFECT_AT_LEAST_4X: return max(scale, 4.0f);
	case UPSCALE_PIXEL_PERFECT_AUTO: // Fall-thru.
	case UPSCALE_STRETCH: // Fall-thru.
	default: return scale;
	}
}

error_t app_init_upscaling(app_t* app, upscale_t upscaling, int offscreen_w, int offscreen_h)
{
	if (app->offscreen_enabled) {
		error_failure("Upscaling is already enabled.");
	}

	app->offscreen_enabled = true;
	app->upscaling = upscaling;
	app->offscreen_w = offscreen_w;
	app->offscreen_h = offscreen_h;

	// Create offscreen buffers.
	sg_image_desc buffer_params = { 0 };
	buffer_params.render_target = true;
	buffer_params.width = offscreen_w;
	buffer_params.height = offscreen_h;
	buffer_params.pixel_format = app->gfx_ctx_params.color_format;
	app->offscreen_color_buffer = sg_make_image(buffer_params);
	if (app->offscreen_color_buffer.id == SG_INVALID_ID) return error_failure("Unable to create offscreen color buffer.");
	buffer_params.pixel_format = app->gfx_ctx_params.depth_format;
	app->offscreen_depth_buffer = sg_make_image(buffer_params);
	if (app->offscreen_depth_buffer.id == SG_INVALID_ID) return error_failure("Unable to create offscreen depth buffer.");

	// Define pass to reference offscreen buffers.
	sg_pass_desc pass_params = { 0 };
	pass_params.color_attachments[0].image = app->offscreen_color_buffer;
	pass_params.depth_stencil_attachment.image = app->offscreen_depth_buffer;
	app->offscreen_pass = sg_make_pass(pass_params);
	if (app->offscreen_pass.id == SG_INVALID_ID) return error_failure("Unable to create offscreen pass.");

	// Initialize static geometry for the offscreen quad.
	float quad[4 * 6];
	s_quad(0, 0, 2, 2, quad);
	sg_buffer_desc quad_params = { 0 };
	quad_params.size = sizeof(quad);
	quad_params.content = quad;
	app->quad = sg_make_buffer(quad_params);
	if (app->quad.id == SG_INVALID_ID) return error_failure("Unable create static quad buffer.");

	// Setup upscaling shader, to draw the offscreen buffer onto the screen as a textured quad.
	app->offscreen_shader = sg_make_shader(upscale_upscale_shader_desc());
	if (app->offscreen_shader.id == SG_INVALID_ID) return error_failure("Unable create offscreen shader.");

	float scale = s_max_scaling_factor(app);
	scale = s_enforce_scale(app->upscaling, scale);
	app->upscale = { scale * (float)app->offscreen_w / (float)app->w, scale * (float)app->offscreen_h / (float)app->h };

	// Setup offscreen rendering pipeline, to draw the offscreen buffer onto the screen.
	sg_pipeline_desc params = { 0 };
	params.layout.buffers[0].stride = sizeof(v2) * 2;
	params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	params.layout.buffers[0].step_rate = 1;
	params.layout.attrs[0].buffer_index = 0;
	params.layout.attrs[0].offset = 0;
	params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	params.layout.attrs[1].buffer_index = 0;
	params.layout.attrs[1].offset = sizeof(v2);
	params.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
	params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	params.shader = app->offscreen_shader;
	app->offscreen_to_screen_pip = sg_make_pipeline(params);
	if (app->offscreen_to_screen_pip.id == SG_INVALID_ID) return error_failure("Unable create offscreen pipeline.");

	return error_success();
}

void app_offscreen_size(app_t* app, int* offscreen_w, int* offscreen_h)
{
	*offscreen_w = app->offscreen_w;
	*offscreen_h = app->offscreen_h;
}

}
