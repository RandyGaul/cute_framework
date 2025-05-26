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
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlgpu3.h>

#ifdef CF_RUNTIME_SHADER_COMPILATION
#include "cute_shader/builtin_shaders.h"
#else
#include "data/builtin_shaders_bytecode.h"
#endif

using namespace Cute;

void cf_imgui_init()
{
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplSDL3_InitForSDLGPU(app->window);
	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = app->device;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(app->device, app->window);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);
}

void cf_imgui_shutdown()
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
}

void cf_imgui_draw(SDL_GPUTexture* swapchain_texture) {
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImDrawData* draw_data = ImGui::GetDrawData();
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	if (swapchain_texture != nullptr && !is_minimized) {
		// This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
		Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, app->cmd); // TODO: Update function name to ImGui_ImplSDLGPU3_PrepareDrawData

		// Setup and start a render pass
		SDL_GPUColorTargetInfo target_info = {};
		target_info.texture = swapchain_texture;
		target_info.load_op = SDL_GPU_LOADOP_LOAD;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(app->cmd, &target_info, 1, nullptr);

		// Render ImGui
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, app->cmd, render_pass);

		SDL_EndGPURenderPass(render_pass);
	}
}
