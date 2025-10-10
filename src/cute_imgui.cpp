/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_defines.h>
#include <cute_c_runtime.h>
#include <cute_graphics.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_imgui_internal.h>
#include <internal/cute_graphics_internal.h>

#include <imgui/imgui.h>

using namespace Cute;

void cf_imgui_init()
{
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		ImGui_ImplOpenGL3_Init("#version 300 es");
		ImGui_ImplSDL3_InitForOpenGL(app->window, cf_gles_get_gl_context());
	} else {
#ifndef CF_EMSCRIPTEN
		SDL_GPUDevice* device = cf_sdlgpu_get_device();
		ImGui_ImplSDL3_InitForSDLGPU(app->window);
		ImGui_ImplSDLGPU3_InitInfo init_info = {};
		init_info.Device = device;
		init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, app->window);
		init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
		ImGui_ImplSDLGPU3_Init(&init_info);
#endif
	}
}

void cf_imgui_shutdown()
{
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		ImGui_ImplOpenGL3_Shutdown();
	} else {
		ImGui_ImplSDLGPU3_Shutdown();
	}
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void cf_imgui_draw()
{
	if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) {
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	} else {
#ifndef CF_EMSCRIPTEN
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

		SDL_GPUTexture* swapchain_tex = cf_sdlgpu_get_swapchain_texture();
		if (swapchain_tex != nullptr && !is_minimized) {
			// This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
			SDL_GPUCommandBuffer* cmd = cf_sdlgpu_get_command_buffer();
			ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);

			// Setup and start a render pass
			SDL_GPUColorTargetInfo target_info = {};
			target_info.texture = swapchain_tex;
			target_info.load_op = SDL_GPU_LOADOP_LOAD;
			target_info.store_op = SDL_GPU_STOREOP_STORE;
			SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &target_info, 1, nullptr);

			// Render ImGui
			ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, render_pass);

			SDL_EndGPURenderPass(render_pass);
		}
#endif
	}
}
