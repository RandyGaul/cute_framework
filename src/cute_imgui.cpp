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

using namespace Cute;

const char* s_imgui_vs = R"(
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 col;

layout (location = 0) out vec4 v_col;
layout (location = 1) out vec2 v_uv;

layout (set = 1, binding = 0) uniform uniform_block {
	mat4 ProjectionMatrix;
};

void main()
{
	v_col = col;
	v_uv  = uv;
	gl_Position = ProjectionMatrix * vec4(pos.xy, 0, 1);
}
)";

const char* s_imgui_fs = R"(
layout (location = 0) in vec4 v_col;
layout (location = 1) in vec2 v_uv;

layout(location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D u_image;

void main()
{
	vec4 color = v_col * texture(u_image, v_uv);
	result = color;
}
)";

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
	CF_Shader shader = cf_make_shader_from_source(s_imgui_vs, s_imgui_fs);
	SDL_GPUShader* vs = ((CF_ShaderInternal*)shader.id)->vs;
	SDL_GPUShader* fs = ((CF_ShaderInternal*)shader.id)->fs;

	SDL_GPUColorTargetDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	color_info.format = SDL_GetGPUSwapchainTextureFormat(app->device, app->window);
	color_info.blend_state.enable_blend = true;
	color_info.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	color_info.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	color_info.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	color_info.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	color_info.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_info.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_info.blend_state.color_write_mask = 0xF;

	SDL_GPUGraphicsPipelineCreateInfo pip_info;
	CF_MEMSET(&pip_info, 0, sizeof(pip_info));
	pip_info.vertex_shader = vs;
	pip_info.fragment_shader = fs;
	pip_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.multisample_state.sample_mask = 0xFFFF;
	pip_info.target_info.num_color_targets = 1;
	pip_info.target_info.color_target_descriptions = &color_info;
	pip_info.target_info.has_depth_stencil_target = false; // @TODO.
	pip_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	pip_info.depth_stencil_state.enable_depth_test = false;
	pip_info.depth_stencil_state.enable_depth_write = false;
	pip_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;

	SDL_GPUVertexBufferDescription vertex_buffer_description;
	CF_MEMSET(&vertex_buffer_description, 0, sizeof(vertex_buffer_description));
	vertex_buffer_description.slot = 0;
	vertex_buffer_description.pitch = sizeof(ImDrawVert);
	vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_description.instance_step_rate = 0;

	SDL_GPUVertexAttribute vertex_attributes[] = {
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = 0
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = sizeof(float) * 2,
		},
		{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
			.offset = sizeof(float) * 4,
		},
	};

	pip_info.vertex_input_state = SDL_GPUVertexInputState {
		.vertex_buffer_descriptions = &vertex_buffer_description,
		.num_vertex_buffers = 1,
		.vertex_attributes = vertex_attributes,
		.num_vertex_attributes = 3,
	};

	pip_info.rasterizer_state = SDL_GPURasterizerState {
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = SDL_GPU_CULLMODE_NONE,
		.front_face = {},
		.depth_bias_constant_factor = {},
		.depth_bias_clamp = {},
		.depth_bias_slope_factor = {},
		.enable_depth_bias = false,
	};

	app->imgui_pip = SDL_CreateGPUGraphicsPipeline(app->device, &pip_info);
	CF_ASSERT(app->imgui_pip);
	SDL_ReleaseGPUShader(app->device, vs);
	SDL_ReleaseGPUShader(app->device, fs);

	int vertex_count = 1024 * 64;
	int index_count = 1024 * 64;
	s_make_buffers(vertex_count, index_count);

	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};

	app->imgui_sampler = SDL_CreateGPUSampler(app->device, &sampler_info);
	CF_ASSERT(app->imgui_sampler);

	unsigned char* pixels;
	int width, height;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	int sz = (int)(width * height * sizeof(uint8_t) * 4);

	SDL_GPUTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = (uint32_t)width,
		.height = (uint32_t)height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = {},
	};

	SDL_GPUTexture* font_tex = SDL_CreateGPUTexture(app->device, &texture_info);
	CF_ASSERT(font_tex);
	app->imgui_font_tex = font_tex;

	SDL_GPUTextureRegion region;
	CF_MEMSET(&region, 0, sizeof(region));
	region.texture = font_tex;
	region.w = (uint32_t)width;
	region.h = (uint32_t)height;
	region.d = 1;

	SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)(width * height * sizeof(uint8_t) * 4),
		.props = 0,
	};
	SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(app->device, &tbuf_info);
	void* memory = SDL_MapGPUTransferBuffer(app->device, tbuf, false);
	CF_MEMCPY(memory, pixels, sz);
	SDL_UnmapGPUTransferBuffer(app->device, tbuf);
	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(app->device);
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTextureTransferInfo src;
	src.rows_per_layer = height;
	src.pixels_per_row = width;
	src.offset = 0;
	src.transfer_buffer = tbuf;
	SDL_UploadToGPUTexture(pass, &src, &region, false);
	SDL_EndGPUCopyPass(pass);
	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(app->device, tbuf);

	auto& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.Fonts->SetTexID((ImTextureID)font_tex);
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
}

void cf_imgui_draw(SDL_GPUTexture* swapchain_texture)
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

#include <imgui/backends/imgui_impl_sdl3.cpp>
