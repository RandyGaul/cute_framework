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
		SDL_GpuReleaseBuffer(app->device, app->imgui_vbuf);
	}
	if (app->imgui_vtbuf) {
		SDL_GpuReleaseTransferBuffer(app->device, app->imgui_vtbuf);
	}
	if (app->imgui_ibuf) {
		SDL_GpuReleaseBuffer(app->device, app->imgui_ibuf);
	}
	if (app->imgui_itbuf) {
		SDL_GpuReleaseTransferBuffer(app->device, app->imgui_itbuf);
	}

	{
		SDL_GpuBufferCreateInfo buf_info = {
			.usageFlags = SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
			.sizeInBytes = (Uint32)(sizeof(ImDrawVert) * vertex_count),
			.props = 0
		};
		app->imgui_vbuf = SDL_GpuCreateBuffer(app->device, &buf_info);
		CF_ASSERT(app->imgui_vbuf);
	}

	{
		SDL_GpuTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = (Uint32)(sizeof(ImDrawVert) * vertex_count),
			.props = 0,
		};
		app->imgui_vtbuf = SDL_GpuCreateTransferBuffer(app->device, &tbuf_info);
		CF_ASSERT(app->imgui_vtbuf);
	}

	{
		SDL_GpuBufferCreateInfo buf_info = {
			.usageFlags = SDL_GPU_BUFFERUSAGE_INDEX_BIT,
			.sizeInBytes = (Uint32)(sizeof(ImDrawIdx) * index_count),
			.props = 0
		};
		app->imgui_ibuf = SDL_GpuCreateBuffer(app->device, &buf_info);
		CF_ASSERT(app->imgui_ibuf);
	}

	{
		SDL_GpuTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.sizeInBytes = (Uint32)(sizeof(ImDrawIdx) * index_count),
			.props = 0,
		};
		app->imgui_itbuf = SDL_GpuCreateTransferBuffer(app->device, &tbuf_info);
		CF_ASSERT(app->imgui_itbuf);
	}
}

void cf_imgui_init()
{
	CF_Shader shader = cf_make_shader_from_source(s_imgui_vs, s_imgui_fs);
	SDL_GpuShader* vs = ((CF_ShaderInternal*)shader.id)->vs;
	SDL_GpuShader* fs = ((CF_ShaderInternal*)shader.id)->fs;

	SDL_GpuColorAttachmentDescription color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	color_info.format = SDL_GpuGetSwapchainTextureFormat(app->device, app->window);
	color_info.blendState.blendEnable = true;
	color_info.blendState.alphaBlendOp = SDL_GPU_BLENDOP_ADD;
	color_info.blendState.colorBlendOp = SDL_GPU_BLENDOP_ADD;
	color_info.blendState.srcColorBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	color_info.blendState.srcAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
	color_info.blendState.dstColorBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_info.blendState.dstAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_info.blendState.colorWriteMask = 0xF;

	SDL_GpuGraphicsPipelineCreateInfo pip_info;
	CF_MEMSET(&pip_info, 0, sizeof(pip_info));
	pip_info.vertexShader = vs;
	pip_info.fragmentShader = fs;
	pip_info.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pip_info.multisampleState.sampleMask = 0xFFFF;
	pip_info.attachmentInfo.colorAttachmentCount = 1;
	pip_info.attachmentInfo.colorAttachmentDescriptions = &color_info;
	pip_info.attachmentInfo.hasDepthStencilAttachment = false; // @TODO.
	pip_info.attachmentInfo.depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
	pip_info.depthStencilState.depthTestEnable = false;
	pip_info.depthStencilState.depthWriteEnable = false;
	pip_info.depthStencilState.compareOp = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;

	SDL_GpuVertexBinding vertex_binding;
	CF_MEMSET(&vertex_binding, 0, sizeof(vertex_binding));
	vertex_binding.binding = 0;
	vertex_binding.stride = sizeof(ImDrawVert);
	vertex_binding.inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_binding.instanceStepRate = 0;

	SDL_GpuVertexAttribute vertex_attributes[] = {
		{
			.location = 0,
			.binding = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = 0
		},
		{
			.location = 1,
			.binding = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = sizeof(float) * 2,
		},
		{
			.location = 2,
			.binding = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
			.offset = sizeof(float) * 4,
		},
	};

	pip_info.vertexInputState = SDL_GpuVertexInputState {
		.vertexBindings = &vertex_binding,
		.vertexBindingCount = 1,
		.vertexAttributes = vertex_attributes,
		.vertexAttributeCount = 3,
	};

	pip_info.rasterizerState = SDL_GpuRasterizerState {
		.fillMode = SDL_GPU_FILLMODE_FILL,
		.cullMode = SDL_GPU_CULLMODE_NONE,
		.frontFace = {},
		.depthBiasEnable = SDL_FALSE,
		.depthBiasConstantFactor = {},
		.depthBiasClamp = {},
		.depthBiasSlopeFactor = {},
	};

	app->imgui_pip = SDL_GpuCreateGraphicsPipeline(app->device, &pip_info);
	CF_ASSERT(app->imgui_pip);
	SDL_GpuReleaseShader(app->device, vs);
	SDL_GpuReleaseShader(app->device, fs);

	int vertex_count = 1024 * 64;
	int index_count = 1024 * 64;
	s_make_buffers(vertex_count, index_count);

	SDL_GpuSamplerCreateInfo sampler_info = {
		.minFilter = SDL_GPU_FILTER_NEAREST,
		.magFilter = SDL_GPU_FILTER_NEAREST,
		.mipmapMode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.addressModeU = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.addressModeV = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.addressModeW = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};

	app->imgui_sampler = SDL_GpuCreateSampler(app->device, &sampler_info);
	CF_ASSERT(app->imgui_sampler);

	unsigned char* pixels;
	int width, height;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	int sz = (int)(width * height * sizeof(uint8_t) * 4);

	SDL_GpuTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER_BIT,
		.width = (uint32_t)width,
		.height = (uint32_t)height,
		.layerCountOrDepth = 1,
		.levelCount = 1,
		.sampleCount = {},
	};

	SDL_GpuTexture* font_tex = SDL_GpuCreateTexture(app->device, &texture_info);
	CF_ASSERT(font_tex);
	app->imgui_font_tex = font_tex;

	SDL_GpuTextureRegion region;
	CF_MEMSET(&region, 0, sizeof(region));
	region.texture = font_tex;
	region.w = (uint32_t)width;
	region.h = (uint32_t)height;
	region.d = 1;

	SDL_GpuTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.sizeInBytes = (Uint32)(width * height * sizeof(uint8_t) * 4),
		.props = 0,
	};
	SDL_GpuTransferBuffer* tbuf = SDL_GpuCreateTransferBuffer(app->device, &tbuf_info);
	void* memory = SDL_GpuMapTransferBuffer(app->device, tbuf, false);
	CF_MEMCPY(memory, pixels, sz);
	SDL_GpuUnmapTransferBuffer(app->device, tbuf);
	SDL_GpuCommandBuffer* cmd = SDL_GpuAcquireCommandBuffer(app->device);
	SDL_GpuCopyPass* pass = SDL_GpuBeginCopyPass(cmd);
	SDL_GpuTextureTransferInfo src;
	src.imageHeight = height;
	src.imagePitch = width;
	src.offset = 0;
	src.transferBuffer = tbuf;
	SDL_GpuUploadToTexture(pass, &src, &region, false);
	SDL_GpuEndCopyPass(pass);
	SDL_GpuSubmit(cmd);
	SDL_GpuReleaseTransferBuffer(app->device, tbuf);

	auto& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.Fonts->SetTexID((ImTextureID)font_tex);
}

void cf_imgui_shutdown()
{
	SDL_GpuReleaseSampler(app->device, app->imgui_sampler);
	SDL_GpuReleaseBuffer(app->device, app->imgui_vbuf);
	SDL_GpuReleaseBuffer(app->device, app->imgui_ibuf);
	SDL_GpuReleaseTransferBuffer(app->device, app->imgui_vtbuf);
	SDL_GpuReleaseTransferBuffer(app->device, app->imgui_itbuf);
	SDL_GpuReleaseGraphicsPipeline(app->device, app->imgui_pip);
	SDL_GpuReleaseTexture(app->device, app->imgui_font_tex);
	ImGui_ImplSDL3_Shutdown();
}

void cf_imgui_draw(SDL_GpuTexture* swapchain_texture)
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
	uint8_t* vertices = (uint8_t*)SDL_GpuMapTransferBuffer(app->device, app->imgui_vtbuf, true);
	uint8_t* indices = (uint8_t*)SDL_GpuMapTransferBuffer(app->device, app->imgui_itbuf, true);
		for (int n = 0; n < draw_data->CmdListsCount; n++) {
			ImDrawList* cmdList = draw_data->CmdLists[n];

			int vertex_sz = (int)(cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			CF_MEMCPY(vertices, cmdList->VtxBuffer.Data, vertex_sz);
			vertices += vertex_sz;

			int index_sz = (int)(cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			CF_MEMCPY(indices, cmdList->IdxBuffer.Data, index_sz);
			indices += index_sz;
		}
	SDL_GpuUnmapTransferBuffer(app->device, app->imgui_vtbuf);
	SDL_GpuUnmapTransferBuffer(app->device, app->imgui_itbuf);

	// Submit commands to upload buffers to the GPU.
	SDL_GpuCopyPass* copy_pass = SDL_GpuBeginCopyPass(app->cmd);
		SDL_GpuTransferBufferLocation src;
		src.offset = 0;
		src.transferBuffer = app->imgui_vtbuf;
		SDL_GpuBufferRegion region;
		region.buffer = app->imgui_vbuf;
		region.offset = 0;
		region.size = (uint32_t)(draw_data->TotalVtxCount * sizeof(ImDrawVert));
		SDL_GpuUploadToBuffer(copy_pass, &src, &region, true);

		src.transferBuffer = app->imgui_itbuf;
		region.buffer = app->imgui_ibuf;
		region.offset = 0;
		region.size = (uint32_t)(draw_data->TotalIdxCount * sizeof(ImDrawIdx));
		SDL_GpuUploadToBuffer(copy_pass, &src, &region, false);
	SDL_GpuEndCopyPass(copy_pass);

	// Setup rendering commands.
	SDL_GpuColorAttachmentInfo color_info;
	CF_MEMSET(&color_info, 0, sizeof(color_info));
	color_info.texture = swapchain_texture;
	color_info.loadOp = SDL_GPU_LOADOP_LOAD;
	color_info.storeOp = SDL_GPU_STOREOP_STORE;

	SDL_GpuDepthStencilAttachmentInfo depth_stencil_info;
	CF_MEMSET(&depth_stencil_info, 0, sizeof(depth_stencil_info));
	//depth_stencil_info.texture = app->depth_buffer;
	depth_stencil_info.cycle = true;
	depth_stencil_info.depthStencilClearValue.depth = 0;
	depth_stencil_info.depthStencilClearValue.stencil = 0;
	depth_stencil_info.loadOp = SDL_GPU_LOADOP_CLEAR;
	depth_stencil_info.storeOp = SDL_GPU_STOREOP_DONT_CARE;
	depth_stencil_info.stencilLoadOp = SDL_GPU_LOADOP_CLEAR;
	depth_stencil_info.stencilStoreOp = SDL_GPU_STOREOP_DONT_CARE;

	// Submit the actual rendering commands.
	SDL_GpuRenderPass* pass = SDL_GpuBeginRenderPass(app->cmd, &color_info, 1, NULL);
	SDL_GpuBindGraphicsPipeline(pass, app->imgui_pip);

	SDL_GpuViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = draw_data->DisplaySize.x,
		.h = draw_data->DisplaySize.y,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	SDL_GpuSetViewport(pass, &viewport);

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
	SDL_GpuPushVertexUniformData(app->cmd, 0, mvp, sizeof(mvp));

	SDL_GpuBufferBinding vertexBufferBinding = {
		.buffer = app->imgui_vbuf,
		.offset = 0
	};
	SDL_GpuBindVertexBuffers(pass, 0, &vertexBufferBinding, 1);
	SDL_GpuBufferBinding indexBufferBinding = {
		.buffer = app->imgui_ibuf,
		.offset = 0
	};
	SDL_GpuBindIndexBuffer(pass, &indexBufferBinding, sizeof(ImDrawIdx) == sizeof(uint16_t) ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);

	uint32_t idx_offset = 0;
	uint32_t vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr) {
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
					SDL_GpuBindGraphicsPipeline(pass, app->imgui_pip);
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
				SDL_GpuSetScissor(pass, &scissorDesc);

				SDL_GpuTextureSamplerBinding samplerBinding = {
					.texture = (SDL_GpuTexture*)pcmd->GetTexID(),
					.sampler = app->imgui_sampler
				};

				SDL_GpuBindFragmentSamplers(pass, 0, &samplerBinding, 1);
				SDL_GpuDrawIndexedPrimitives(pass, vtx_offset + pcmd->VtxOffset, idx_offset + pcmd->IdxOffset, pcmd->ElemCount, 1, 0);
			}
		}

		idx_offset += cmd_list->IdxBuffer.Size;
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	SDL_GpuEndRenderPass(pass);
}

#include <imgui/backends/imgui_impl_sdl3.cpp>
