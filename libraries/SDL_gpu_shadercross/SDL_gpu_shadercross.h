#ifndef SDL_GPU_SHADERCROSS_H
#define SDL_GPU_SHADERCROSS_H

#include <SDL3/SDL.h>

#ifndef SDL_GPU_SHADERCROSS_SPIRVCROSS
#define SDL_GPU_SHADERCROSS_SPIRVCROSS 1
#endif

#ifndef SDL_GPU_SHADERCROSS_HLSL
#define SDL_GPU_SHADERCROSS_HLSL 1
#endif

#if SDL_GPU_SHADERCROSS_SPIRVCROSS
extern void *SDL_CompileFromSPIRV(SDL_GpuDevice *device,
                                  void *createInfo,
                                  SDL_bool isCompute);
#endif

#if SDL_GPU_SHADERCROSS_HLSL
extern void *SDL_CompileFromHLSL(SDL_GpuDevice *device,
                                 void *createInfo,
                                 const char *hlslSource,
                                 const char *shaderProfile);
#endif

#endif /* SDL_GPU_SHADERCROSS_H */

#ifdef SDL_GPU_SHADERCROSS_IMPLEMENTATION

#if SDL_GPU_SHADERCROSS_HLSL

/* Win32 Type Definitions */
typedef int HRESULT;
typedef const void *LPCVOID;
typedef size_t SIZE_T;
typedef const char *LPCSTR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void *REFIID;          /* hack, unused */
typedef void D3D_SHADER_MACRO; /* hack, unused */
typedef void ID3DInclude;      /* hack, unused */

/* Dynamic Library / Linking */
#ifdef D3DCOMPILER_DLL
#undef D3DCOMPILER_DLL
#endif
#if defined(_WIN32)
#define D3DCOMPILER_DLL "d3dcompiler_47.dll"
#elif defined(__APPLE__)
#define D3DCOMPILER_DLL "libvkd3d-utils.1.dylib"
#else
#define D3DCOMPILER_DLL "libvkd3d-utils.so.1"
#endif

/* __stdcall declaration, largely taken from vkd3d_windows.h */
#ifndef _WIN32
#ifdef __stdcall
#undef __stdcall
#endif
#if defined(__x86_64__) || defined(__arm64__)
#define __stdcall __attribute__((ms_abi))
#else
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) || defined(__APPLE__)
#define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#else
#define __stdcall __attribute__((__stdcall__))
#endif
#endif
#endif

/* ID3DBlob definition, used by both D3DCompiler and DXCompiler */
typedef struct ID3DBlob ID3DBlob;
typedef struct ID3DBlobVtbl
{
    HRESULT(__stdcall *QueryInterface)
    (ID3DBlob *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)
    (ID3DBlob *This);
    ULONG(__stdcall *Release)
    (ID3DBlob *This);
    LPVOID(__stdcall *GetBufferPointer)
    (ID3DBlob *This);
    SIZE_T(__stdcall *GetBufferSize)
    (ID3DBlob *This);
} ID3DBlobVtbl;
struct ID3DBlob
{
    const ID3DBlobVtbl *lpVtbl;
};
#define ID3D10Blob ID3DBlob

/* D3DCompiler */
static void *d3dcompiler_dll = NULL;

typedef HRESULT(__stdcall *pfn_D3DCompile)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    const D3D_SHADER_MACRO *pDefines,
    ID3DInclude *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob **ppCode,
    ID3DBlob **ppErrorMsgs);

static pfn_D3DCompile SDL_D3DCompile = NULL;

void *SDL_CompileFromHLSL(
    SDL_GpuDevice *device,
    void *createInfo,
    const char *hlslSource,
    const char *shaderProfile)
{
    ID3DBlob *blob;
    ID3DBlob *errorBlob;
    HRESULT ret;
    void *result;

    /* FIXME: d3dcompiler could probably be loaded in a better spot */
    if (d3dcompiler_dll == NULL) {
        d3dcompiler_dll = SDL_LoadObject(D3DCOMPILER_DLL);
        if (d3dcompiler_dll == NULL) {
            return NULL;
        }
    }

    if (SDL_D3DCompile == NULL) {
        SDL_D3DCompile = (pfn_D3DCompile)SDL_LoadFunction(d3dcompiler_dll, "D3DCompile");
        if (SDL_D3DCompile == NULL) {
            return NULL;
        }
    }

    ret = SDL_D3DCompile(
        hlslSource,
        SDL_strlen(hlslSource),
        NULL,
        NULL,
        NULL,
        ((SDL_GpuShaderCreateInfo *)createInfo)->entryPointName,
        shaderProfile,
        0,
        0,
        &blob,
        &errorBlob);

    if (ret < 0) {
        SDL_LogError(
            SDL_LOG_CATEGORY_GPU,
            "HLSL compilation failed: %s",
            (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob));
        return NULL;
    }

    if (shaderProfile[0] == 'c' && shaderProfile[1] == 's') {
        SDL_GpuComputePipelineCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuComputePipelineCreateInfo *)createInfo;
        newCreateInfo.code = blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_GpuCreateComputePipeline(device, &newCreateInfo);
    } else {
        SDL_GpuShaderCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuShaderCreateInfo *)createInfo;
        newCreateInfo.code = blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_GpuCreateShader(device, &newCreateInfo);
    }

    blob->lpVtbl->Release(blob);

    return result;
}

#endif

#if SDL_GPU_SHADERCROSS_SPIRVCROSS

#if !SDL_GPU_SHADERCROSS_HLSL
#error SDL_GPU_SHADERCROSS_HLSL must be enabled for SDL_GPU_SHADERCROSS_SPIRVCROSS!
#endif

#include <spirv_cross_c.h>

#if defined(_WIN32)
#define SPIRV_CROSS_DLL "spirv-cross-c-shared.dll"
#elif defined(__APPLE__)
#define SPIRV_CROSS_DLL "libspirv-cross-c-shared.0.dylib"
#else
#define SPIRV_CROSS_DLL "libspirv-cross-c-shared.so.0"
#endif

#define SPVC_ERROR(func) \
    SDL_SetError(#func " failed: %s", SDL_spvc_context_get_last_error_string(context))

static void *spirvcross_dll = NULL;

typedef spvc_result (*pfn_spvc_context_create)(spvc_context *context);
typedef void (*pfn_spvc_context_destroy)(spvc_context);
typedef spvc_result (*pfn_spvc_context_parse_spirv)(spvc_context, const SpvId *, size_t, spvc_parsed_ir *);
typedef spvc_result (*pfn_spvc_context_create_compiler)(spvc_context, spvc_backend, spvc_parsed_ir, spvc_capture_mode, spvc_compiler *);
typedef spvc_result (*pfn_spvc_compiler_create_compiler_options)(spvc_compiler, spvc_compiler_options *);
typedef spvc_result (*pfn_spvc_compiler_options_set_uint)(spvc_compiler_options, spvc_compiler_option, unsigned);
typedef spvc_result (*pfn_spvc_compiler_install_compiler_options)(spvc_compiler, spvc_compiler_options);
typedef spvc_result (*pfn_spvc_compiler_compile)(spvc_compiler, const char **);
typedef const char *(*pfn_spvc_context_get_last_error_string)(spvc_context);
typedef SpvExecutionModel (*pfn_spvc_compiler_get_execution_model)(spvc_compiler compiler);
typedef const char *(*pfn_spvc_compiler_get_cleansed_entry_point_name)(spvc_compiler compiler, const char *name, SpvExecutionModel model);

static pfn_spvc_context_create SDL_spvc_context_create = NULL;
static pfn_spvc_context_destroy SDL_spvc_context_destroy = NULL;
static pfn_spvc_context_parse_spirv SDL_spvc_context_parse_spirv = NULL;
static pfn_spvc_context_create_compiler SDL_spvc_context_create_compiler = NULL;
static pfn_spvc_compiler_create_compiler_options SDL_spvc_compiler_create_compiler_options = NULL;
static pfn_spvc_compiler_options_set_uint SDL_spvc_compiler_options_set_uint = NULL;
static pfn_spvc_compiler_install_compiler_options SDL_spvc_compiler_install_compiler_options = NULL;
static pfn_spvc_compiler_compile SDL_spvc_compiler_compile = NULL;
static pfn_spvc_context_get_last_error_string SDL_spvc_context_get_last_error_string = NULL;
static pfn_spvc_compiler_get_execution_model SDL_spvc_compiler_get_execution_model = NULL;
static pfn_spvc_compiler_get_cleansed_entry_point_name SDL_spvc_compiler_get_cleansed_entry_point_name = NULL;

void *SDL_CompileFromSPIRV(
    SDL_GpuDevice *device,
    void *originalCreateInfo,
    SDL_bool isCompute)
{
    SDL_GpuShaderCreateInfo *createInfo;
    spvc_result result;
    spvc_backend backend;
    unsigned shadermodel;
    SDL_GpuShaderFormat format;
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_compiler_options options = NULL;
    const char *translated_source;
    const char *cleansed_entrypoint;
    void *compiledResult;

    /* SDL_GpuShaderCreateInfo and SDL_GpuComputePipelineCreateInfo
     * share the same struct layout for their first 3 members, which
     * is all we need to transpile them!
     */
    createInfo = (SDL_GpuShaderCreateInfo *)originalCreateInfo;

    switch (SDL_GpuGetDriver(device)) {
    case SDL_GPU_DRIVER_D3D11:
    case SDL_GPU_DRIVER_D3D12:
        backend = SPVC_BACKEND_HLSL;
        format = SDL_GPU_SHADERFORMAT_DXBC;
        break;
    case SDL_GPU_DRIVER_METAL:
        backend = SPVC_BACKEND_MSL;
        format = SDL_GPU_SHADERFORMAT_MSL;
        break;
    default:
        SDL_SetError("SDL_CreateShaderFromSPIRV: Unexpected SDL_GpuBackend");
        return NULL;
    }

    /* FIXME: spirv-cross could probably be loaded in a better spot */
    if (spirvcross_dll == NULL) {
        spirvcross_dll = SDL_LoadObject(SPIRV_CROSS_DLL);
        if (spirvcross_dll == NULL) {
            return NULL;
        }
    }

#define CHECK_FUNC(func)                                                  \
    if (SDL_##func == NULL) {                                             \
        SDL_##func = (pfn_##func)SDL_LoadFunction(spirvcross_dll, #func); \
        if (SDL_##func == NULL) {                                         \
            return NULL;                                                  \
        }                                                                 \
    }
    CHECK_FUNC(spvc_context_create)
    CHECK_FUNC(spvc_context_destroy)
    CHECK_FUNC(spvc_context_parse_spirv)
    CHECK_FUNC(spvc_context_create_compiler)
    CHECK_FUNC(spvc_compiler_create_compiler_options)
    CHECK_FUNC(spvc_compiler_options_set_uint)
    CHECK_FUNC(spvc_compiler_install_compiler_options)
    CHECK_FUNC(spvc_compiler_compile)
    CHECK_FUNC(spvc_context_get_last_error_string)
    CHECK_FUNC(spvc_compiler_get_execution_model)
    CHECK_FUNC(spvc_compiler_get_cleansed_entry_point_name)
#undef CHECK_FUNC

    /* Create the SPIRV-Cross context */
    result = SDL_spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return NULL;
    }

    /* Parse the SPIR-V into IR */
    result = SDL_spvc_context_parse_spirv(context, (const SpvId *)createInfo->code, createInfo->codeSize / sizeof(SpvId), &ir);
    if (result < 0) {
        SPVC_ERROR(spvc_context_parse_spirv);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Create the cross-compiler */
    result = SDL_spvc_context_create_compiler(context, backend, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
    if (result < 0) {
        SPVC_ERROR(spvc_context_create_compiler);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Set up the cross-compiler options */
    result = SDL_spvc_compiler_create_compiler_options(compiler, &options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_create_compiler_options);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    if (backend == SPVC_BACKEND_HLSL) {
        if (SDL_GpuGetDriver(device) == SDL_GPU_DRIVER_D3D11) {
            shadermodel = 50;
        } else {
            shadermodel = 51;
        }
        SDL_spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, shadermodel);
        SDL_spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_NONWRITABLE_UAV_TEXTURE_AS_SRV, 1);
    }

    result = SDL_spvc_compiler_install_compiler_options(compiler, options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_install_compiler_options);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Compile to the target shader language */
    result = SDL_spvc_compiler_compile(compiler, &translated_source);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_compile);
        SDL_spvc_context_destroy(context);
        return NULL;
    }

    /* Determine the "cleansed" entrypoint name (e.g. main -> main0 on MSL) */
    cleansed_entrypoint = SDL_spvc_compiler_get_cleansed_entry_point_name(
        compiler,
        createInfo->entryPointName,
        SDL_spvc_compiler_get_execution_model(compiler));

    /* Copy the original create info, but with the new source code */
    if (isCompute) {
        SDL_GpuComputePipelineCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuComputePipelineCreateInfo *)createInfo;
        newCreateInfo.format = format;
        newCreateInfo.entryPointName = cleansed_entrypoint;

        if (backend == SPVC_BACKEND_HLSL) {
            compiledResult = SDL_CompileFromHLSL(
                device,
                &newCreateInfo,
                translated_source,
                (shadermodel == 50) ? "cs_5_0" : "cs_5_1");
        } else {
            newCreateInfo.code = (const Uint8 *)translated_source;
            newCreateInfo.codeSize = SDL_strlen(translated_source) + 1;
            compiledResult = SDL_GpuCreateComputePipeline(device, &newCreateInfo);
        }
    } else {
        SDL_GpuShaderCreateInfo newCreateInfo;
        newCreateInfo = *createInfo;
        newCreateInfo.format = format;
        newCreateInfo.entryPointName = cleansed_entrypoint;

        if (backend == SPVC_BACKEND_HLSL) {
            const char *profile;
            if (newCreateInfo.stage == SDL_GPU_SHADERSTAGE_VERTEX) {
                profile = (shadermodel == 50) ? "vs_5_0" : "vs_5_1";
            } else {
                profile = (shadermodel == 50) ? "ps_5_0" : "ps_5_1";
            }
            compiledResult = SDL_CompileFromHLSL(
                device,
                &newCreateInfo,
                translated_source,
                profile);
        } else {
            newCreateInfo.code = (const Uint8 *)translated_source;
            newCreateInfo.codeSize = SDL_strlen(translated_source) + 1;
            compiledResult = SDL_GpuCreateShader(device, &newCreateInfo);
        }
    }

    /* Clean up */
    SDL_spvc_context_destroy(context);

    return compiledResult;
}

#endif

#endif /* SDL_GPU_SHADERCROSS_IMPLEMENTATION */
