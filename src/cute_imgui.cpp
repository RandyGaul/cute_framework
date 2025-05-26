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

static void s_make_buffers(int vertex_count, int index_count)
{
	if (app->imgui_vbuf) {
		SDL_ReleaseGPUBuffer(app->device, app->imgui_vbuf);
	}
	if (app->imgui_vtbuf) {
		SDL_ReleaseGPUTransferBuffer(app->device, app->imgui_vtbuf);
	}
	if (app->imgui_ibuf) {
		SDL_ReleaseGPUBuffer(app->device, app->imgui_ibuf);
	}
	if (app->imgui_itbuf) {
		SDL_ReleaseGPUTransferBuffer(app->device, app->imgui_itbuf);
	}

	{
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = (Uint32)(sizeof(ImDrawVert) * vertex_count),
			.props = 0
		};
		app->imgui_vbuf = SDL_CreateGPUBuffer(app->device, &buf_info);
		CF_ASSERT(app->imgui_vbuf);
	}

	{
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)(sizeof(ImDrawVert) * vertex_count),
			.props = 0,
		};
		app->imgui_vtbuf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
		CF_ASSERT(app->imgui_vtbuf);
	}

	{
		SDL_GPUBufferCreateInfo buf_info = {
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = (Uint32)(sizeof(ImDrawIdx) * index_count),
			.props = 0
		};
		app->imgui_ibuf = SDL_CreateGPUBuffer(app->device, &buf_info);
		CF_ASSERT(app->imgui_ibuf);
	}

	{
		SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (Uint32)(sizeof(ImDrawIdx) * index_count),
			.props = 0,
		};
		app->imgui_itbuf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
		CF_ASSERT(app->imgui_itbuf);
	}
}

void cf_imgui_init()
{
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiBackendFlags_RendererHasVtxOffset;     // Enable Keyboard Controls
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
	SDL_ReleaseGPUSampler(app->device, app->imgui_sampler);
	SDL_ReleaseGPUBuffer(app->device, app->imgui_vbuf);
	SDL_ReleaseGPUBuffer(app->device, app->imgui_ibuf);
	SDL_ReleaseGPUTransferBuffer(app->device, app->imgui_vtbuf);
	SDL_ReleaseGPUTransferBuffer(app->device, app->imgui_itbuf);
	SDL_ReleaseGPUGraphicsPipeline(app->device, app->imgui_pip);
	SDL_ReleaseGPUTexture(app->device, app->imgui_font_tex);
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
		target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
		target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		target_info.store_op = SDL_GPU_STOREOP_STORE;
		target_info.mip_level = 0;
		target_info.layer_or_depth_plane = 0;
		target_info.cycle = false;
		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(app->cmd, &target_info, 1, nullptr);

		// Render ImGui
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, app->cmd, render_pass);

		SDL_EndGPURenderPass(render_pass);
	}
}

void cf_imgui_draw_old(SDL_GPUTexture* swapchain_texture)
{
	ImDrawData* draw_data = ImGui::GetDrawData();
	if (draw_data->TotalVtxCount == 0)
		return;

	// Resize buffers if necessary.
	int vertex_count = 0;
	int index_count = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		ImDrawList* cmdList = draw_data->CmdLists[n];
		vertex_count += cmdList->VtxBuffer.Size;
		index_count += cmdList->IdxBuffer.Size;
	}
	if (vertex_count > app->imgui_vertex_count || index_count > app->imgui_index_count) {
		s_make_buffers(vertex_count * 2, index_count * 2);
	}

	// Map data onto GPU driver staging space.
	uint8_t* vertices = (uint8_t*)SDL_MapGPUTransferBuffer(app->device, app->imgui_vtbuf, true);
	uint8_t* indices = (uint8_t*)SDL_MapGPUTransferBuffer(app->device, app->imgui_itbuf, true);
		for (int n = 0; n < draw_data->CmdListsCount; n++) {
			ImDrawList* cmdList = draw_data->CmdLists[n];

			int vertex_sz = (int)(cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			CF_MEMCPY(vertices, cmdList->VtxBuffer.Data, vertex_sz);
			vertices += vertex_sz;

			int index_sz = (int)(cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			CF_MEMCPY(indices, cmdList->IdxBuffer.Data, index_sz);
			indices += index_sz;
		}
	SDL_UnmapGPUTransferBuffer(app->device, app->imgui_vtbuf);
	SDL_UnmapGPUTransferBuffer(app->device, app->imgui_itbuf);

	// Submit commands to upload buffers to the GPU.
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(app->cmd);
		SDL_GPUTransferBufferLocation src;
		src.offset = 0;
		src.transfer_buffer = app->imgui_vtbuf;
		SDL_GPUBufferRegion region;
		region.buffer = app->imgui_vbuf;
		region.offset = 0;
		region.size = (uint32_t)(draw_data->TotalVtxCount * sizeof(ImDrawVert));
		SDL_UploadToGPUBuffer(copy_pass, &src, &region, true);

		src.transfer_buffer = app->imgui_itbuf;
		region.buffer = app->imgui_ibuf;
		region.offset = 0;
		region.size = (uint32_t)(draw_data->TotalIdxCount * sizeof(ImDrawIdx));
		SDL_UploadToGPUBuffer(copy_pass, &src, &region, false);
	SDL_EndGPUCopyPass(copy_pass);

	// Setup rendering commands.
	SDL_GPUColorTargetInfo color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	color_info.texture = swapchain_texture;
	color_info.load_op = SDL_GPU_LOADOP_LOAD;
	color_info.store_op = SDL_GPU_STOREOP_STORE;

	SDL_GPUDepthStencilTargetInfo depth_stencil_info;
	CF_MEMSET(&depth_stencil_info, 0, sizeof(depth_stencil_info));
	//depth_stencil_info.texture = app->depth_buffer;
	depth_stencil_info.cycle = true;
	depth_stencil_info.clear_depth = 0;
	depth_stencil_info.clear_stencil = 0;
	depth_stencil_info.load_op = SDL_GPU_LOADOP_CLEAR;
	depth_stencil_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_stencil_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
	depth_stencil_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

	// Submit the actual rendering commands.
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(app->cmd, &color_info, 1, NULL);
	SDL_BindGPUGraphicsPipeline(pass, app->imgui_pip);

	SDL_GPUViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = draw_data->DisplaySize.x,
		.h = draw_data->DisplaySize.y,
		.min_depth = 0.0f,
		.max_depth = 1.0f,
	};
	SDL_SetGPUViewport(pass, &viewport);

	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	float mvp[4][4] = {
		{ 2.0f / (R - L),     0.0f,              0.0f,  0.0f },
		{ 0.0f,               2.0f / (T - B),    0.0f,  0.0f },
		{ 0.0f,               0.0f,              0.5f,  0.0f },
		{ (R + L) / (L - R),  (T + B) / (B - T), 0.5f,  1.0f },
	};
	SDL_PushGPUVertexUniformData(app->cmd, 0, mvp, sizeof(mvp));

	SDL_GPUBufferBinding vertexBufferBinding = {
		.buffer = app->imgui_vbuf,
		.offset = 0
	};
	SDL_BindGPUVertexBuffers(pass, 0, &vertexBufferBinding, 1);
	SDL_GPUBufferBinding indexBufferBinding = {
		.buffer = app->imgui_ibuf,
		.offset = 0
	};
	SDL_BindGPUIndexBuffer(pass, &indexBufferBinding, sizeof(ImDrawIdx) == sizeof(uint16_t) ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);

	uint32_t idx_offset = 0;
	uint32_t vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr) {
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
					SDL_BindGPUGraphicsPipeline(pass, app->imgui_pip);
				} else {
					pcmd->UserCallback(cmd_list, pcmd);
				}
			} else {
				// Project scissor/clipping rectangles into frame buffer space.
				ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
					continue;
				}

				SDL_Rect scissorDesc = {
					.x = (Sint32)clip_min.x,
					.y = (Sint32)clip_min.y,
					.w = (Sint32)(clip_max.x - clip_min.x),
					.h = (Sint32)(clip_max.y - clip_min.y)
				};
				SDL_SetGPUScissor(pass, &scissorDesc);

				SDL_GPUTextureSamplerBinding samplerBinding = {
					.texture = (SDL_GPUTexture*)pcmd->GetTexID(),
					.sampler = app->imgui_sampler
				};

				SDL_BindGPUFragmentSamplers(pass, 0, &samplerBinding, 1);
				SDL_DrawGPUIndexedPrimitives(pass, pcmd->ElemCount, 1, idx_offset + pcmd->IdxOffset, vtx_offset + pcmd->VtxOffset, 0);
			}
		}

		idx_offset += cmd_list->IdxBuffer.Size;
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	SDL_EndGPURenderPass(pass);
}
