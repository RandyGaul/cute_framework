#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define GFX_USE_DIRECTX_INCLUDES_AND_IMPORTS
#define GFX_USE_D3D_ERR_LIB

#define GFX_ASSERT assert

#define GFX_ERROR_SUCCESS (0)
#define GFX_ERROR_FAILURE (-1)

// HACK: Was originally gfx_error_raise. gfx.h needs a proper error handler thingy.
#ifndef gfx_error_raise
	#define gfx_error_raise(x) GFX_ERROR_FAILURE
#endif // gfx_error_raise
#define GFX_UNUSED(x) (void)x

// the point of high level primitives is to avoid manually creating draw calls
// the context can be used to store global render state
// otherwise lower level primitives can be used for custom rendering or fx

#define GFX_VERTEX_BUFFER_MAX_ATTRIBUTES (16)
#define GFX_DRAW_CALL_MAX_TEXTURES (8)
#define GFX_DRAW_CALL_MAX_UNIFORMS (16)
#define GFX_OFFEST_OF(T, member) ((int)((uintptr_t)(&(((T*)0)->member))))

struct gfx_t;

enum gfx_vertex_buffer_type_t
{
	GFX_VERTEX_BUFFER_TYPE_STATIC,
	GFX_VERTEX_BUFFER_TYPE_DYNAMIC
};

enum gfx_index_buffer_type_t
{
	GFX_INDEX_BUFFER_TYPE_NONE,
	GFX_INDEX_BUFFER_TYPE_UINT16,
	GFX_INDEX_BUFFER_TYPE_UINT32,
};

struct gfx_vertex_buffer_t;

struct gfx_vertex_buffer_params_t
{
	gfx_vertex_buffer_type_t type = GFX_VERTEX_BUFFER_TYPE_DYNAMIC;
	int attribute_count = 0;
	int num_components[GFX_VERTEX_BUFFER_MAX_ATTRIBUTES];
	int offsets[GFX_VERTEX_BUFFER_MAX_ATTRIBUTES];
	int stride = 0;

	int vertex_count = 0;
	gfx_index_buffer_type_t index_type = GFX_INDEX_BUFFER_TYPE_NONE;
	int index_count = 0;
};

inline void gfx_vertex_buffer_params_add_attribute(gfx_vertex_buffer_params_t* params, int num_components, int offset)
{
	int i = params->attribute_count++;
	params->num_components[i] = num_components;
	params->offsets[i] = offset;
}

gfx_vertex_buffer_t* gfx_vertex_buffer_new(gfx_t* gfx, gfx_vertex_buffer_params_t* params);
void gfx_vertex_buffer_free(gfx_t* gfx, gfx_vertex_buffer_t* buffer);

int gfx_vertex_buffer_map(gfx_t* gfx, gfx_vertex_buffer_t* buffer, int vertex_count, void** vertices, int index_count = 0, void** indices = NULL);
int gfx_vertex_buffer_unmap(gfx_t* gfx, gfx_vertex_buffer_t* buffer);

struct gfx_matrix_t
{
	float data[16];
};

void matrix_identity(gfx_matrix_t* m);

void projection_ortho_2d(gfx_matrix_t* projection, float w, float h, float x, float y);

inline gfx_matrix_t projection_identity()
{
	gfx_matrix_t projection;
	memset(&projection, 0, sizeof(projection));
	matrix_identity(&projection);
	return projection;
}

enum gfx_pixel_format_t
{
	GFX_PIXEL_FORMAT_R8B8G8A8
};

enum gfx_wrap_mode_t
{
	GFX_WRAP_MODE_REPEAT,
	GFX_WRAP_MODE_CLAMP_EDGE,
	GFX_WRAP_MODE_CLAMP_BORDER,
	GFX_WRAP_MODE_REPEAT_MIRRORED,
};

struct gfx_texture_t;

struct gfx_texture_params_t
{
	int w = 0, h = 0;
	gfx_pixel_format_t pixel_format = GFX_PIXEL_FORMAT_R8B8G8A8;
	gfx_wrap_mode_t wrap_mode = GFX_WRAP_MODE_REPEAT;
	void* pixels = 0;
};

gfx_texture_t *texture_create(gfx_t* gfx, gfx_texture_params_t* params);
void texture_clean_up(gfx_t* gfx, gfx_texture_t* tex);

struct gfx_render_texture_t;

gfx_render_texture_t* render_texture_new(gfx_t* gfx, int w, int h, gfx_pixel_format_t pixel_format = GFX_PIXEL_FORMAT_R8B8G8A8, gfx_wrap_mode_t wrap_mode = GFX_WRAP_MODE_REPEAT);
void render_texture_clean_up(gfx_t* gfx, gfx_render_texture_t* render_texture);

enum gfx_uniform_type_t
{
	GFX_UNIFORM_TYPE_INVALID,
	GFX_UNIFORM_TYPE_BOOL,
	GFX_UNIFORM_TYPE_FLOAT,
	GFX_UNIFORM_TYPE_FLOAT2,
	GFX_UNIFORM_TYPE_FLOAT3,
	GFX_UNIFORM_TYPE_FLOAT4,
	GFX_UNIFORM_TYPE_INT,
	GFX_UNIFORM_TYPE_TEXTURE,
	GFX_UNIFORM_TYPE_MATRIX,
};

struct gfx_shader_t;

struct gfx_shader_params_t
{
	gfx_vertex_buffer_t* buffer;
	const char* vertex_shader;
	const char* pixel_shader;
};

gfx_shader_t* gfx_shader_new(gfx_t* gfx, gfx_shader_params_t* params);
void gfx_shader_free(gfx_t* gfx, gfx_shader_t* shader);
int gfx_shader_set_uniform(gfx_t* gfx, gfx_shader_t* shader, const char* uniform_name, void* value, gfx_uniform_type_t type);
int gfx_shader_set_mvp(gfx_t* gfx, gfx_shader_t* shader, gfx_matrix_t* mvp);
int gfx_shader_set_screen_wh(gfx_t* gfx, gfx_shader_t* shader, float w, float h);

struct gfx_viewport_t
{
	float x, y;
	float w, h;
};

struct gfx_scissor_t
{
	int left;
	int top;
	int right;
	int bottom;
};

struct gfx_uniform_t
{
	int dirty;
	const char* name;
	uint64_t h;
	gfx_uniform_type_t type;
	union
	{
		float data[16];
		void* texture_handle;
	} u;
};

struct gfx_buffer_indices_t
{
	int index0 = 0;
	int index1 = 0;
	int element_count = 0;
	int stride = 0;
	int buffer_number = 0;
};

struct gfx_draw_call_t
{
	int use_mvp = 0;
	gfx_matrix_t mvp;
	gfx_shader_t* shader = NULL;
	gfx_viewport_t* veiwport = NULL;
	int use_scissor = 0;
	gfx_scissor_t scissor;
	gfx_vertex_buffer_t* buffer = NULL;
	gfx_buffer_indices_t vertex_indices;
	gfx_buffer_indices_t index_indices;
	int texture_count = 0;
	gfx_texture_t* textures[GFX_DRAW_CALL_MAX_TEXTURES];
	const char* texture_uniform_names[GFX_DRAW_CALL_MAX_TEXTURES];
	int uniform_count = 0;
	gfx_uniform_t uniforms[GFX_DRAW_CALL_MAX_UNIFORMS];
};

// TODO:
// Draw call and uniform can *not* store strings if rendering is moved off the main thread,
// otherwise the pointers will dangle. Copy strings into local buffers or something!

void gfx_draw_call_set_mvp(gfx_draw_call_t* call, gfx_matrix_t* mvp);
void gfx_draw_call_set_scissor_box(gfx_draw_call_t* call, gfx_scissor_t* scissor);
void gfx_draw_call_add_texture(gfx_draw_call_t* call, gfx_texture_t* texture, const char* uniform_name);
void gfx_draw_call_add_verts(gfx_t* gfx, gfx_draw_call_t* call, void* verts, int vert_count);
void gfx_draw_call_add_uniform(gfx_draw_call_t* call, const char* uniform_name, void* value, gfx_uniform_type_t type);

enum gfx_type_t
{
	GFX_TYPE_D3D9,
	GFX_TYPE_GL,
	GFX_TYPE_GL_ES
};

enum gfx_upscale_maximum_t
{
	GFX_UPSCALE_MAXIMUM_ANY,
	GFX_UPSCALE_MAXIMUM_4X,
	GFX_UPSCALE_MAXIMUM_3X,
	GFX_UPSCALE_MAXIMUM_2X,
	GFX_UPSCALE_MAXIMUM_1X,
};

struct gfx_t;

gfx_t* gfx_new(gfx_type_t type, gfx_pixel_format_t pixel_format, int screen_w, int screen_h, int tex_w, int tex_h, gfx_upscale_maximum_t upscale, void* platform_handle = NULL);
void gfx_clean_up(gfx_t* gfx);
void gfx_push_draw_call(gfx_t* gfx, gfx_draw_call_t* call);
void gfx_flush(gfx_t* gfx);
void gfx_flush_to_texture(gfx_t* gfx, gfx_texture_t* render_texture);

void gfx_set_alpha(gfx_t* gfx, int one_for_enabled);
gfx_type_t gfx_type(gfx_t* gfx);

void gfx_set_clear_color(gfx_t* gfx, int color);

// texture wrap modes

// ortho 2d
// perspective
// mul
// identity
// copy

void gfx_line_mvp(gfx_t* gfx, gfx_matrix_t* projection);
void gfx_line_color(gfx_t* gfx, float r, float g, float b);
void gfx_line(gfx_t* gfx, float ax, float ay, float bx, float by);
void gfx_line_width(gfx_t* gfx, float width);
void gfx_line_depth_test(gfx_t* gfx, int zero_for_off);
void gfx_line_submit_draw_call(gfx_t* gfx);

// make/clean up framebuffer

#endif // GFX_H

#ifdef GFX_IMPLEMENTATION
#ifndef GFX_IMPLEMENTATION_ONCE
#define GFX_IMPLEMENTATION_ONCE

// HACKS HERE
#include <assert.h>
#define GFX_ALLOC malloc
#define GFX_FREE free
#define GFX_MEMCPY memcpy

#define GFX_DLIST_INIT(sentinel) \
	do { \
		(sentinel)->next = (sentinel); \
		(sentinel)->prev = (sentinel); \
	} while (0)

#define GFX_DLIST_INSERT(sentinel, element) \
	do { \
		(element)->prev = (sentinel); \
		(element)->next = (sentinel)->next; \
		(sentinel)->next->prev = (element); \
		(sentinel)->next = (element); \
	} while (0)

#define GFX_DLIST_REMOVE(element) \
	do { \
		(element)->prev->next = (element)->next; \
		(element)->next->prev = (element)->prev; \
	} while (0)

#define GFX_DLIST_HEAD_INSERT(head, element) \
	do { \
		if (head) { \
			GFX_DLIST_INSERT(head, element); \
		} else { \
			head = element; \
		} \
	} while (0)

#define GFX_DLIST_HEAD_REMOVE(head, element) \
	do { \
		if (head == element) { \
			head = element->next; \
			if (head == element) { \
				head = NULL; \
			} \
		} \
		GFX_DLIST_REMOVE(element); \
	} while (0)

#define GFX_CHECK_BUFFER_GROW(ctx, count, capacity, data, type, initial) \
	do { \
		if (ctx->count == ctx->capacity) \
		{ \
			int new_capacity = ctx->capacity ? ctx->capacity * 2 : initial; \
			void* new_data = GFX_ALLOC(sizeof(type) * new_capacity); \
			GFX_MEMCPY(new_data, ctx->data, sizeof(type) * ctx->count); \
			GFX_FREE(ctx->data); \
			ctx->data = (type*)new_data; \
			ctx->capacity = new_capacity; \
		} \
	} while (0)

enum gfx_dummy_enum_t { GFX_EDUMMY };
inline void* operator new(size_t, gfx_dummy_enum_t, void* ptr) { return ptr; }
#define GFX_PLACEMENT_NEW(ptr) new(GFX_EDUMMY, ptr)
#define GFX_NEW(T) new(GFX_EDUMMY, GFX_ALLOC(sizeof(T))) T

struct gfx_vertex_t
{
	float x, y;
	float u, v;
};

#define GFX_STR(X) #X
// HACKS END

struct gfx_line_vertex_t
{
	float x, y;
	float r, g, b;
};

struct gfx_shader_data_t
{
	void* handle = NULL;
	void* impl = NULL;
	int uniform_count = 0;
	gfx_uniform_t *uniforms = NULL;
};

struct gfx_shader_t
{
	gfx_vertex_buffer_t* buffer = NULL;
	gfx_shader_data_t vertex_shader;
	gfx_shader_data_t pixel_shader;
};

struct gfx_vertex_buffer_t
{
	int vertex_count = 0;
	int index_count = 0;
	gfx_buffer_indices_t vertex_indices;
	gfx_buffer_indices_t index_indices;
	gfx_vertex_buffer_params_t desc;
	gfx_index_buffer_type_t index_type;
	int buffer_count = 0;
	void* vertex_buffers[3];
	void* index_buffers[3];
	void* desc_handle;
	gfx_vertex_buffer_t* next;
	gfx_vertex_buffer_t* prev;
};

struct gfx_t
{
	gfx_type_t type;
	int alpha_one_for_enabled = 1;
	gfx_pixel_format_t screen_pixel_format;
	int screen_w, screen_h;
	int clear_color;
	gfx_upscale_maximum_t upscale_max_setting;
	gfx_viewport_t viewport;
	gfx_matrix_t default_projection;
	int draw_call_count = 0;
	int draw_call_capacity = 0;
	gfx_draw_call_t* draw_calls = 0;
	gfx_render_texture_t* render_texture = 0;
	gfx_vertex_buffer_t* static_render_texture_quad = 0;
	gfx_shader_t* upscale_shader = 0;
	gfx_shader_t* line_shader = 0;
	gfx_vertex_buffer_t* line_buffer = 0;
	float r, g, b;
	int line_vert_count = 0;
	int line_vert_capacity = 0;
	float* line_verts = 0;
	int line_depth_test = 0;
	float line_width = 1.0f;
	void* platform_handle = 0;
	void* impl = 0;

	gfx_texture_t* checker;
};

void s_shader_copy_uniform_data_from_draw_call(gfx_t* gfx, gfx_draw_call_t* call);
int s_is_lost(gfx_t* gfx);

inline int calc_index_size(gfx_index_buffer_type_t type)
{
	switch (type)
	{
	case GFX_INDEX_BUFFER_TYPE_NONE: return 0;
	case GFX_INDEX_BUFFER_TYPE_UINT16: return 2;
	case GFX_INDEX_BUFFER_TYPE_UINT32: return 4;
	default: return 0;
	}
}

inline int advance_buffer_indices(gfx_vertex_buffer_type_t type, gfx_buffer_indices_t* indices, int count)
{
	if (count > indices->element_count) {
		gfx_error_raise("Attempted to map a vertex buffer segment larger than the vertex buffer itself.");
		return GFX_ERROR_FAILURE;
	}

	int new_index = indices->index1 + count;
	if (type == GFX_VERTEX_BUFFER_TYPE_DYNAMIC) {
		if (new_index > indices->element_count) {
			++indices->buffer_number;
			indices->buffer_number %= 3;
			indices->index0 = 0;
			indices->index1 = count;
		} else {
			indices->index0 = indices->index1;
			indices->index1 = new_index;
		}
	} else {
		indices->index1 = new_index;
	}

	return GFX_ERROR_SUCCESS;
}

#ifdef GFX_USE_DIRECTX_INCLUDES_AND_IMPORTS

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#include <d3dx9.h>

#ifdef GFX_USE_D3D_ERR_LIB
#	pragma comment(lib, "dxerr.lib")
#	pragma comment(lib, "legacy_stdio_definitions.lib")
#	include <DxErr.h>

	__declspec(thread) char d3d9_error_string_buffer[1024];
	const char* get_error_string_d3d9(HRESULT hr)
	{
		snprintf(d3d9_error_string_buffer, 1024, "%s, %s.", DXGetErrorString(hr), DXGetErrorDescription(hr));
		return d3d9_error_string_buffer;
	}

#	define HR_CHECK(X) do { HRESULT hr = (X); if (FAILED(hr)) { gfx_error_raise(get_error_string_d3d9(hr)); } } while (0)
#endif

int s_d3d9_compile_shader(gfx_t* gfx, gfx_shader_t* shader, const char* vs, const char* ps);

struct d3d9_render_texture_t
{
	int w, h;
	gfx_pixel_format_t pixel_format;
	gfx_wrap_mode_t wrap_mode;
	IDirect3DTexture9* texture;
	IDirect3DSurface9* surface;
	d3d9_render_texture_t* next;
	d3d9_render_texture_t* prev;
};

struct d3d9_context_t
{
	int lost;
	const char* vs_profile;
	const char* ps_profile;
	IDirect3DDevice9* dev;
	IDirect3D9* d3d9;
	gfx_vertex_buffer_t* buffers;
	d3d9_render_texture_t* render_textures;
	D3DPRESENT_PARAMETERS params;
	D3DCAPS9 caps;
	IDirect3DSurface9* screen_surface;
};

static void s_d3d9_setup_render_and_sampler_states(IDirect3DDevice9* dev)
{
	HR_CHECK(dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE));
	HR_CHECK(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	HR_CHECK(dev->SetRenderState(D3DRS_LIGHTING, FALSE));
	HR_CHECK(dev->SetRenderState(D3DRS_SCISSORTESTENABLE, D3DZB_TRUE));
	HR_CHECK(dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	HR_CHECK(dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	HR_CHECK(dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT));
	HR_CHECK(dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT));
}

static void* s_d3d9_create_impl(gfx_t* gfx, void* platform_handle)
{
	d3d9_context_t* impl = (d3d9_context_t*)GFX_ALLOC(sizeof(d3d9_context_t));
	impl->lost = 0;
	IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d9) {
		gfx_error_raise("Failed to initialize Direct3D 9 - the application was built against the correct header files.");
		return NULL;
	}

	D3DDISPLAYMODE mode;
	HRESULT res = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
	if (FAILED(res)) {
		gfx_error_raise("Direct3D 9 was unable to get adapter display mode.");
		return NULL;
	}

	res = d3d9->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format, D3DFMT_A8R8G8B8, true);
	if (FAILED(res)) {
		gfx_error_raise("HAL was detected as not supported by DirectD 9 for the D3DFMT_A8R8G8B8 adapter/backbuffer format.");
		return NULL;
	}

	res = d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &impl->caps);
	if (FAILED(res)) {
		gfx_error_raise("Failed to gather Direct3D 9 device caps.");
		return NULL;
	}

	int flags = D3DCREATE_FPU_PRESERVE;
	if (impl->caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
		assert(impl->caps.VertexProcessingCaps != 0);
	} else {
		flags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		assert(impl->caps.VertexProcessingCaps);
	}

	if (impl->caps.DevCaps & D3DDEVCAPS_PUREDEVICE) {
		flags |= D3DCREATE_PUREDEVICE;
	}

	// TODO: Think about if full screen mode should be supported.
	// TODO: Think about how to enable depth/stencil buffers in the API.

	memset(&impl->params, 0, sizeof(impl->params));
	impl->params.BackBufferWidth = gfx->screen_w;
	impl->params.BackBufferHeight = gfx->screen_h;
	impl->params.BackBufferFormat = D3DFMT_A8R8G8B8;
	//impl->params.EnableAutoDepthStencil = 1;
	//impl->params.AutoDepthStencilFormat = D3DFMT_D16;
	impl->params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	impl->params.hDeviceWindow = (HWND)platform_handle;
	impl->params.Windowed = true;
	impl->params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	IDirect3DDevice9* dev;
	res = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)platform_handle, flags, &impl->params, &dev);
	if (FAILED(res)) {
		gfx_error_raise("Failed to create Direct3D 9 device.");
		return NULL;
	}

	impl->dev = dev;
	const char* vs_profile = D3DXGetVertexShaderProfile(dev);
	const char* ps_profile = D3DXGetPixelShaderProfile(dev);
	impl->vs_profile = "vs_2_0";
	impl->ps_profile = "ps_2_0";

	if (vs_profile[3] < impl->vs_profile[3]) {
		gfx_error_raise("The user machine does not support vertex shader profile 2.0.");
		return NULL;
	}

	if (ps_profile[3] < impl->ps_profile[3]) {
		gfx_error_raise("The user machine does not support pixel shader profile 2.0.");
		return NULL;
	}

	res = impl->dev->GetRenderTarget(0, &impl->screen_surface);
	if (FAILED(res)) {
		gfx_error_raise("Unable to get render target.");
		return NULL;
	}

	impl->buffers = NULL;
	impl->render_textures = NULL;

	s_d3d9_setup_render_and_sampler_states(dev);

	return impl;
}

static int s_d3d9_vertex_buffer_map(gfx_vertex_buffer_t* buffer, int vertex_count, void** vertices, int index_count, void** indices)
{
	if (advance_buffer_indices(buffer->desc.type, &buffer->vertex_indices, vertex_count)) {
		return GFX_ERROR_FAILURE;
	}

	IDirect3DVertexBuffer9* vbuf = (IDirect3DVertexBuffer9*)buffer->vertex_buffers[buffer->vertex_indices.buffer_number];
	int offset = buffer->vertex_indices.index0 * buffer->desc.stride;
	int stream_size = buffer->vertex_indices.index1 * buffer->desc.stride - offset;
	HRESULT res = vbuf->Lock(offset, stream_size, vertices, D3DLOCK_NOOVERWRITE);
	if (FAILED(res)) {
		gfx_error_raise("Failed to lock vertex buffer.");
		return GFX_ERROR_FAILURE;
	}

	if (indices) {
		if (advance_buffer_indices(buffer->desc.type, &buffer->index_indices, index_count)) {
			return GFX_ERROR_FAILURE;
		}

		IDirect3DIndexBuffer9* ibuf = (IDirect3DIndexBuffer9*)buffer->index_buffers[buffer->index_indices.buffer_number];
		int stride = buffer->index_indices.stride;
		offset = buffer->index_indices.index0 * stride;
		stream_size = buffer->index_indices.index1 * stride - offset;
		ibuf->Lock(offset, stream_size, indices, D3DLOCK_NOOVERWRITE);

		if (FAILED(res)) {
			gfx_error_raise("Failed to lock index buffer.");
			return GFX_ERROR_FAILURE;
		}
	}

	return GFX_ERROR_SUCCESS;
}

static int s_d3d9_vertex_buffer_unmap(gfx_vertex_buffer_t* buffer)
{
	IDirect3DVertexBuffer9* vbuf = (IDirect3DVertexBuffer9*)buffer->vertex_buffers[buffer->vertex_indices.buffer_number];
	HRESULT res = vbuf->Unlock();
	if (FAILED(res)) {
		return gfx_error_raise("Failed to lock vertex buffer.");
	}

	if (buffer->index_type != GFX_INDEX_BUFFER_TYPE_NONE) {
		IDirect3DIndexBuffer9* ibuf = (IDirect3DIndexBuffer9*)buffer->index_buffers[buffer->index_indices.buffer_number];
		res = ibuf->Unlock();
		if (FAILED(res)) {
			return gfx_error_raise("Failed to lock vertex buffer.");
		}
	}

	return GFX_ERROR_SUCCESS;
}

static inline int s_num_components_to_D3DDECLTYPE(int num_components)
{
	switch (num_components)
	{
	case 1: return D3DDECLTYPE_FLOAT1;
	case 2: return D3DDECLTYPE_FLOAT2;
	case 3: return D3DDECLTYPE_FLOAT3;
	case 4: return D3DDECLTYPE_FLOAT4;
	default: return gfx_error_raise("The number of components for a vertex_buffer_desc_t must be > 0 and <= 4.");
	}
}

// TODO: Consider this stuff:
// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/d3dxgetshaderinputsemantics
// D3DXGetShaderInputSemantics
// Introspect vertex stream input usage and usage index (POSITION0, POSITION1, TEXCOORD0, etc...)

static int s_d3d9_vertex_buffer_init(gfx_vertex_buffer_t* buffer, d3d9_context_t* impl)
{
	D3DVERTEXELEMENT9 attribute_descriptors[GFX_VERTEX_BUFFER_MAX_ATTRIBUTES + 1];
	IDirect3DVertexDeclaration9* decl;

	if (GFX_VERTEX_BUFFER_MAX_ATTRIBUTES > MAXD3DDECLUSAGEINDEX + 1) {
		return gfx_error_raise("VERTEX_BUFFER_MAX_ATTRIBUTES is defined as an invalid number. Please modify and recompile the source code.");
	}

	if (buffer->desc.attribute_count > MAXD3DDECLUSAGEINDEX) {
		return gfx_error_raise("The max number of attributes for a vertex is VERTEX_BUFFER_MAX_ATTRIBUTES.");
	}

	for (int i = 0; i < buffer->desc.attribute_count; ++i)
	{
		D3DVERTEXELEMENT9 element;
		element.Stream = 0;
		element.Offset = (WORD)buffer->desc.offsets[i];
		element.Type = (BYTE)s_num_components_to_D3DDECLTYPE(buffer->desc.num_components[i]);
		element.Method = (BYTE)D3DDECLMETHOD_DEFAULT;
		element.Usage = i == 0 ? (BYTE)D3DDECLUSAGE_POSITION : (BYTE)D3DDECLUSAGE_TEXCOORD;
		element.UsageIndex = (BYTE)(i == 0 ? 0 : i -1);
		attribute_descriptors[i] = element;
	}

	attribute_descriptors[buffer->desc.attribute_count] = D3DDECL_END();

	// create the generic decl
	HRESULT res = impl->dev->CreateVertexDeclaration(attribute_descriptors, &decl);
	if (FAILED(res)) {
		return gfx_error_raise(get_error_string_d3d9(res));
	}
	buffer->desc_handle = decl;

	int buffer_count = buffer->desc.type == GFX_VERTEX_BUFFER_TYPE_STATIC ? 1 : 3;
	buffer->buffer_count = buffer_count;
	DWORD usage = buffer->desc.type == GFX_VERTEX_BUFFER_TYPE_STATIC ? 0 : D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	D3DPOOL pool = buffer->desc.type == GFX_VERTEX_BUFFER_TYPE_STATIC ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
	IDirect3DVertexBuffer9* vertex_buffer;
	int vertex_buffer_size = buffer->desc.stride * buffer->vertex_count;
	int index_stride = calc_index_size(buffer->index_type);
	int index_buffer_size = index_stride * buffer->index_count;

	for (int i = 0; i < buffer_count; ++i)
	{
		res = impl->dev->CreateVertexBuffer(vertex_buffer_size, usage, 0, pool, &vertex_buffer, NULL);
		if (FAILED(res)) {
			return gfx_error_raise("Failed to create vertex buffer.");
		} else {
			buffer->vertex_buffers[i] = vertex_buffer;
			buffer->vertex_indices.element_count = buffer->vertex_count;
			buffer->vertex_indices.stride = buffer->desc.stride;

			if (buffer->index_type != GFX_INDEX_BUFFER_TYPE_NONE) {
				IDirect3DIndexBuffer9* index_buffer;
				res = impl->dev->CreateIndexBuffer(index_buffer_size, usage, buffer->index_type == GFX_INDEX_BUFFER_TYPE_UINT16 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, pool, &index_buffer, NULL);
				if (FAILED(res)) {
					return gfx_error_raise("Failed to create index buffer.");
				} else {
					buffer->index_buffers[i] = index_buffer;
					buffer->index_indices.element_count = buffer->index_count;
					buffer->index_indices.stride = index_stride;
				}
			}
		}
	}

	return GFX_ERROR_SUCCESS;
}

static gfx_vertex_buffer_t* s_d3d9_vertex_buffer_init(gfx_t* gfx, gfx_vertex_buffer_params_t* params)
{
	gfx_vertex_buffer_t* buffer = (gfx_vertex_buffer_t*)GFX_ALLOC(sizeof(gfx_vertex_buffer_t));
	GFX_PLACEMENT_NEW(buffer) gfx_vertex_buffer_t;
	if (!buffer) return NULL;
	GFX_DLIST_INIT(buffer);
	buffer->desc = *params;
	buffer->vertex_count = params->vertex_count;
	buffer->index_count = params->index_count;
	buffer->index_type = params->index_type;
	buffer->vertex_indices.buffer_number = 0;
	buffer->index_indices.buffer_number = 0;
	if (s_d3d9_vertex_buffer_init(buffer, (d3d9_context_t*)gfx->impl)) {
		GFX_FREE(buffer);
		return NULL;
	} else {
		if (params->index_type != params->type != GFX_VERTEX_BUFFER_TYPE_STATIC) {
			d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
			GFX_DLIST_HEAD_INSERT(impl->buffers, buffer);
		}
		return buffer;
	}
}

static void* s_d3d9_texture_init(gfx_t* gfx, gfx_texture_params_t* params)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	IDirect3DTexture9* texture;
	int w = params->w;
	int h = params->h;
	HRESULT res = impl->dev->CreateTexture((UINT)w, (UINT)h, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
	if (FAILED(res)) {
		gfx_error_raise("Failed to create texture.");
		return NULL;
	}

	D3DLOCKED_RECT rect;
	HR_CHECK(texture->LockRect(0, &rect, NULL, D3DUSAGE_WRITEONLY));

	for (int i = 0; i < h; ++i)
	{
		uint32_t* row = (uint32_t*)((uint8_t*)rect.pBits + (h - i - 1) * rect.Pitch);
		for (int j = 0; j < w; ++j)
		{
			// RBGA to BGRA
			uint32_t pixel = ((uint32_t*)params->pixels)[i * w + j];
			pixel = ((pixel & 0x000000FF) << 16)
			      | ((pixel & 0x0000FF00))
			      | ((pixel & 0x00FF0000) >> 16)
			      | ((pixel & 0xFF000000));
			row[j] = pixel;
		}
	}

	HR_CHECK(texture->UnlockRect(0));

	return texture;
}

static void texture_clean_up_d3d9(gfx_texture_t* tex)
{
	IDirect3DTexture9* texture = (IDirect3DTexture9*)tex;
	texture->Release();
}

static inline gfx_uniform_type_t s_d3d9_type_to_uniform_type(D3DXCONSTANT_DESC* desc)
{
	int reg_count = desc->RegisterCount;

	switch (desc->Type)
	{
	default:
		goto invalid_type;

	case D3DXPT_BOOL: return GFX_UNIFORM_TYPE_BOOL;
	case D3DXPT_INT:  return GFX_UNIFORM_TYPE_INT;
	case D3DXPT_FLOAT:
		switch (desc->Rows * desc->Columns)
		{
		case 1:  return GFX_UNIFORM_TYPE_FLOAT;
		case 2:  return GFX_UNIFORM_TYPE_FLOAT2;
		case 3:  return GFX_UNIFORM_TYPE_FLOAT3;
		case 4:  return GFX_UNIFORM_TYPE_FLOAT4;
		case 16: return GFX_UNIFORM_TYPE_MATRIX;
		default: goto invalid_type;
		}
	case D3DXPT_SAMPLER:   return GFX_UNIFORM_TYPE_TEXTURE;
	case D3DXPT_SAMPLER1D: return GFX_UNIFORM_TYPE_TEXTURE;
	case D3DXPT_SAMPLER2D: return GFX_UNIFORM_TYPE_TEXTURE;
	case D3DXPT_SAMPLER3D: return GFX_UNIFORM_TYPE_TEXTURE;
	}

invalid_type:
	gfx_error_raise("Unknown uniform type encountered in HLSL shader.");
	return GFX_UNIFORM_TYPE_INVALID;
}

static uint64_t s_gfx_FNV1a(const char* str)
{
	uint64_t h = (uint64_t)14695981039346656037;
	char c;

	while ((c = *str++))
	{
		h = h ^ (uint64_t)c;
		h = h * (uint64_t)1099511628211;
	}

	return h;
}

static void s_d3d9_build_constant_table(d3d9_context_t* impl, gfx_shader_data_t* s, ID3DXConstantTable* table)
{
	D3DXCONSTANTTABLE_DESC table_desc;
	HRESULT res = table->GetDesc(&table_desc);
	if (FAILED(res)) {
		gfx_error_raise("Failed to get constant description from D3DX shader constant table.");
		return;
	}

	int num_constants = table_desc.Constants;
	s->uniform_count = num_constants;
	s->uniforms = (gfx_uniform_t*)GFX_ALLOC(sizeof(gfx_uniform_t) * num_constants);

	D3DXCONSTANT_DESC descriptors[16];
	for (int i = 0; i < num_constants; ++i)
	{
		D3DXHANDLE handle = table->GetConstant(NULL, i);
		UINT desc_count = 16;
		table->GetConstantDesc(handle, descriptors, &desc_count);

		if (desc_count != 1) {
			gfx_error_raise("Unable to handle samplers appearing in more than one constant table.");
			// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/id3dxconstanttable--getconstantdesc
		}

		D3DXCONSTANT_DESC* desc = descriptors + 0;
		gfx_uniform_t* u = s->uniforms + i;
		uint64_t h = s_gfx_FNV1a(desc->Name);
		u->dirty = 0;
		u->name = strdup(desc->Name); // TODO: Memory leak here.
		u->h = h;
		u->type = s_d3d9_type_to_uniform_type(desc);
		u->u.texture_handle = 0;
	}

	for (int i = 0; i < s->uniform_count; ++i)
	{
		gfx_uniform_t* a = s->uniforms + i;
		for (int j = i + 1; j < s->uniform_count; ++j)
		{
			gfx_uniform_t* b = s->uniforms + j;
			if (a->h == b->h) {
				gfx_error_raise("Duplicate shader uniform, or hash collision, was detected.");
			}
		}
	}
}

static gfx_shader_t* s_d3d9_compile_shader(gfx_t* gfx, gfx_shader_params_t* params)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;

	ID3DXBuffer* compiled_shader;
	ID3DXBuffer* error_msgs;
	ID3DXConstantTable* constant_table;

	const char* vs = params->vertex_shader;
	const char* ps = params->pixel_shader;

	gfx_shader_t* shader = GFX_NEW(gfx_shader_t);

	// TODO: Confirm if D3DXSHADER_PACKMATRIX_ROWMAJOR is needed or not for the flags param.
	// Very nice info by Hodgman: https://www.gamedev.net/forums/topic/682063-vector-and-matrix-multiplication-order-in-diregfx-and-opengl/

	// vertex shader
	HRESULT res = D3DXCompileShader(vs, strlen(vs), NULL, NULL, "main", impl->vs_profile, 0, &compiled_shader, &error_msgs, &constant_table);
	if (FAILED(res)) {
		const char* error_str = (const char*)error_msgs->GetBufferPointer();
		gfx_error_raise(error_str);
		goto cleanup;
	} else {
		IDirect3DVertexShader9* shader_handle;
		res = impl->dev->CreateVertexShader((const DWORD*)compiled_shader->GetBufferPointer(), &shader_handle);
		shader->vertex_shader.handle = shader_handle;
		shader->vertex_shader.impl = constant_table;
		s_d3d9_build_constant_table(impl, &shader->vertex_shader, constant_table);
		if (FAILED(res)) {
			gfx_error_raise("Failed to create shader.");
			goto cleanup;
		} else {
			compiled_shader->Release();
		}
	}

	// pixel shader
	res = D3DXCompileShader(ps, strlen(ps), NULL, NULL, "main", impl->ps_profile, 0, &compiled_shader, &error_msgs, &constant_table);
	if (FAILED(res)) {
		const char* error_str = (const char*)error_msgs->GetBufferPointer();
		gfx_error_raise(error_str);
		goto cleanup;
	} else {
		IDirect3DPixelShader9* shader_handle;
		res = impl->dev->CreatePixelShader((const DWORD*)compiled_shader->GetBufferPointer(), &shader_handle);
		shader->pixel_shader.handle = shader_handle;
		shader->pixel_shader.impl = constant_table;
		s_d3d9_build_constant_table(impl, &shader->pixel_shader, constant_table);
		if (FAILED(res)) {
			gfx_error_raise("Failed to create pixel shader.");
			goto cleanup;
		} else {
			compiled_shader->Release();
		}
	}

	return shader;

cleanup:
	GFX_FREE(shader);
	return NULL;
}

static void s_d3d9_free_shader(gfx_t* gfx, gfx_shader_t* shader)
{
	IDirect3DVertexShader9* shader_handle = (IDirect3DVertexShader9*)shader->pixel_shader.handle;
	ID3DXConstantTable* constant_table = (ID3DXConstantTable*)shader->pixel_shader.impl;
	constant_table->Release();
	shader_handle->Release();
	GFX_FREE(shader);
}

#if 0
void s_d3d9_shader_set_active(gfx_t* gfx, gfx_shader_t* shader)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;

	if (shader) {
		impl->dev->SetVertexShader((IDirect3DVertexShader9*)shader->vertex_shader.handle);
		impl->dev->SetPixelShader((IDirect3DPixelShader9*)shader->pixel_shader.handle);
	} else {
		impl->dev->SetVertexShader(NULL);
		impl->dev->SetPixelShader(NULL);
	}
}
#endif

static int s_d3d9_shader_send_uniforms(gfx_t* gfx, gfx_shader_data_t* s)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	IDirect3DVertexShader9* shader = (IDirect3DVertexShader9*)s->handle;
	ID3DXConstantTable* table = (ID3DXConstantTable*)s->impl;

	for (int i = 0; i < s->uniform_count; ++i)
	{
		gfx_uniform_t* u = s->uniforms + i;

		// TODO
		// Potentially optimize by setting consts as a contiguous buffer.
		// For now this is really simple though, and that's good.

		D3DXHANDLE uniform = table->GetConstantByName(0, u->name);
		HRESULT res = D3D_OK;
		void* value = &u->u;

		switch (u->type)
		{
		case GFX_UNIFORM_TYPE_BOOL:
			res = table->SetBool(impl->dev, uniform, *(bool*)value);
			break;

		case GFX_UNIFORM_TYPE_FLOAT:
			res = table->SetFloat(impl->dev, uniform, *(float*)value);
			break;

		case GFX_UNIFORM_TYPE_FLOAT2:
			res = table->SetValue(impl->dev, uniform, value, sizeof(float) * 2);
			break;

		case GFX_UNIFORM_TYPE_FLOAT3:
			res = table->SetValue(impl->dev, uniform, value, sizeof(float) * 3);
			break;

		case GFX_UNIFORM_TYPE_FLOAT4:
			res = table->SetValue(impl->dev, uniform, value, sizeof(float) * 4);
			break;

		case GFX_UNIFORM_TYPE_INT:
			res = table->SetInt(impl->dev, uniform, *(int*)value);
			break;

		// Unnecessary to do this in DX. But for GL there's another layer of indirection
		// from texture handle to uniform referring to a handle.
		case GFX_UNIFORM_TYPE_TEXTURE:
		{
			//DWORD stage = table->GetSamplerIndex(uniform);
			//IDirect3DTexture9* texture = (IDirect3DTexture9*)((texture_t*)value)->impl;
			//res = impl->dev->SetTexture(stage, texture);
		}	break;

		case GFX_UNIFORM_TYPE_MATRIX:
			res = table->SetValue(impl->dev, uniform, value, sizeof(float) * 16);
			break;
		}

		if (FAILED(res)) {
			return gfx_error_raise(get_error_string_d3d9(res));
		}
	}

	return GFX_ERROR_SUCCESS;
}

static int s_d3d9_shader_set_screen_wh(gfx_t* gfx, gfx_shader_t* shader, float w, float h)
{
	ID3DXConstantTable* table = (ID3DXConstantTable*)shader->vertex_shader.impl;
	D3DXHANDLE uniform = table->GetConstantByName(0, "u_inv_screen_wh");
	float wh[2] = { 1.0f / w, 1.0f / h };
	HRESULT res = table->SetValue(((d3d9_context_t*)gfx->impl)->dev, uniform, (void*)wh, sizeof(float) * 2);
	if (FAILED(res)) {
		return gfx_error_raise(get_error_string_d3d9(res));
	}
	return GFX_ERROR_SUCCESS;
}

static void s_d3d9_vertex_buffer_release(gfx_vertex_buffer_t* buffer)
{
	for (int i = 0; i < buffer->buffer_count; ++i)
	{
		IDirect3DVertexBuffer9* vbuf = (IDirect3DVertexBuffer9*)buffer->vertex_buffers[i];
		vbuf->Release();
		buffer->vertex_buffers[i] = 0;

		if (buffer->index_type != GFX_INDEX_BUFFER_TYPE_NONE) {
			IDirect3DIndexBuffer9* ibuf = (IDirect3DIndexBuffer9*)buffer->index_buffers[i];
			ibuf->Release();
			buffer->index_buffers[i] = 0;
		}
	}
}

static void vertex_buffer_clean_up_d3d9(gfx_t* gfx, gfx_vertex_buffer_t* buffer)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;

	GFX_DLIST_HEAD_REMOVE(impl->buffers, buffer);
	s_d3d9_vertex_buffer_release(buffer);
}

static int s_d3d9_render_texture_init(d3d9_context_t* impl, d3d9_render_texture_t* render_texture)
{
	IDirect3DTexture9* texture;
	HRESULT res = impl->dev->CreateTexture((UINT)render_texture->w, (UINT)render_texture->h, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
	if (FAILED(res)) {
		return gfx_error_raise("Failed to create d3d9 texture.");
	}

	IDirect3DSurface9* surface;
	res = texture->GetSurfaceLevel(0, &surface);
	if (FAILED(res)) {
		return gfx_error_raise("Failed to get d3d9 surface level.");
	}

	render_texture->texture = texture;
	render_texture->surface = surface;

	return GFX_ERROR_SUCCESS;
}

static void* s_d3d9_render_texture_new(gfx_t* gfx, int w, int h, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode)
{
	GFX_UNUSED(pixel_format); // TODO: use.
	GFX_UNUSED(wrap_mode); // TODO: use.

	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	d3d9_render_texture_t* render_texture = (d3d9_render_texture_t*)GFX_ALLOC(sizeof(d3d9_render_texture_t));
	render_texture->w = w;
	render_texture->h = h;
	render_texture->pixel_format = pixel_format;
	render_texture->wrap_mode = wrap_mode;
	GFX_DLIST_INIT(render_texture);
	GFX_DLIST_HEAD_INSERT(impl->render_textures, render_texture);

	if (s_d3d9_render_texture_init(impl, render_texture) != GFX_ERROR_SUCCESS) {
		return NULL;
	}

	return (void*)render_texture;
}

static void s_d3d9_render_texture_release(d3d9_render_texture_t* render_texture)
{
	render_texture->surface->Release();
	render_texture->surface = 0;
	render_texture->texture->Release();
	render_texture->texture = 0;
}

static void s_d3d9_render_texture_clean_up(gfx_t* gfx, void* render_texture)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	d3d9_render_texture_t* tex = (d3d9_render_texture_t*)render_texture;
	s_d3d9_render_texture_release(tex);
	GFX_DLIST_HEAD_REMOVE(impl->render_textures, tex);
	GFX_FREE(tex);
}

static void s_d3d9_on_device_lost(d3d9_context_t* impl)
{
	// Release everything used with D3DPOOL_DEFAULT.

	{
		gfx_vertex_buffer_t* buffer = impl->buffers;
		gfx_vertex_buffer_t* sentinel = buffer;
		if (buffer) {
			do
			{
				s_d3d9_vertex_buffer_release(buffer);
				buffer = buffer->next;
			} while (buffer != sentinel);
		}
	}

	{
		d3d9_render_texture_t* render_texture = impl->render_textures;
		d3d9_render_texture_t* sentinel = render_texture;
		do
		{
			s_d3d9_render_texture_release(render_texture);
			render_texture = render_texture->next;
		} while (render_texture != sentinel);
	}

	impl->screen_surface->Release();
	impl->screen_surface = 0;
}

static int s_d3d9_on_device_reset(gfx_t* gfx, d3d9_context_t* impl)
{
	// Recreate everything used with D3DPOOL_DEFAULT.

	{
		gfx_vertex_buffer_t* buffer = impl->buffers;
		gfx_vertex_buffer_t* sentinel = buffer;
		if (buffer) {
			do
			{
				if (s_d3d9_vertex_buffer_init(buffer, impl) != GFX_ERROR_SUCCESS) {
					return GFX_ERROR_FAILURE;
				}

				buffer = buffer->next;
			} while (buffer != sentinel);
		}
	}

	{
		d3d9_render_texture_t* render_texture = impl->render_textures;
		d3d9_render_texture_t* sentinel = render_texture;
		do
		{
			if (s_d3d9_render_texture_init(impl, render_texture) != GFX_ERROR_SUCCESS) {
				return GFX_ERROR_FAILURE;
			}

			render_texture = render_texture->next;
		} while (render_texture != sentinel);
	}

	HRESULT res = impl->dev->GetRenderTarget(0, &impl->screen_surface);
	if (FAILED(res)) {
		return gfx_error_raise("Unable to get render target.");
	}

	s_d3d9_setup_render_and_sampler_states(impl->dev);
	gfx_set_alpha(gfx, gfx->alpha_one_for_enabled); // Hacky env scoping, oh well.

	return GFX_ERROR_SUCCESS;
}

static int s_d3d9_handle_lost_device(gfx_t* gfx, d3d9_context_t* impl)
{
	HRESULT hr = impl->dev->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST) {
		// Device is lost and cannot yet be reset.
		return GFX_ERROR_FAILURE;
	} else if (hr == D3DERR_DEVICENOTRESET) {
		// Device is lost and can be reset.
		if (!impl->lost) {
			s_d3d9_on_device_lost(impl);
			impl->lost = 1;
		}

		hr = impl->dev->Reset(&impl->params);
		Sleep(1);

		if (hr == D3DERR_INVALIDCALL) {
			return gfx_error_raise("Reset d3d9 device failed. It is likely some resources have been leaked. Check for D3DX objects, anything allocated with D3DPOOL_DEFAULT, and any surfaces from render targets.");
		}

		return FAILED(hr) ? GFX_ERROR_FAILURE : GFX_ERROR_SUCCESS;
	} else {
		if (impl->lost) {
			if (s_d3d9_on_device_reset(gfx, impl) != GFX_ERROR_SUCCESS) {
				return GFX_ERROR_FAILURE;
			} else {
				impl->lost = 0;
				return GFX_ERROR_SUCCESS;
			}
		} else {
			return GFX_ERROR_SUCCESS;
		}
	}
}

static int s_d3d9_do_draw_calls(gfx_t* gfx)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;

	for (int i = 0; i < gfx->draw_call_count; ++i)
	{
		gfx_draw_call_t call = gfx->draw_calls[i];

		// Set vertex and pixel shadeer.
		HR_CHECK(impl->dev->SetVertexShader((IDirect3DVertexShader9*)call.shader->vertex_shader.handle));
		HR_CHECK(impl->dev->SetPixelShader((IDirect3DPixelShader9*)call.shader->pixel_shader.handle));

		// Set textures.
		ID3DXConstantTable* table = (ID3DXConstantTable*)call.shader->pixel_shader.impl;
		for (int i = 0; i < call.texture_count; ++i)
		{
			IDirect3DTexture9* texture = (IDirect3DTexture9*)call.textures[i];
			UINT index = table->GetSamplerIndex(call.texture_uniform_names[i]);
			if (index <= 255) {
				HR_CHECK(impl->dev->SetTexture(index, texture));
			}
		}

		// Model-view-projection (mvp).
		if (call.use_mvp) {
			gfx_shader_set_mvp(gfx, call.shader, &call.mvp);
		}

		// TODO:
		// Set blend state.
		// Set viewport.

		// Scissor boxing.
		if (call.use_scissor) {
			RECT rect;
			rect.left = call.scissor.left;
			rect.right = call.scissor.right;
			rect.top = call.scissor.top;
			rect.bottom = call.scissor.bottom;
			HR_CHECK(impl->dev->SetScissorRect(&rect));
		} else {
			int w = gfx->screen_w;
			int h = gfx->screen_h;
			RECT rect;
			rect.left = 0;
			rect.right = w;
			rect.top = 0;
			rect.bottom = h;
			HR_CHECK(impl->dev->SetScissorRect(&rect));
		}

		// Copy uniforms out from the draw call to the shader itself.
		s_shader_copy_uniform_data_from_draw_call(gfx, &call);

		// Send uniforms to gpu.
		s_d3d9_shader_send_uniforms(gfx, &call.shader->vertex_shader);
		s_d3d9_shader_send_uniforms(gfx, &call.shader->pixel_shader);

		// Set stream source (vertex buffer).
		IDirect3DVertexBuffer9* verts = (IDirect3DVertexBuffer9*)call.buffer->vertex_buffers[call.vertex_indices.buffer_number];
		int offset = call.vertex_indices.index0 * call.buffer->desc.stride;
		HR_CHECK(impl->dev->SetStreamSource(0, verts, (UINT)offset, call.vertex_indices.stride));
		HR_CHECK(impl->dev->SetVertexDeclaration((IDirect3DVertexDeclaration9*)call.buffer->desc_handle));

		// Set index buffer.
		if (call.buffer->index_type != GFX_INDEX_BUFFER_TYPE_NONE) {
			IDirect3DIndexBuffer9* indices = (IDirect3DIndexBuffer9*)call.buffer->index_buffers[call.index_indices.buffer_number];
			HR_CHECK(impl->dev->SetIndices(indices));
		}

		// DrawPrim.
		if (call.buffer->index_type != GFX_INDEX_BUFFER_TYPE_NONE) {
			// TODO: Audit this code. Needs to be tested with an indexed mesh, and thought-through.
			int v_index0 = call.vertex_indices.index0;
			int v_index1 = call.vertex_indices.index1;
			int i_index0 = call.index_indices.index0;
			int i_index1 = call.index_indices.index1;
			int prim_count = (i_index1 - i_index0) / 3;
			HR_CHECK(impl->dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, v_index0, v_index0, v_index1 - v_index0, i_index0, prim_count));
		} else {
			int v_index0 = call.vertex_indices.index0;
			int v_index1 = call.vertex_indices.index1;
			int prim_count = (v_index1 - v_index0) / 3;
			HR_CHECK(impl->dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, prim_count));
		}

		// Unbind textures.
		for (int i = 0; i < call.texture_count; ++i)
		{
			UINT index = table->GetSamplerIndex(call.texture_uniform_names[i]);
			if (index > 255) {
				return gfx_error_raise("Invalid sampler index encountered.");
			}
			HR_CHECK(impl->dev->SetTexture(index, NULL));
		}
	}

	gfx->draw_call_count = 0;

	return GFX_ERROR_SUCCESS;
}

static void* s_gfx_get_render_texture_handle(gfx_t* gfx)
{
	d3d9_render_texture_t* render_texture = (d3d9_render_texture_t*)gfx->render_texture;
	return (void*)render_texture->texture;
}

static void s_d3d9_gfx_flush_to_texture(gfx_t* gfx, gfx_texture_t* render_texture)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	if (s_d3d9_handle_lost_device(gfx, impl) != GFX_ERROR_SUCCESS) {
		return;
	}

	d3d9_render_texture_t* texture = (d3d9_render_texture_t*)render_texture;

	HR_CHECK(impl->dev->SetRenderTarget(0, texture->surface));
	HR_CHECK(impl->dev->Clear(0, NULL, D3DCLEAR_TARGET, gfx->clear_color, 1.0f, 0));

	s_d3d9_do_draw_calls(gfx);
}

static void s_d3d9_flush(gfx_t* gfx)
{
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	if (s_d3d9_handle_lost_device(gfx, impl) != GFX_ERROR_SUCCESS) {
		return;
	}

	// Render all draw calls to render-texture.
	HR_CHECK(impl->dev->BeginScene());
	s_d3d9_gfx_flush_to_texture(gfx, (gfx_texture_t*)gfx->render_texture);

	// Draw render-texture on screen via upscale shader.
	HR_CHECK(impl->dev->SetRenderTarget(0, impl->screen_surface));
	HR_CHECK(impl->dev->Clear(0, NULL, D3DCLEAR_TARGET, 0xFFFF0000, 1.0f, 0));

	gfx_draw_call_t call;
	call.buffer = gfx->static_render_texture_quad;
	call.shader = gfx->upscale_shader;
	gfx_texture_t* tex = (gfx_texture_t*)s_gfx_get_render_texture_handle(gfx);
	gfx_draw_call_add_texture(&call, tex, "u_screen_image");
	call.vertex_indices = call.buffer->vertex_indices;
	call.index_indices = call.buffer->index_indices;
	gfx_push_draw_call(gfx, &call);

	s_d3d9_do_draw_calls(gfx);

	HR_CHECK(impl->dev->EndScene());
	impl->dev->Present(NULL, NULL, NULL, NULL);
}

static void s_d3d9_gfx_set_alpha(gfx_t* gfx, int one_for_enabled)
{
	gfx->alpha_one_for_enabled = one_for_enabled;
	d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
	HR_CHECK(impl->dev->SetRenderState(D3DRS_ALPHABLENDENABLE, one_for_enabled));
	HR_CHECK(impl->dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, one_for_enabled));

	HR_CHECK(impl->dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	HR_CHECK(impl->dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	HR_CHECK(impl->dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

	HR_CHECK(impl->dev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD));
	HR_CHECK(impl->dev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA));
	HR_CHECK(impl->dev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE));
}

void s_d3d9_free(d3d9_context_t* impl)
{
	s_d3d9_on_device_lost(impl);

	impl->dev->Release();
	impl->d3d9->Release();

	GFX_FREE(impl);
}

#else

// TODO: Stubs needed here.

#endif // GFX_USE_DIRECTX_INCLUDES_AND_IMPORTS

int gfx_vertex_buffer_map(gfx_t* gfx, gfx_vertex_buffer_t* buffer, int vertex_count, void** vertices, int index_count, void** indices)
{
	if (s_is_lost(gfx)) {
		return GFX_ERROR_FAILURE;
	}

	if (gfx->type == GFX_TYPE_D3D9) {
		return s_d3d9_vertex_buffer_map(buffer, vertex_count, vertices, index_count, indices);
	} else {
		return gfx_error_raise("Not yet implemented.");
	}
}

int gfx_vertex_buffer_unmap(gfx_t* gfx, gfx_vertex_buffer_t* buffer)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return s_d3d9_vertex_buffer_unmap(buffer);
	} else {
		return gfx_error_raise("Not yet implemented.");
	}
}

gfx_vertex_buffer_t* gfx_vertex_buffer_new(gfx_t* gfx, gfx_vertex_buffer_params_t* params)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return s_d3d9_vertex_buffer_init(gfx, params);
	} else {
		gfx_error_raise("Not yet implemented.");
		return NULL;
	}
}

void gfx_vertex_buffer_free(gfx_t* gfx, gfx_vertex_buffer_t* buffer)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		vertex_buffer_clean_up_d3d9(gfx, buffer);
	} else {
		gfx_error_raise("Not yet implemented.");
	}

	GFX_FREE(buffer);
}

void matrix_identity(gfx_matrix_t* m)
{
	memset(m, 0, sizeof(*m));
	m->data[0] = 1.0f;
	m->data[5] = 1.0f;
	m->data[10] = 1.0f;
	m->data[15] = 1.0f;
}

void projection_ortho_2d(gfx_matrix_t* projection, float w, float h, float x, float y)
{
	float L = -w / 2.0f;
	float R = w / 2.0f;
	float T = h / 2.0f;
	float B = -h / 2.0f;

	memset(projection, 0, sizeof(*projection));

	// ortho
	projection->data[0] = 2.0f / (R - L);
	projection->data[5] = 2.0f / (T - B);
	projection->data[10] = -0.5f;
	projection->data[15] = 1.0f;

	// translate
	projection->data[12] = -x * projection->data[0];
	projection->data[13] = -y * projection->data[5];
}

gfx_texture_t *texture_create(gfx_t* gfx, gfx_texture_params_t* params)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return (gfx_texture_t*)s_d3d9_texture_init(gfx, params);
	} else {
		gfx_error_raise("Not yet implemented.");
		return NULL;
	}
}

void texture_clean_up(gfx_t* gfx, gfx_texture_t* tex)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		texture_clean_up_d3d9(tex);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}

gfx_shader_t* gfx_shader_new(gfx_t* gfx, gfx_shader_params_t* params)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return s_d3d9_compile_shader(gfx, params);
	} else {
		gfx_error_raise("Not yet implemented.");
		return NULL;
	}
}

void gfx_shader_free(gfx_t* gfx, gfx_shader_t* shader)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return s_d3d9_free_shader(gfx, shader);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}

// Seems not needed -- instead set uniforms upon draw calls.
#if 0
void shader_set_active(gfx_t* gfx, gfx_shader_t* shader)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_shader_set_active(gfx, shader);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}
#endif

static void s_set_uniform_data(gfx_uniform_t* u, void* value, gfx_uniform_type_t type)
{
	u->dirty = 1;
	int size = 0;

	switch (type) {
	default: // fall-through
	case GFX_UNIFORM_TYPE_INVALID:
		gfx_error_raise("Invalid uniform type.");
		return;

	case GFX_UNIFORM_TYPE_BOOL:   size = sizeof(bool); break;
	case GFX_UNIFORM_TYPE_FLOAT:  size = sizeof(float); break;
	case GFX_UNIFORM_TYPE_FLOAT2: size = sizeof(float) * 2; break;
	case GFX_UNIFORM_TYPE_FLOAT3: size = sizeof(float) * 3; break;
	case GFX_UNIFORM_TYPE_FLOAT4: size = sizeof(float) * 4; break;
	case GFX_UNIFORM_TYPE_INT:    size = sizeof(int); break;

	case GFX_UNIFORM_TYPE_TEXTURE:
		size = sizeof(void*);
		break;

	case GFX_UNIFORM_TYPE_MATRIX:
		size = sizeof(float) * 16;
		break;
	}

	memcpy(&u->u, value, size);
}

static int s_set_uniform(gfx_shader_data_t* s, uint64_t h, void* value, gfx_uniform_type_t type)
{
	if (!s) {
		return gfx_error_raise("Unable to find uniform.");
	}

	int found = 0;

	for (int i = 0; i < s->uniform_count; ++i)
	{
		gfx_uniform_t* u = s->uniforms + i;
		if (u->h == h) {
			if (u->type != type) {
				return gfx_error_raise("Type mismatch when assigning uniform.");
			}
			found = 1;
			s_set_uniform_data(u, value, type);
			break;
		}
	}

	if (!found) {
		return GFX_ERROR_FAILURE;
	}

	return GFX_ERROR_SUCCESS;
}

int gfx_shader_set_uniform(gfx_t* gfx, gfx_shader_t* shader, const char* uniform_name, void* value, gfx_uniform_type_t type)
{
	uint64_t h = s_gfx_FNV1a(uniform_name);

	if (s_set_uniform(&shader->vertex_shader, h, value, type) == GFX_ERROR_FAILURE) {
		if (s_set_uniform(&shader->pixel_shader, h, value, type) == GFX_ERROR_FAILURE) {
			return gfx_error_raise("Unable to find uniform.");
		} else {
			return GFX_ERROR_SUCCESS;
		}
	} else {
		return GFX_ERROR_SUCCESS;
	}
}

static void s_shader_copy_uniform_data_from_draw_call(gfx_t* gfx, gfx_draw_call_t* call)
{
	for (int i = 0; i < call->uniform_count; ++i)
	{
		gfx_uniform_t* u = call->uniforms + i;
		int error = gfx_shader_set_uniform(gfx, call->shader, u->name, &u->u, u->type);
		if (error != GFX_ERROR_SUCCESS) {
			gfx_error_raise("Unable to set shader uniform from draw call uniform.");
		}
	}
}

int s_is_lost(gfx_t* gfx)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		d3d9_context_t* impl = (d3d9_context_t*)gfx->impl;
		return impl->lost;
	} else {
		return 0;
	}
}

int gfx_shader_set_mvp(gfx_t* gfx, gfx_shader_t* shader, gfx_matrix_t* mvp)
{
	return gfx_shader_set_uniform(gfx, shader, "u_mvp", mvp->data, GFX_UNIFORM_TYPE_MATRIX);
}

int gfx_shader_set_screen_wh(gfx_t* gfx, gfx_shader_t* shader, float w, float h)
{
	float wh[] = { -1.0f / w, 1.0f / h };
	return gfx_shader_set_uniform(gfx, shader, "u_inv_screen_wh", wh, GFX_UNIFORM_TYPE_FLOAT2);
}

void gfx_draw_call_add_texture(gfx_draw_call_t* call, gfx_texture_t* texture, const char* uniform_name)
{
	int i = call->texture_count++;
	call->textures[i] = texture;
	call->texture_uniform_names[i] = uniform_name;
}

void gfx_draw_call_set_mvp(gfx_draw_call_t* call, gfx_matrix_t* mvp)
{
	call->use_mvp = 1;
	call->mvp = *mvp;
}

void gfx_draw_call_set_scissor_box(gfx_draw_call_t* call, gfx_scissor_t* scissor)
{
	call->use_scissor = 1;
	call->scissor = *scissor;
}

void gfx_draw_call_add_verts(gfx_t* gfx, gfx_draw_call_t* call, void* verts, int vert_count)
{
	// Mapping verts can fail for d3d9 if the device is currently lost.
	void* mapped_verts;
	if (gfx_vertex_buffer_map(gfx, call->buffer, vert_count, &mapped_verts) == GFX_ERROR_SUCCESS) {
		memcpy(mapped_verts, verts, call->buffer->desc.stride * vert_count);
		gfx_vertex_buffer_unmap(gfx, call->buffer);
		call->vertex_indices = call->buffer->vertex_indices;
		call->index_indices = call->buffer->index_indices;
	}
}

void gfx_draw_call_add_uniform(gfx_draw_call_t* call, const char* uniform_name, void* value, gfx_uniform_type_t type)
{
	gfx_uniform_t u;
	u.dirty = 0;
	u.name = uniform_name;
	u.type = type;
	s_set_uniform_data(&u, value, type);
	if (type == GFX_UNIFORM_TYPE_TEXTURE) __debugbreak();

	GFX_ASSERT(call->uniform_count < GFX_DRAW_CALL_MAX_UNIFORMS);
	call->uniforms[call->uniform_count++] = u;
}

static void s_gfx_sprite_quad(float x, float y, float sx, float sy, gfx_vertex_t* out)
{
	out[0].x = -0.5f; out[0].y = 0.5f;  out[0].u = 0; out[0].v = 0;
	out[1].x = 0.5f;  out[1].y = -0.5f; out[1].u = 1; out[1].v = 1;
	out[2].x = 0.5f;  out[2].y = 0.5f;  out[2].u = 1; out[2].v = 0;

	out[3].x = -0.5f; out[3].y =  0.5f; out[3].u = 0; out[3].v = 0;
	out[4].x = -0.5f; out[4].y = -0.5f; out[4].u = 0; out[4].v = 1;
	out[5].x = 0.5f;  out[5].y = -0.5f; out[5].u = 1; out[5].v = 1;

	for (int i = 0; i < 6; ++i)
	{
		out[i].x = out[i].x * sx + x;
		out[i].y = out[i].y * sy + y;
	}
}

gfx_t* gfx_new(gfx_type_t type, gfx_pixel_format_t pixel_format, int screen_w, int screen_h, int tex_w, int tex_h, gfx_upscale_maximum_t upscale, void* platform_handle)
{
	gfx_t* gfx = GFX_NEW(gfx_t);
	gfx->type = type;
	gfx->screen_pixel_format = pixel_format;
	gfx->screen_w = screen_w;
	gfx->screen_h = screen_h;
	gfx->clear_color = 0xFFFFFFFF;
	gfx->upscale_max_setting = upscale;
	gfx->viewport = { 0, 0, (float)screen_w, (float)screen_h };
	gfx->default_projection = projection_identity();
	gfx->draw_call_count = 0;
	gfx->draw_call_capacity = 0;
	gfx->draw_calls = NULL;

	if (gfx->type == GFX_TYPE_D3D9) {
		gfx->impl = s_d3d9_create_impl(gfx, platform_handle);
	} else {
		gfx_error_raise("Not yet implemented.");
		return NULL;
	}

	// Setup the render target texture.
	gfx->render_texture = render_texture_new(gfx, tex_w, tex_h, pixel_format);
	gfx_vertex_buffer_params_t vertex_params;
	vertex_params.type = GFX_VERTEX_BUFFER_TYPE_STATIC;
	vertex_params.stride = sizeof(gfx_vertex_t);
	gfx_vertex_buffer_params_add_attribute(&vertex_params, 2, GFX_OFFEST_OF(gfx_vertex_t, x));
	gfx_vertex_buffer_params_add_attribute(&vertex_params, 2, GFX_OFFEST_OF(gfx_vertex_t, u));
	vertex_params.vertex_count = 6;
	gfx->static_render_texture_quad = gfx_vertex_buffer_new(gfx, &vertex_params);

	// Map full-screen quad (mesh) into a static buffer.
	void* verts;
	gfx_vertex_buffer_map(gfx, gfx->static_render_texture_quad, 6, &verts);
	gfx_vertex_t quad[6];
	s_gfx_sprite_quad(0, 0, 2.0f, 2.0f, quad);
	memcpy(verts, quad, sizeof(gfx_vertex_t) * 6);
	gfx_vertex_buffer_unmap(gfx, gfx->static_render_texture_quad);

	// Calc max scaling factor.
	float scale = 1.0f;
	int i = 0;
	while (1)
	{
		float new_scale = scale + 1;
		int can_scale_x = new_scale * tex_w <= screen_w;
		int can_scale_y = new_scale * tex_h <= screen_h;
		if (can_scale_x && can_scale_y) {
			++i;
			scale = new_scale;
		} else {
			break;
		}
	}

	// Create upscale shader for the full screen render texture.
	// TODO: switch on type for shader code?
	const char* vs = GFX_STR(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float2 uv  : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		float2 u_inv_screen_wh;
		float2 u_scale;

		interp_t main(vertex_t vtx)
		{
			vtx.pos.x *= u_scale.x;
			vtx.pos.y *= u_scale.y;
			float4 posH = float4(vtx.pos, 0, 1);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.uv = vtx.uv;
			return interp;
		}
	);

	const char* ps = GFX_STR(
		struct interp_t
		{
			float4 posH : POSITION0;
			float2 uv   : TEXCOORD0;
		};

		sampler2D u_screen_image;

		float4 main(interp_t interp) : COLOR
		{
			float4 color = tex2D(u_screen_image, interp.uv);
			return color;
		}
	);

	gfx_shader_params_t upscale_shader_params;
	upscale_shader_params.pixel_shader = ps;
	upscale_shader_params.vertex_shader = vs;
	upscale_shader_params.buffer = gfx->static_render_texture_quad;

	gfx->upscale_shader = gfx_shader_new(gfx, &upscale_shader_params);
	float vscale[] = { scale * (float)tex_w / (float)screen_w, scale * (float)tex_h / (float)screen_h };
	gfx_shader_set_uniform(gfx, gfx->upscale_shader, "u_scale", vscale, GFX_UNIFORM_TYPE_FLOAT2);
	gfx_shader_set_screen_wh(gfx, gfx->upscale_shader, screen_w, screen_h); // TODO: Rename to gfx set screen w/h or whatever and do upscales there
	//texture_t temp;
	//temp.impl = s_gfx_get_render_texture_handle(gfx);
	//shader_set_uniform(gfx, &gfx->upscale_shader, "u_screen_image", &temp, UNIFORM_TYPE_TEXTURE);

	// Initialize the debug line rendering.
	gfx_vertex_buffer_params_t line_buffer_params;
	line_buffer_params.type = GFX_VERTEX_BUFFER_TYPE_DYNAMIC;
	line_buffer_params.stride = sizeof(gfx_line_vertex_t);
	gfx_vertex_buffer_params_add_attribute(&line_buffer_params, 2, GFX_OFFEST_OF(gfx_line_vertex_t, x));
	gfx_vertex_buffer_params_add_attribute(&line_buffer_params, 3, GFX_OFFEST_OF(gfx_line_vertex_t, r));
	line_buffer_params.vertex_count = 1024 * 10;
	gfx->line_buffer = gfx_vertex_buffer_new(gfx, &line_buffer_params);

	// TODO: switch on type for shader code?
	const char* line_vs = GFX_STR(
		struct vertex_t
		{
			float2 pos : POSITION0;
			float3 col : TEXCOORD0;
		};

		struct interp_t
		{
			float4 posH : POSITION0;
			float3 col  : TEXCOORD0;
		};

		float4x4 u_mvp;
		float2 u_inv_screen_wh;

		interp_t main(vertex_t vtx)
		{
			float4 posH = mul(float4(round(vtx.pos), 0, 1), u_mvp);

			posH.xy += u_inv_screen_wh;
			interp_t interp;
			interp.posH = posH;
			interp.col = vtx.col;
			return interp;
		}
	);

	const char* line_ps = GFX_STR(
		struct interp_t
		{
			float4 posH : POSITION0;
			float3 col  : TEXCOORD0;
		};

		float4 main(interp_t interp) : COLOR
		{
			float4 color = float4(interp.col, 1);
			return color;
		}
	);

	gfx_shader_params_t line_shader_params;
	line_shader_params.pixel_shader = line_ps;
	line_shader_params.vertex_shader = line_vs;
	line_shader_params.buffer = gfx->line_buffer;
	gfx->line_shader = gfx_shader_new(gfx, &line_shader_params);
	gfx->line_vert_count = 0;
	gfx->line_vert_capacity = 1024 * 10;
	gfx->line_verts = (float*)GFX_ALLOC(sizeof(gfx_line_vertex_t) * gfx->line_vert_capacity);
	gfx->line_depth_test = 0;
	gfx_line_color(gfx, 1.0f, 1.0f, 1.0f);
	gfx_shader_set_screen_wh(gfx, gfx->line_shader, screen_w, screen_h); // TODO: Rename to gfx set screen w/h or whatever and do upscales there

	return gfx;
}

void gfx_clean_up(gfx_t* gfx)
{
	GFX_FREE(gfx->line_verts);
	// TODO: Cleanup debug line shader/buffer or others?

	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_free((d3d9_context_t*)gfx->impl);
	} else {
		gfx_error_raise("Not yet implemented.");
	}

	GFX_FREE(gfx);
}

void gfx_push_draw_call(gfx_t* gfx, gfx_draw_call_t* call)
{
	GFX_CHECK_BUFFER_GROW(gfx, draw_call_count, draw_call_capacity, draw_calls, gfx_draw_call_t, 64);
	gfx->draw_calls[gfx->draw_call_count++] = *call;
}

void gfx_flush(gfx_t* gfx)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_flush(gfx);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}

void gfx_set_alpha(gfx_t* gfx, int one_for_enabled)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_gfx_set_alpha(gfx, one_for_enabled);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}

gfx_type_t gfx_type(gfx_t* gfx)
{
	return gfx->type;
}

void gfx_set_clear_color(gfx_t* gfx, int color)
{
	gfx->clear_color = color;
}

void gfx_line_mvp(gfx_t* gfx, gfx_matrix_t* projection)
{
	gfx_shader_set_mvp(gfx, gfx->line_shader, projection);
}

void gfx_line_color(gfx_t* gfx, float r, float g, float b)
{
	gfx->r = r; gfx->g = g; gfx->b = b;
}

struct gfx_v2_t
{
	float x, y;
};

static gfx_v2_t s_gfx_v2(float x, float y)
{
	gfx_v2_t v;
	v.x = x;
	v.y = y;
	return v;
}

static gfx_v2_t operator-(gfx_v2_t a, gfx_v2_t b) { gfx_v2_t c; c.x = a.x - b.x; c.y = a.y - b.y; return c; }
static gfx_v2_t operator+(gfx_v2_t a, gfx_v2_t b) { gfx_v2_t c; c.x = a.x + b.x; c.y = a.y + b.y; return c; }
static gfx_v2_t operator*(gfx_v2_t a, float b) { gfx_v2_t c; c.x = a.x * b; c.y = a.y * b; return c; }
static gfx_v2_t s_gfx_skew(gfx_v2_t v) { return s_gfx_v2(-v.y, v.x); }
static float s_gfx_len(gfx_v2_t v) { return sqrtf(v.x * v.x + v.y * v.y); }
static gfx_v2_t s_gfx_norm(gfx_v2_t v) { return v * (1.0f / s_gfx_len(v)); }

void gfx_line(gfx_t* gfx, float ax, float ay, float bx, float by)
{
	if (gfx->line_vert_count + 6 > gfx->line_vert_capacity)
	{
		gfx->line_vert_capacity *= 2;
		void* old_verts = gfx->line_verts;
		gfx->line_verts = (float*)GFX_ALLOC(sizeof(gfx_line_vertex_t) * gfx->line_vert_capacity);
		GFX_MEMCPY(gfx->line_verts, old_verts, sizeof(gfx_line_vertex_t) * gfx->line_vert_count);
		GFX_FREE(old_verts);
	}

	gfx_v2_t a0 = s_gfx_v2(ax, ay);
	gfx_v2_t b0 = s_gfx_v2(bx, by);
	gfx_v2_t n = s_gfx_skew(s_gfx_norm(b0 - a0)) * gfx->line_width * 0.5f;
	gfx_v2_t a = a0 + n;
	gfx_v2_t b = a0 - n;
	gfx_v2_t c = b0 - n;
	gfx_v2_t d = b0 + n;

	float rc = gfx->r, bc = gfx->b, gc = gfx->g;
	gfx_line_vertex_t verts[6] = {
		{ a.x, a.y, rc, gc, bc },
		{ c.x, c.y, rc, gc, bc },
		{ b.x, b.y, rc, gc, bc },

		{ a.x, a.y, rc, gc, bc },
		{ d.x, d.y, rc, gc, bc },
		{ c.x, c.y, rc, gc, bc },
	};

	GFX_MEMCPY(gfx->line_verts + gfx->line_vert_count * (sizeof(gfx_line_vertex_t) / sizeof(float)), verts, sizeof(verts));
	gfx->line_vert_count += 6;
}

void gfx_line_width(gfx_t* gfx, float width)
{
	gfx->line_width = width;
}

void gfx_line_depth_test(gfx_t* gfx, int zero_for_off)
{
	gfx->line_depth_test = zero_for_off;
}

void gfx_line_submit_draw_call(gfx_t* gfx)
{
	gfx_draw_call_t call;
	call.buffer = gfx->line_buffer;
	call.shader = gfx->line_shader;
	gfx_draw_call_add_verts(gfx, &call, gfx->line_verts, gfx->line_vert_count);
	gfx_push_draw_call(gfx, &call);
	gfx->line_vert_count = 0;
}

gfx_render_texture_t* render_texture_new(gfx_t* gfx, int w, int h, gfx_pixel_format_t pixel_format, gfx_wrap_mode_t wrap_mode)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		return (gfx_render_texture_t*)s_d3d9_render_texture_new(gfx, w, h, pixel_format, wrap_mode);
	} else {
		gfx_error_raise("Not yet implemented.");
		return NULL;
	}
}

void render_texture_clean_up(gfx_t* gfx, void* render_texture)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_render_texture_clean_up(gfx, render_texture);
	} else {
		gfx_error_raise("Not yet implemented.");
	}
}

void gfx_flush_to_texture(gfx_t* gfx, gfx_texture_t* render_texture)
{
	if (gfx->type == GFX_TYPE_D3D9) {
		s_d3d9_gfx_flush_to_texture(gfx, render_texture);
	} else {
		gfx_error_raise("Not yet implemented.");
	}

	gfx->draw_call_count = 0;
}

#endif // GFX_IMPLEMENTATION_ONCE
#endif // GFX_IMPLEMENTATION
