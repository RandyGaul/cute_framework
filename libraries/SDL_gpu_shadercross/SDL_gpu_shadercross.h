#ifndef SDL_GPU_SHADERCROSS_H
#define SDL_GPU_SHADERCROSS_H

#include <SDL3/SDL.h>

#ifndef SDL_GPU_SHADERCROSS_SPIRVCROSS
#define SDL_GPU_SHADERCROSS_SPIRVCROSS 1
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#ifndef SDL_GPU_SHADERCROSS_HLSL
#define SDL_GPU_SHADERCROSS_HLSL 1
#endif /* SDL_GPU_SHADERCROSS_HLSL */

extern SDL_GpuShaderFormat SDL_ShaderCross_GetShaderFormats(void);

#if SDL_GPU_SHADERCROSS_SPIRVCROSS
extern void *SDL_ShaderCross_CompileFromSPIRV(SDL_GpuDevice *device,
                                              void *createInfo,
                                              SDL_bool isCompute);
#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#if SDL_GPU_SHADERCROSS_HLSL
extern void *SDL_ShaderCross_CompileFromHLSL(SDL_GpuDevice *device,
                                             void *createInfo,
                                             const char *hlslSource,
                                             const char *shaderProfile);
#endif /* SDL_GPU_SHADERCROSS_HLSL */

#endif /* SDL_GPU_SHADERCROSS_H */

#ifdef SDL_GPU_SHADERCROSS_IMPLEMENTATION

SDL_GpuShaderFormat SDL_ShaderCross_GetShaderFormats(void)
{
    return (0
#if SDL_GPU_SHADERCROSS_SPIRVCROSS
      | SDL_GPU_SHADERFORMAT_SPIRV
      | SDL_GPU_SHADERFORMAT_MSL
#endif
#if SDL_GPU_SHADERCROSS_HLSL
      | SDL_GPU_SHADERFORMAT_SPIRV
      | SDL_GPU_SHADERFORMAT_DXBC
      | SDL_GPU_SHADERFORMAT_DXIL
#endif /* SDL_GPU_SHADERCROSS_HLSL */
    );
}

#if SDL_GPU_SHADERCROSS_HLSL

/* Win32 Type Definitions */

typedef int HRESULT;
typedef const void *LPCVOID;
typedef size_t SIZE_T;
typedef const char *LPCSTR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef void *REFIID;

/* DXIL via DXC */

/* dxcompiler Type Definitions */
typedef int BOOL;
typedef void *REFCLSID;
typedef wchar_t *LPCWSTR;
typedef void IDxcBlobEncoding; /* hack, unused */
typedef void IDxcBlobWide; /* hack, unused */
typedef void IDxcIncludeHandler; /* hack, unused */

/* Dynamic Library / Linking */
#ifdef DXCOMPILER_DLL
#undef DXCOMPILER_DLL
#endif
#if defined(_WIN32)
#define DXCOMPILER_DLL "dxcompiler.dll"
#elif defined(__APPLE__)
#define DXCOMPILER_DLL "libdxcompiler.dylib"
#else
#define DXCOMPILER_DLL "libdxcompiler.so"
#endif

#ifdef DXIL_DLL
#undef DXIL_DLL
#endif
#if defined(_WIN32)
#define DXIL_DLL "dxil.dll"
#elif defined(__APPLE__)
#define DXIL_DLL "libdxil.dylib"
#else
#define DXIL_DLL "libdxil.so"
#endif

/* Unlike vkd3d-utils, libdxcompiler.so does not use msabi */
#if !defined(_WIN32)
#define __stdcall
#endif

/* Compiler Interface, _technically_ unofficial but it's MS C++, come on */
typedef enum DXC_OUT_KIND
{
    DXC_OUT_NONE = 0,
    DXC_OUT_OBJECT = 1,
    DXC_OUT_ERRORS = 2,
    DXC_OUT_PDB = 3,
    DXC_OUT_SHADER_HASH = 4,
    DXC_OUT_DISASSEMBLY = 5,
    DXC_OUT_HLSL = 6,
    DXC_OUT_TEXT = 7,
    DXC_OUT_REFLECTION = 8,
    DXC_OUT_ROOT_SIGNATURE = 9,
    DXC_OUT_EXTRA_OUTPUTS = 10,
    DXC_OUT_REMARKS = 11,
    DXC_OUT_TIME_REPORT = 12,
    DXC_OUT_TIME_TRACE = 13,
    DXC_OUT_LAST = DXC_OUT_TIME_TRACE,
    DXC_OUT_NUM_ENUMS,
    DXC_OUT_FORCE_DWORD = 0xFFFFFFFF
} DXC_OUT_KIND;

typedef struct DxcBuffer
{
    LPCVOID Ptr;
    SIZE_T Size;
    UINT Encoding;
} DxcBuffer;

static Uint8 IID_IDxcBlob[] = {
    0x08, 0xFB, 0xA5, 0x8B,
    0x95, 0x51,
    0xE2, 0x40,
    0xAC,
    0x58,
    0x0D,
    0x98,
    0x9C,
    0x3A,
    0x01,
    0x02
};
typedef struct IDxcBlob IDxcBlob;
typedef struct IDxcBlobVtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcBlob *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcBlob *This);
    ULONG(__stdcall *Release)(IDxcBlob *This);

    LPVOID(__stdcall *GetBufferPointer)(IDxcBlob *This);
    SIZE_T(__stdcall *GetBufferSize)(IDxcBlob *This);
} IDxcBlobVtbl;
struct IDxcBlob
{
    IDxcBlobVtbl *lpVtbl;
};

static Uint8 IID_IDxcBlobUtf8[] = {
    0xC9, 0x36, 0xA6, 0x3D,
    0x71, 0xBA,
    0x24, 0x40,
    0xA3,
    0x01,
    0x30,
    0xCB,
    0xF1,
    0x25,
    0x30,
    0x5B
};
typedef struct IDxcBlobUtf8 IDxcBlobUtf8;
typedef struct IDxcBlobUtf8Vtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcBlobUtf8 *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcBlobUtf8 *This);
    ULONG(__stdcall *Release)(IDxcBlobUtf8 *This);

    LPVOID(__stdcall *GetBufferPointer)(IDxcBlobUtf8 *This);
    SIZE_T(__stdcall *GetBufferSize)(IDxcBlobUtf8 *This);

    HRESULT(__stdcall *GetEncoding)(IDxcBlobUtf8 *This, BOOL *pKnown, Uint32 *pCodePage);

    LPCSTR(__stdcall *GetStringPointer)(IDxcBlobUtf8 *This);
    SIZE_T(__stdcall *GetStringLength)(IDxcBlobUtf8 *This);
} IDxcBlobUtf8Vtbl;
struct IDxcBlobUtf8
{
    IDxcBlobUtf8Vtbl *lpVtbl;
};

static Uint8 IID_IDxcResult[] = {
    0xDA, 0x6C, 0x34, 0x58,
    0xE7, 0xDD,
    0x97, 0x44,
    0x94,
    0x61,
    0x6F,
    0x87,
    0xAF,
    0x5E,
    0x06,
    0x59
};
typedef struct IDxcResult IDxcResult;
typedef struct IDxcResultVtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcResult *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcResult *This);
    ULONG(__stdcall *Release)(IDxcResult *This);

    HRESULT(__stdcall *GetStatus)(IDxcResult *This, HRESULT *pStatus);
    HRESULT(__stdcall *GetResult)(IDxcResult *This, IDxcBlob **ppResult);
    HRESULT(__stdcall *GetErrorBuffer)(IDxcResult *This, IDxcBlobEncoding **ppErrors);

    BOOL(__stdcall *HasOutput)(IDxcResult *This, DXC_OUT_KIND dxcOutKind);
    HRESULT(__stdcall *GetOutput)(
        IDxcResult *This,
        DXC_OUT_KIND dxcOutKind,
        REFIID iid,
        void **ppvObject,
        IDxcBlobWide **ppOutputName
    );
    Uint32(__stdcall *GetNumOutputs)(IDxcResult *This);
    DXC_OUT_KIND(__stdcall *GetOutputByIndex)(IDxcResult *This, Uint32 Index);
    DXC_OUT_KIND(__stdcall *PrimaryOutput)(IDxcResult *This);
} IDxcResultVtbl;
struct IDxcResult
{
    IDxcResultVtbl *lpVtbl;
};

static struct
{
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8 Data4[8];
} CLSID_DxcCompiler = {
    .Data1 = 0x73e22d93,
    .Data2 = 0xe6ce,
    .Data3 = 0x47f3,
    .Data4 = { 0xb5, 0xbf, 0xf0, 0x66, 0x4f, 0x39, 0xc1, 0xb0 }
};
static Uint8 IID_IDxcCompiler3[] = {
    0x87, 0x46, 0x8B, 0x22,
    0x6A, 0x5A,
    0x30, 0x47,
    0x90,
    0x0C,
    0x97,
    0x02,
    0xB2,
    0x20,
    0x3F,
    0x54
};
typedef struct IDxcCompiler3 IDxcCompiler3;
typedef struct IDxcCompiler3Vtbl
{
    HRESULT(__stdcall *QueryInterface)(IDxcCompiler3 *This, REFIID riid, void **ppvObject);
    ULONG(__stdcall *AddRef)(IDxcCompiler3 *This);
    ULONG(__stdcall *Release)(IDxcCompiler3 *This);

    HRESULT(__stdcall *Compile)(
        IDxcCompiler3 *This,
        const DxcBuffer *pSource,
        LPCWSTR *pArguments,
        Uint32 argCount,
        IDxcIncludeHandler *pIncludeHandler,
        REFIID riid,
        LPVOID *ppResult
    );

    HRESULT(__stdcall *Disassemble)(
        IDxcCompiler3 *This,
        const DxcBuffer *pObject,
        REFIID riid,
        LPVOID *ppResult
    );
} IDxcCompiler3Vtbl;
struct IDxcCompiler3
{
    const IDxcCompiler3Vtbl *lpVtbl;
};

/* DXCompiler */
static void *dxcompiler_dll = NULL;

typedef HRESULT (__stdcall *DxcCreateInstanceProc)(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv);

static DxcCreateInstanceProc SDL_DxcCreateInstance = NULL;
static IDxcCompiler3 *SDL_DxcInstance = NULL;

static void *SDL_ShaderCross_INTERNAL_CompileDXC(
    SDL_GpuDevice *device,
    void *createInfo,
    const char *hlslSource,
    const char *shaderProfile,
    SDL_bool spirv)
{
    DxcBuffer source;
    IDxcResult *dxcResult;
    DXC_OUT_KIND blobOut;
    IDxcBlob *blob;
    IDxcBlobUtf8 *errors;
    LPCWSTR args[] = {
        (LPCWSTR)L"-E",
        (LPCWSTR)L"main", /* FIXME */
        NULL,
        NULL,
        NULL
    };
    Uint32 argCount = 2;
    HRESULT ret;
    void *result;

    /* FIXME: dxcompiler could probably be loaded in a better spot */
    if (dxcompiler_dll == NULL) {
        dxcompiler_dll = SDL_LoadObject(DXCOMPILER_DLL);
        if (dxcompiler_dll == NULL) {
            return NULL;
        }
    }

    /* Try to load DXIL, we don't need it directly but if it doesn't exist the code will not be loadable */
    if (!spirv) {
        if (!SDL_LoadObject(DXIL_DLL)) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "Failed to load DXIL library, this will cause pipeline creation failures!");
            return NULL;
        }
        // ???
        //SDL_UnloadObject(DXIL_DLL);
    }

    if (SDL_DxcCreateInstance == NULL) {
        SDL_DxcCreateInstance = (DxcCreateInstanceProc)SDL_LoadFunction(dxcompiler_dll, "DxcCreateInstance");
        if (SDL_DxcCreateInstance == NULL) {
            return NULL;
        }
    }

    if (SDL_DxcInstance == NULL) {
        SDL_DxcCreateInstance(&CLSID_DxcCompiler,
                              IID_IDxcCompiler3,
                              (void**) &SDL_DxcInstance);
        if (SDL_DxcInstance == NULL) {
            return NULL;
        }
    }

    source.Ptr = hlslSource;
    source.Size = SDL_strlen(hlslSource) + 1;
    source.Encoding = 0; /* FIXME: The docs for this are a _bit_ scarce */

    if (SDL_strcmp(shaderProfile, "ps_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"ps_6_0";
    } else if (SDL_strcmp(shaderProfile, "vs_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"vs_6_0";
    } else if (SDL_strcmp(shaderProfile, "cs_6_0") == 0) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"cs_6_0";
    }

    if (spirv) {
        args[argCount++] = (LPCWSTR)L"-spirv";
    }

    ret = SDL_DxcInstance->lpVtbl->Compile(
        SDL_DxcInstance,
        &source,
        args,
        argCount,
        NULL,
        IID_IDxcResult,
        (void**) &dxcResult);

    if (ret < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "IDxcShaderCompiler3::Compile failed: %X",
                     ret);
        return NULL;
    } else if (dxcResult == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "HLSL compilation failed with no IDxcResult");
        return NULL;
    }


    blobOut = dxcResult->lpVtbl->PrimaryOutput(dxcResult);
    if (blobOut == DXC_OUT_ERRORS) {
        dxcResult->lpVtbl->GetOutput(dxcResult,
                                     DXC_OUT_ERRORS,
                                     IID_IDxcBlobUtf8,
                                     (void**) &errors,
                                     NULL);
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "HLSL compilation failed: %s",
                      errors->lpVtbl->GetStringPointer(errors));
        dxcResult->lpVtbl->Release(dxcResult);
        return NULL;
    } else if (blobOut == DXC_OUT_OBJECT) {
        dxcResult->lpVtbl->GetOutput(dxcResult,
                                     DXC_OUT_OBJECT,
                                     IID_IDxcBlob,
                                     (void**) &blob,
                                     NULL);
        if (blob == NULL || blob->lpVtbl->GetBufferSize(blob) == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "IDxcBlob fetch failed");
            dxcResult->lpVtbl->Release(dxcResult);
            return NULL;
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "Unexpeced DxcResult output: %d",
                     blobOut);
        dxcResult->lpVtbl->Release(dxcResult);
        return NULL;
    }

    if (shaderProfile[0] == 'c' && shaderProfile[1] == 's') {
        SDL_GpuComputePipelineCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuComputePipelineCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8*)blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = spirv ?
            SDL_GPU_SHADERFORMAT_SPIRV :
            SDL_GPU_SHADERFORMAT_DXIL;

        result = SDL_GpuCreateComputePipeline(device, &newCreateInfo);
    } else {
        SDL_GpuShaderCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuShaderCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8*)blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = spirv ?
            SDL_GPU_SHADERFORMAT_SPIRV :
            SDL_GPU_SHADERFORMAT_DXIL;

        result = SDL_GpuCreateShader(device, &newCreateInfo);
    }
    dxcResult->lpVtbl->Release(dxcResult);

    return result;
}

/* DXBC via FXC */

/* d3dcompiler Type Definitions */
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

static void *SDL_ShaderCross_INTERNAL_CompileFXC(
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
        newCreateInfo.code = (const Uint8*)blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_GpuCreateComputePipeline(device, &newCreateInfo);
    } else {
        SDL_GpuShaderCreateInfo newCreateInfo;
        newCreateInfo = *(SDL_GpuShaderCreateInfo *)createInfo;
        newCreateInfo.code = (const Uint8*)blob->lpVtbl->GetBufferPointer(blob);
        newCreateInfo.codeSize = blob->lpVtbl->GetBufferSize(blob);
        newCreateInfo.format = SDL_GPU_SHADERFORMAT_DXBC;

        result = SDL_GpuCreateShader(device, &newCreateInfo);
    }

    blob->lpVtbl->Release(blob);

    return result;
}

extern void *SDL_ShaderCross_CompileFromHLSL(SDL_GpuDevice *device,
                                             void *createInfo,
                                             const char *hlslSource,
                                             const char *shaderProfile)
{
    switch (SDL_GpuGetDriver(device)) {
    case SDL_GPU_DRIVER_D3D11:
        return SDL_ShaderCross_INTERNAL_CompileFXC(device, createInfo, hlslSource, shaderProfile);
    case SDL_GPU_DRIVER_D3D12:
        return SDL_ShaderCross_INTERNAL_CompileDXC(device, createInfo, hlslSource, shaderProfile, SDL_FALSE);
    case SDL_GPU_DRIVER_VULKAN:
        return SDL_ShaderCross_INTERNAL_CompileDXC(device, createInfo, hlslSource, shaderProfile, SDL_TRUE);
    default:
        SDL_SetError("SDL_ShaderCross_CompileFromHLSL: Unexpected SDL_GpuBackend");
        return NULL;
    }
}

#endif /* SDL_GPU_SHADERCROSS_HLSL */

#if SDL_GPU_SHADERCROSS_SPIRVCROSS

#if !SDL_GPU_SHADERCROSS_HLSL
#error SDL_GPU_SHADERCROSS_HLSL must be enabled for SDL_GPU_SHADERCROSS_SPIRVCROSS!
#endif /* !SDL_GPU_SHADERCROSS_HLSL */

#include "spirv_cross_c.h"

#ifndef SDL_GPU_SHADERCROSS_STATIC

#ifndef SDL_GPU_SPIRV_CROSS_DLL
    #if defined(_WIN32)
        #define SDL_GPU_SPIRV_CROSS_DLL "spirv-cross-c-shared.dll"
    #elif defined(__APPLE__)
        #define SDL_GPU_SPIRV_CROSS_DLL "libspirv-cross-c-shared.0.dylib"
    #else
        #define SDL_GPU_SPIRV_CROSS_DLL "libspirv-cross-c-shared.so.0"
    #endif
#endif /* SDL_GPU_SPIRV_CROSS_DLL */

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

#else /* SDL_GPU_SHADERCROSS_STATIC */

#define SDL_spvc_context_create spvc_context_create
#define SDL_spvc_context_destroy spvc_context_destroy
#define SDL_spvc_context_parse_spirv spvc_context_parse_spirv
#define SDL_spvc_context_create_compiler spvc_context_create_compiler
#define SDL_spvc_compiler_create_compiler_options spvc_compiler_create_compiler_options
#define SDL_spvc_compiler_options_set_uint spvc_compiler_options_set_uint
#define SDL_spvc_compiler_install_compiler_options spvc_compiler_install_compiler_options
#define SDL_spvc_compiler_compile spvc_compiler_compile
#define SDL_spvc_context_get_last_error_string spvc_context_get_last_error_string
#define SDL_spvc_compiler_get_execution_model spvc_compiler_get_execution_model
#define SDL_spvc_compiler_get_cleansed_entry_point_name spvc_compiler_get_cleansed_entry_point_name

#endif /* SDL_GPU_SHADERCROSS_STATIC */

#define SPVC_ERROR(func) \
    SDL_SetError(#func " failed: %s", SDL_spvc_context_get_last_error_string(context))

void *SDL_ShaderCross_CompileFromSPIRV(
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

    switch (SDL_GpuGetDriver(device)) {
    case SDL_GPU_DRIVER_VULKAN:
        if (isCompute) {
            return SDL_GpuCreateComputePipeline(device, (SDL_GpuComputePipelineCreateInfo*) originalCreateInfo);
        } else {
            return SDL_GpuCreateShader(device, (SDL_GpuShaderCreateInfo*) originalCreateInfo);
        }
    case SDL_GPU_DRIVER_D3D11:
        backend = SPVC_BACKEND_HLSL;
        format = SDL_GPU_SHADERFORMAT_DXBC;
        break;
    case SDL_GPU_DRIVER_D3D12:
        backend = SPVC_BACKEND_HLSL;
        format = SDL_GPU_SHADERFORMAT_DXIL;
        break;
    case SDL_GPU_DRIVER_METAL:
        backend = SPVC_BACKEND_MSL;
        format = SDL_GPU_SHADERFORMAT_MSL;
        break;
    default:
        SDL_SetError("SDL_ShaderCross_CompileFromSPIRV: Unexpected SDL_GpuBackend");
        return NULL;
    }

    /* FIXME: spirv-cross could probably be loaded in a better spot */
#ifndef SDL_GPU_SHADERCROSS_STATIC
    if (spirvcross_dll == NULL) {
        spirvcross_dll = SDL_LoadObject(SDL_GPU_SPIRV_CROSS_DLL);
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
#endif /* SDL_GPU_SHADERCROSS_STATIC */

    /* Create the SPIRV-Cross context */
    result = SDL_spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return NULL;
    }

    /* SDL_GpuShaderCreateInfo and SDL_GpuComputePipelineCreateInfo
     * share the same struct layout for their first 3 members, which
     * is all we need to transpile them!
     */
    createInfo = (SDL_GpuShaderCreateInfo *)originalCreateInfo;

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
            shadermodel = 60;
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
            compiledResult = SDL_ShaderCross_CompileFromHLSL(
                device,
                &newCreateInfo,
                translated_source,
                (shadermodel == 50) ? "cs_5_0" : "cs_6_0");
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
                profile = (shadermodel == 50) ? "vs_5_0" : "vs_6_0";
            } else {
                profile = (shadermodel == 50) ? "ps_5_0" : "ps_6_0";
            }
            compiledResult = SDL_ShaderCross_CompileFromHLSL(
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

#endif /* SDL_GPU_SHADERCROSS_SPIRVCROSS */

#endif /* SDL_GPU_SHADERCROSS_IMPLEMENTATION */
