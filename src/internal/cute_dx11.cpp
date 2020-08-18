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

#include <cute_c_runtime.h>

#include <internal/cute_dx11.h>

#define CINTERFACE
#define COBJMACROS
#define D3D11_NO_HELPERS
#include <d3d11.h>
#include <dxgi.h>

#include <windowsx.h>

#define SAFE_RELEASE(class, obj) if (obj) { class##_Release(obj); obj=0; }

namespace cute
{

static struct {
	HWND hwnd;
	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	int w;
	int h;
	int sample_count;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	IDXGISwapChain* swap_chain;
	ID3D11Texture2D* render_target;
	ID3D11RenderTargetView* render_target_view;
	ID3D11Texture2D* depth_stencil_buffer;
	ID3D11DepthStencilView* depth_stencil_view;
} state;

void d3d11_create_default_render_target() {
	HRESULT hr;
	hr = IDXGISwapChain_GetBuffer(state.swap_chain, 0, IID_ID3D11Texture2D, (void**)&state.render_target);
	CUTE_ASSERT(SUCCEEDED(hr) && state.render_target);
	hr = ID3D11Device_CreateRenderTargetView(state.device, (ID3D11Resource*)state.render_target, NULL, &state.render_target_view);
	CUTE_ASSERT(SUCCEEDED(hr) && state.render_target_view);

	D3D11_TEXTURE2D_DESC ds_desc = { 0 };
	ds_desc.Width = state.w;
	ds_desc.Height = state.h;
	ds_desc.MipLevels = 1;
	ds_desc.ArraySize = 1;
	ds_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ds_desc.SampleDesc = state.swap_chain_desc.SampleDesc;
	ds_desc.Usage = D3D11_USAGE_DEFAULT;
	ds_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = ID3D11Device_CreateTexture2D(state.device, &ds_desc, NULL, &state.depth_stencil_buffer);
	CUTE_ASSERT(SUCCEEDED(hr) && state.depth_stencil_buffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	SecureZeroMemory(&dsv_desc, sizeof(dsv_desc));
	dsv_desc.Format = ds_desc.Format;
	dsv_desc.ViewDimension = state.sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = ID3D11Device_CreateDepthStencilView(state.device, (ID3D11Resource*)state.depth_stencil_buffer, &dsv_desc, &state.depth_stencil_view);
	CUTE_ASSERT(SUCCEEDED(hr) && state.depth_stencil_view);
}

void d3d11_destroy_default_render_target(void) {
	SAFE_RELEASE(ID3D11Texture2D, state.render_target);
	SAFE_RELEASE(ID3D11RenderTargetView, state.render_target_view);
	SAFE_RELEASE(ID3D11Texture2D, state.depth_stencil_buffer);
	SAFE_RELEASE(ID3D11DepthStencilView, state.depth_stencil_view);
}

void d3d11_update_default_render_target(void) {
	if (state.swap_chain) {
		d3d11_destroy_default_render_target();
		IDXGISwapChain_ResizeBuffers(state.swap_chain, 2, state.w, state.h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
		d3d11_create_default_render_target();
	}
}

void dx11_init(void* hwnd, int w, int h, int sample_count)
{
	state.hwnd = (HWND)hwnd;
	state.w = w;
	state.h = h;
	state.sample_count = 1;

	/* create device and swap chain */
	state.swap_chain_desc.BufferDesc.Width = w;
	state.swap_chain_desc.BufferDesc.Height = h;
	state.swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	state.swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	state.swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	state.swap_chain_desc.OutputWindow = state.hwnd;
	state.swap_chain_desc.Windowed = true;
	state.swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	state.swap_chain_desc.BufferCount = 2;
	state.swap_chain_desc.SampleDesc.Count = state.sample_count;
	state.swap_chain_desc.SampleDesc.Quality = state.sample_count > 1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	state.swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	int create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	#ifdef _DEBUG
		create_flags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	D3D_FEATURE_LEVEL feature_level;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL,                       /* pAdapter (use default) */
		D3D_DRIVER_TYPE_HARDWARE,   /* DriverType */
		NULL,                       /* Software */
		create_flags,               /* Flags */
		NULL,                       /* pFeatureLevels */
		0,                          /* FeatureLevels */
		D3D11_SDK_VERSION,          /* SDKVersion */
		&state.swap_chain_desc,     /* pSwapChainDesc */
		&state.swap_chain,          /* ppSwapChain */
		&state.device,              /* ppDevice */
		&feature_level,             /* pFeatureLevel */
		&state.device_context);     /* ppImmediateContext */
	(void)hr;
	CUTE_ASSERT(SUCCEEDED(hr) && state.swap_chain && state.device && state.device_context);

	/* default render target and depth-stencil-buffer */
	d3d11_create_default_render_target();
}

static const void* s_d3d11_device(void) { 
	return (const void*)state.device;
}

static const void* s_d3d11_device_context(void) {
	return (const void*)state.device_context;
}

static const void* s_d3d11_render_target_view(void) {
	return (const void*)state.render_target_view;
}

static const void* s_d3d11_depth_stencil_view(void) {
	return (const void*)state.depth_stencil_view;
}

sg_context_desc dx11_get_context()
{
	sg_context_desc desc;
	desc.color_format = SG_PIXELFORMAT_RGBA8;
	desc.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
	desc.sample_count = state.sample_count;
	desc.d3d11.device = s_d3d11_device();
	desc.d3d11.device_context = s_d3d11_device_context();
	desc.d3d11.render_target_view_cb = s_d3d11_render_target_view;
	desc.d3d11.depth_stencil_view_cb = s_d3d11_depth_stencil_view;
	return desc;
}

void dx11_present()
{
	IDXGISwapChain_Present(state.swap_chain, 1, 0);

	/* handle window resizing */
	RECT r;
	if (GetClientRect(state.hwnd, &r)) {
		const int cur_width = r.right - r.left;
		const int cur_height = r.bottom - r.top;
		if (((cur_width > 0) && (cur_width != state.w)) ||
			((cur_height > 0) && (cur_height != state.h))) 
		{
			/* need to reallocate the default render target */
			state.w = cur_width;
			state.h = cur_height;
			d3d11_update_default_render_target();
		}
	}
}

void dx11_shutdown()
{
	d3d11_destroy_default_render_target();
	SAFE_RELEASE(IDXGISwapChain, state.swap_chain);
	SAFE_RELEASE(ID3D11DeviceContext, state.device_context);
	SAFE_RELEASE(ID3D11Device, state.device);
}

}
