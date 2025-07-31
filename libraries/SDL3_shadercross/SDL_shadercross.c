/*
  Simple DirectMedia Layer Shader Cross Compiler
  Copyright (C) 2024 Sam Lantinga <slouken@libsdl.org>

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

#include <SDL3_shadercross/SDL_shadercross.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>

/* Constants */
#define MAX_DEFINES 64
#define MAX_DEFINE_STRING_LENGTH 256

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
#ifdef SDL_SHADERCROSS_DXC

/* dxcompiler Type Definitions */
typedef int BOOL;
typedef void *REFCLSID;
typedef wchar_t *LPCWSTR;
typedef void IDxcBlobEncoding;   /* hack, unused */
typedef void IDxcBlobWide;       /* hack, unused */
typedef void IDxcIncludeHandler; /* hack, unused */

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
    // DXC_OUT_FORCE_DWORD = 0xFFFFFFFF
} DXC_OUT_KIND;

#define DXC_CP_UTF8  65001
#define DXC_CP_UTF16 1200
#define DXC_CP_UTF32 12000
/* This is for binary, ANSI-text, or to tell the compiler to try autodetecting UTF using the BOM */
#define DXC_CP_ACP 0

typedef struct DxcBuffer
{
    LPCVOID Ptr;
    SIZE_T Size;
    UINT Encoding;
} DxcBuffer;

/* *INDENT-OFF* */ // clang-format off

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

// We need all this DxcUtils garbage for DXC include dir support. Thanks Microsoft!
typedef struct IMalloc IMalloc;
typedef struct IStream IStream;
typedef struct DxcDefine DxcDefine;
typedef struct IDxcCompilerArgs IDxcCompilerArgs;

static struct
{
    Uint32 Data1;
    Uint16 Data2;
    Uint16 Data3;
    Uint8 Data4[8];
} CLSID_DxcUtils = {
    .Data1 = 0x6245d6af,
    .Data2 = 0x66e0,
    .Data3 = 0x48fd,
    .Data4 = {0x80, 0xb4, 0x4d, 0x27, 0x17, 0x96, 0x74, 0x8c}};
static Uint8 IID_IDxcUtils[] = {
    0xcb, 0xc4, 0x05, 0x46,
    0x19, 0x20,
    0x2a, 0x49,
    0xad,
    0xa4,
    0x65,
    0xf2,
    0x0b,
    0xb7,
    0xd6,
    0x7f
};
typedef struct IDxcUtilsVtbl
{
    HRESULT (__stdcall *QueryInterface)(void *pSelf, REFIID riid, void **ppvObject);
    ULONG (__stdcall *AddRef)(void *pSelf);
    ULONG (__stdcall *Release)(void *pSelf);

    HRESULT (__stdcall *CreateBlobFromBlob)(void *pSelf, IDxcBlob *pBlob, UINT offset, UINT length, IDxcBlob **ppResult);
    HRESULT (__stdcall *CreateBlobFromPinned)(void *pSelf, LPCVOID pData, UINT size, UINT codePage, IDxcBlobEncoding **pBlobEncoding);
    HRESULT (__stdcall *MoveToBlob)(void *pSelf, LPCVOID pData, IMalloc *pIMalloc, UINT size, UINT codePage, IDxcBlobEncoding **pBlobEncoding);
    HRESULT (__stdcall *CreateBlob)(void *pSelf, LPCVOID pData, UINT size, UINT codePage, IDxcBlobEncoding **pBlobEncoding);
    HRESULT (__stdcall *LoadFile)(void *pSelf, LPCWSTR pFileName, UINT *pCodePage, IDxcBlobEncoding **pBlobEncoding);
    HRESULT (__stdcall *CreateReadOnlyStreamFromBlob)(void *pSelf, IDxcBlob *pBlob, IStream **ppStream);
    HRESULT (__stdcall *CreateDefaultIncludeHandler)(void *pSelf, IDxcIncludeHandler **ppResult);
    HRESULT (__stdcall *GetBlobAsUtf8)(void *pSelf, IDxcBlob *pBlob, IDxcBlobUtf8 **pBlobEncoding);
    HRESULT (__stdcall *GetBlobAsWide)(void *pSelf, IDxcBlob *pBlob, IDxcBlobWide **pBlobEncoding);
    HRESULT (__stdcall *GetDxilContainerPart)(void *pSelf, const DxcBuffer *pShader, UINT DxcPart, void **ppPartData, UINT *pPartSizeInBytes);
    HRESULT (__stdcall *CreateReflection)(void *pSelf, const DxcBuffer *pData, REFIID iid, void **ppvReflection);
    HRESULT (__stdcall *BuildArguments)(void *pSelf, LPCWSTR pSourceName, LPCWSTR pEntryPoint, LPCWSTR pTargetProfile, LPCWSTR *pArguments, UINT argCount, const DxcDefine *pDefines, UINT defineCount, IDxcCompilerArgs **ppArgs);
    HRESULT (__stdcall *GetPDBContents)(void *pSelf, IDxcBlob *pPDBBlob, IDxcBlob **ppHash, IDxcBlob **ppContainer);
} IDxcUtilsVtbl;

typedef struct IDxcUtils IDxcUtils;
struct IDxcUtils
{
    const IDxcUtilsVtbl *lpVtbl;
};

/* *INDENT-ON* */ // clang-format on

/* DXCompiler */
#if defined(SDL_PLATFORM_XBOXONE) || defined(SDL_PLATFORM_XBOXSERIES) || defined(SDL_PLATFORM_WINDOWS)
extern HRESULT __stdcall DxcCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
#else
extern HRESULT DxcCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
#endif

#endif /* SDL_SHADERCROSS_DXC */

static void *SDL_ShaderCross_INTERNAL_CompileUsingDXC(
    const SDL_ShaderCross_HLSL_Info *info,
    bool spirv,
    size_t *size) // filled in with number of bytes of returned buffer
{
#ifdef SDL_SHADERCROSS_DXC
    DxcBuffer source;
    IDxcResult *dxcResult;
    IDxcBlob *blob;
    IDxcBlobUtf8 *errors;
    size_t entryPointLength = SDL_utf8strlen(info->entrypoint) + 1;
    wchar_t *entryPointUtf16 = NULL;
    size_t includeDirLength = 0;
    wchar_t *includeDirUtf16 = NULL;
    wchar_t *nameUtf16 = NULL;
    wchar_t **defineStringsUtf16 = NULL;
    size_t numDefineStrings = 0;
    HRESULT ret;

    /* Non-static DxcInstance, since the functions we call on it are not thread-safe */
    IDxcCompiler3 *dxcInstance = NULL;
    IDxcUtils *utils = NULL;
    IDxcIncludeHandler *includeHandler = NULL;

    DxcCreateInstance(
        &CLSID_DxcCompiler,
        IID_IDxcCompiler3,
        (void **)&dxcInstance);

    DxcCreateInstance(
        &CLSID_DxcUtils,
        &IID_IDxcUtils,
        (void **)(&utils));

    if (dxcInstance == NULL) {
        SDL_SetError("%s", "Could not create DXC instance!");
        return NULL;
    }

    if (utils == NULL) {
        SDL_SetError("%s", "Could not create DXC utils instance!");
        dxcInstance->lpVtbl->Release(dxcInstance);
        return NULL;
    }

    utils->lpVtbl->CreateDefaultIncludeHandler(utils, &includeHandler);
    if (includeHandler == NULL) {
        SDL_SetError("%s", "Failed to create a default include handler!");
        dxcInstance->lpVtbl->Release(dxcInstance);
        utils->lpVtbl->Release(utils);
        return NULL;
    }

    entryPointUtf16 = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", info->entrypoint, entryPointLength);
    if (entryPointUtf16 == NULL) {
        SDL_SetError("%s", "Failed to convert entrypoint to WCHAR_T!");
        dxcInstance->lpVtbl->Release(dxcInstance);
        utils->lpVtbl->Release(utils);
        return NULL;
    }

    if (info->defines != NULL) {
        for (Uint32 i = 0; i < MAX_DEFINES; i += 1) {
            if (info->defines[i].name == NULL) {
                break;
            }
            numDefineStrings += 1;
        }
    }

    char defineString[MAX_DEFINE_STRING_LENGTH];
    defineStringsUtf16 = SDL_malloc(sizeof(LPCWSTR) * numDefineStrings);
    for (Uint32 i = 0; i < numDefineStrings; i += 1) {
        if (info->defines[i].value == NULL) {
            SDL_snprintf(defineString, MAX_DEFINE_STRING_LENGTH, "-D%s=%s", info->defines[i].name, "1");
        } else {
            SDL_snprintf(defineString, MAX_DEFINE_STRING_LENGTH, "-D%s=%s", info->defines[i].name, info->defines[i].value);
        }

        defineStringsUtf16[i] = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", defineString, MAX_DEFINE_STRING_LENGTH);
    }

    LPCWSTR *args = SDL_malloc(sizeof(LPCWSTR) * (numDefineStrings + 11));
    Uint32 argCount = 0;

    for (Uint32 i = 0; i < numDefineStrings; i += 1) {
        args[argCount++] = defineStringsUtf16[i];
    }

    args[argCount++] = (LPCWSTR)L"-E";
    args[argCount++] = (LPCWSTR)entryPointUtf16;

    if (info->include_dir != NULL) {
        includeDirLength = SDL_utf8strlen(info->include_dir) + 1;
        includeDirUtf16 = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", info->include_dir, includeDirLength);

        if (includeDirUtf16 == NULL) {
            SDL_SetError("%s", "Failed to convert include dir to WCHAR_T!");
            dxcInstance->lpVtbl->Release(dxcInstance);
            utils->lpVtbl->Release(utils);
            SDL_free(entryPointUtf16);
            return NULL;
        }
        args[argCount++] = (LPCWSTR)L"-I";
        args[argCount++] = includeDirUtf16;
    }

    source.Ptr = info->source;
    source.Size = SDL_strlen(info->source) + 1;
    source.Encoding = DXC_CP_ACP;

    if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"vs_6_0";
    } else if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"ps_6_0";
    } else { // compute
        args[argCount++] = (LPCWSTR)L"-T";
        args[argCount++] = (LPCWSTR)L"cs_6_0";
    }

    if (spirv) {
        args[argCount++] = (LPCWSTR)L"-spirv";
        args[argCount++] = (LPCWSTR)L"-fspv-flatten-resource-arrays";
    }

    if (info->enable_debug) {
        if (spirv) {
            // https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#debugging
            args[argCount++] = (LPCWSTR)L"-fspv-debug=vulkan-with-source";
        } else {
            // https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SourceLevelDebuggingHLSL.rst#command-line-options
            args[argCount++] = (LPCWSTR)L"-Zi";
        }
    }

    if (info->name) {
        nameUtf16 = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", info->name, SDL_utf8strlen(info->name) + 1);
        if (nameUtf16 != NULL) {
            args[argCount++] = nameUtf16; // a bare string inserted into the arguments is treated as the source file name
        }
    }

#if defined(SDL_PLATFORM_XBOXONE) || defined(SDL_PLATFORM_XBOXSERIES)
    args[argCount++] = L"-D__XBOX_DISABLE_PRECOMPILE=1";
#endif

    ret = dxcInstance->lpVtbl->Compile(
        dxcInstance,
        &source,
        args,
        argCount,
        includeHandler,
        IID_IDxcResult,
        (void **)&dxcResult);

    SDL_free(entryPointUtf16);
    if (includeDirUtf16 != NULL) {
        SDL_free(includeDirUtf16);
    }
    if (nameUtf16 != NULL) {
        SDL_free(nameUtf16);
    }

    if (ret < 0) {
        SDL_SetError("IDxcShaderCompiler3::Compile failed: %X", ret);
        dxcInstance->lpVtbl->Release(dxcInstance);
        utils->lpVtbl->Release(utils);
        return NULL;
    } else if (dxcResult == NULL) {
        SDL_SetError("%s", "HLSL compilation failed with no IDxcResult");
        dxcInstance->lpVtbl->Release(dxcInstance);
        utils->lpVtbl->Release(utils);
        return NULL;
    }

    ret = dxcResult->lpVtbl->GetOutput(dxcResult,
                                       DXC_OUT_OBJECT,
                                       IID_IDxcBlob,
                                       (void **)&blob,
                                       NULL);

    HRESULT retStatus;
    if (ret < 0 || dxcResult->lpVtbl->GetStatus(dxcResult, &retStatus) < 0 || retStatus < 0 ) {
        // Compilation failed, display errors
        dxcResult->lpVtbl->GetOutput(
            dxcResult,
            DXC_OUT_ERRORS,
            IID_IDxcBlobUtf8,
            (void **)&errors,
            NULL);

        if (errors != NULL && errors->lpVtbl->GetBufferSize(errors) != 0) {
            SDL_SetError(
            "HLSL compilation failed: %s",
                (char *)errors->lpVtbl->GetBufferPointer(errors));
        } else {
            SDL_SetError("%s", "Compilation failed with unknown error");
        }

        // teardown
        dxcResult->lpVtbl->Release(dxcResult);
        dxcInstance->lpVtbl->Release(dxcInstance);
        utils->lpVtbl->Release(utils);
        return NULL;
    }

    // If compilation succeeded, but there are errors, those are warnings
    dxcResult->lpVtbl->GetOutput(
        dxcResult,
        DXC_OUT_ERRORS,
        IID_IDxcBlobUtf8,
        (void **)&errors,
        NULL);

    if (errors != NULL && errors->lpVtbl->GetBufferSize(errors) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "HLSL compiled with warnings: %s",
            (char *)errors->lpVtbl->GetBufferPointer(errors));
    }

    *size = blob->lpVtbl->GetBufferSize(blob);
    void *buffer = SDL_malloc(*size);
    SDL_memcpy(buffer, blob->lpVtbl->GetBufferPointer(blob), *size);

    blob->lpVtbl->Release(blob);
    dxcResult->lpVtbl->Release(dxcResult);
    dxcInstance->lpVtbl->Release(dxcInstance);
    utils->lpVtbl->Release(utils);

    for (Uint32 i = 0; i < numDefineStrings; i += 1) {
        SDL_free(defineStringsUtf16[i]);
    }
    SDL_free(args);

    return buffer;
#else
    SDL_SetError("%s", "Shadercross was not built with DXC support, cannot compile using DXC!");
    return NULL;
#endif /* SDL_SHADERCROSS_DXC */
}

void *SDL_ShaderCross_CompileDXILFromHLSL(
    const SDL_ShaderCross_HLSL_Info *info,
    size_t *size)
{
#if SDL_PLATFORM_GDK
    return SDL_ShaderCross_INTERNAL_CompileUsingDXC(info, false, size);
#else
    // Roundtrip to SPIR-V to support things like Structured Buffers.
    size_t spirvSize;
    void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
        info,
        &spirvSize);

    if (spirv == NULL) {
        return NULL;
    }

    SDL_ShaderCross_SPIRV_Info spirvInfo;
    spirvInfo.bytecode = spirv;
    spirvInfo.bytecode_size = spirvSize;
    spirvInfo.entrypoint = info->entrypoint;
    spirvInfo.shader_stage = info->shader_stage;
    spirvInfo.enable_debug = info->enable_debug;
    spirvInfo.name = info->name;
    spirvInfo.props = 0;

    void *translatedSource = SDL_ShaderCross_TranspileHLSLFromSPIRV(
        &spirvInfo);

    SDL_free(spirv);
    if (translatedSource == NULL) {
        return NULL;
    }

    SDL_ShaderCross_HLSL_Info translatedHlslInfo;
    SDL_memcpy(&translatedHlslInfo, info, sizeof(SDL_ShaderCross_HLSL_Info));
    translatedHlslInfo.source = translatedSource;

    return SDL_ShaderCross_INTERNAL_CompileUsingDXC(
        &translatedHlslInfo,
        false,
        size);
#endif
}

void *SDL_ShaderCross_CompileSPIRVFromHLSL(
    const SDL_ShaderCross_HLSL_Info *info,
    size_t *size)
{
    return SDL_ShaderCross_INTERNAL_CompileUsingDXC(
        info,
        true,
        size);
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
static SDL_SharedObject *d3dcompiler_dll = NULL;

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

// FIXME: includes and defines
static ID3DBlob *SDL_ShaderCross_INTERNAL_CompileDXBC(
    const char *hlslSource,
    const char *entrypoint,
    const char *shaderProfile,
    bool enableDebug)
{
    ID3DBlob *blob;
    ID3DBlob *errorBlob;
    HRESULT ret;

    if (SDL_D3DCompile == NULL) {
        SDL_SetError("%s", "Could not load D3DCompile!");
        return NULL;
    }

    ret = SDL_D3DCompile(
        hlslSource,
        SDL_strlen(hlslSource),
        NULL,
        NULL,
        NULL,
        entrypoint,
        shaderProfile,
        enableDebug ? 1 : 0, // D3DCOMPILE_DEBUG = 1
        0,
        &blob,
        &errorBlob);

    if (ret < 0) {
        if (errorBlob != NULL) {
            SDL_SetError(
                "HLSL compilation failed: %s",
                (char *)errorBlob->lpVtbl->GetBufferPointer(errorBlob));
        } else {
            SDL_SetError("HLSL compilation failed for an unknown reason.");
        }
        return NULL;
    }

    return blob;
}

void *SDL_ShaderCross_INTERNAL_CompileDXBCFromHLSL(
    const SDL_ShaderCross_HLSL_Info *info,
    bool enableRoundtrip,
    size_t *size) // filled in with number of bytes of returned buffer
{
    char *transpiledSource = NULL;

    if (enableRoundtrip) {
        // Need to roundtrip to SM 5.1
        size_t spirv_size;
        void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
            info,
            &spirv_size);

        if (spirv == NULL) {
            return NULL;
        }

        SDL_ShaderCross_SPIRV_Info spirvInfo;
        spirvInfo.bytecode = spirv;
        spirvInfo.bytecode_size = spirv_size;
        spirvInfo.entrypoint = info->entrypoint;
        spirvInfo.shader_stage = info->shader_stage;
        spirvInfo.enable_debug = info->enable_debug;
        spirvInfo.name = info->name;
        spirvInfo.props = 0;

        transpiledSource = SDL_ShaderCross_TranspileHLSLFromSPIRV(
            &spirvInfo);
        SDL_free(spirv);

        if (transpiledSource == NULL) {
            return NULL;
        }
    }

    const char *shaderProfile;
    if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
        shaderProfile = "vs_5_1";
    } else if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
        shaderProfile = "ps_5_1";
    } else { // compute
        shaderProfile = "cs_5_1";
    }

    ID3DBlob *blob = SDL_ShaderCross_INTERNAL_CompileDXBC(
        transpiledSource != NULL ? transpiledSource : info->source,
        info->entrypoint,
        shaderProfile,
        info->enable_debug);

    if (blob == NULL) {
        *size = 0;
        return NULL;
    }

    *size = blob->lpVtbl->GetBufferSize(blob);
    void *buffer = SDL_malloc(*size);
    SDL_memcpy(buffer, blob->lpVtbl->GetBufferPointer(blob), *size);
    blob->lpVtbl->Release(blob);

    if (transpiledSource != NULL) {
        SDL_free(transpiledSource);
    }

    return buffer;
}

// Returns raw byte buffer
void *SDL_ShaderCross_CompileDXBCFromHLSL(
    const SDL_ShaderCross_HLSL_Info *info,
    size_t *size) // filled in with number of bytes of returned buffer
{
    return SDL_ShaderCross_INTERNAL_CompileDXBCFromHLSL(
        info,
        true,
        size);
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_HLSL_Info *info,
    SDL_ShaderCross_GraphicsShaderMetadata *metadata)
{
    size_t bytecodeSize;

    // We'll go through SPIRV-Cross for all of these to more easily obtain reflection metadata.
    void *spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(
        info,
        &bytecodeSize);

    if (spirv == NULL) {
        // Error output from DXC will have already been set
        return NULL;
    }

    SDL_ShaderCross_SPIRV_Info spirvInfo;
    spirvInfo.bytecode = spirv;
    spirvInfo.bytecode_size = bytecodeSize;
    spirvInfo.entrypoint = info->entrypoint;
    spirvInfo.shader_stage = info->shader_stage;
    spirvInfo.enable_debug = info->enable_debug;
    spirvInfo.name = info->name;
    spirvInfo.props = 0;

    void *result;
    if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
        result = SDL_ShaderCross_CompileComputePipelineFromSPIRV(
            device,
            &spirvInfo,
            (void *)metadata);
    } else {
        result = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            device,
            &spirvInfo,
            (void *)metadata);
    }
    SDL_free(spirv);
    return result;
}

SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromHLSL(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_HLSL_Info *info,
    SDL_ShaderCross_GraphicsShaderMetadata *metadata)
{
    return (SDL_GPUShader *)SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(
        device,
        info,
        (void *)metadata);
}

SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromHLSL(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_HLSL_Info *info,
    SDL_ShaderCross_ComputePipelineMetadata *metadata)
{
    return (SDL_GPUComputePipeline *)SDL_ShaderCross_INTERNAL_CreateShaderFromHLSL(
        device,
        info,
        (void *)metadata);
}

#include <spirv_cross_c.h>

#define SPVC_ERROR(func) \
    SDL_SetError(#func " failed: %s", spvc_context_get_last_error_string(context))

static int parse_version_number(const char* str)
{
    unsigned major, minor, patch;
    if (SDL_sscanf(str, "%u.%u.%u", &major, &minor, &patch) == 3) {
        return (major * 10000) + (minor) * 100 + patch;
    }
    return -1;
}

typedef struct SPIRVTranspileContext {
    spvc_context context;
    const char *translated_source;
    const char *cleansed_entrypoint;
} SPIRVTranspileContext;

static void SDL_ShaderCross_INTERNAL_DestroyTranspileContext(
    SPIRVTranspileContext *context)
{
    spvc_context_destroy(context->context);
    SDL_free(context);
}

static SPIRVTranspileContext *SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
    spvc_backend backend,
    unsigned shadermodel, // only used for HLSL
    SDL_ShaderCross_ShaderStage shaderStage, // only used for MSL
    const Uint8 *code,
    size_t codeSize,
    const char *entrypoint,
    SDL_PropertiesID props
) {
    spvc_result result;
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_compiler_options options = NULL;
    SPIRVTranspileContext *transpileContext = NULL;
    const char *translated_source;
    const char *cleansed_entrypoint;

    /* Create the SPIRV-Cross context */
    result = spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return NULL;
    }

    /* Parse the SPIR-V into IR */
    result = spvc_context_parse_spirv(context, (const SpvId *)code, codeSize / sizeof(SpvId), &ir);
    if (result < 0) {
        SPVC_ERROR(spvc_context_parse_spirv);
        spvc_context_destroy(context);
        return NULL;
    }

    /* Create the cross-compiler */
    result = spvc_context_create_compiler(context, backend, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
    if (result < 0) {
        SPVC_ERROR(spvc_context_create_compiler);
        spvc_context_destroy(context);
        return NULL;
    }

    /* Set up the cross-compiler options */
    result = spvc_compiler_create_compiler_options(compiler, &options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_create_compiler_options);
        spvc_context_destroy(context);
        return NULL;
    }

    if (backend == SPVC_BACKEND_HLSL) {
        spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL, shadermodel);
        spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_NONWRITABLE_UAV_TEXTURE_AS_SRV, 1);
        spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_FLATTEN_MATRIX_VERTEX_INPUT_SEMANTICS, 1);
        spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_HLSL_USE_ENTRY_POINT_NAME, !SDL_GetBooleanProperty(props, SDL_SHADERCROSS_PROP_SPIRV_PSSL_COMPATIBILITY, false));
        spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_HLSL_POINT_SIZE_COMPAT, true);
    }

    SpvExecutionModel executionModel;
    if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_VERTEX) {
        executionModel = SpvExecutionModelVertex;
    } else if (shaderStage == SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT) {
        executionModel = SpvExecutionModelFragment;
    } else { // compute
        if (backend == SPVC_BACKEND_HLSL) {
            executionModel = SpvExecutionModelKernel;
        } else {
            executionModel = SpvExecutionModelGLCompute;
        }
    }

    if (backend == SPVC_BACKEND_MSL) {
        const char *_mslVersion = SDL_GetStringProperty(props, SDL_SHADERCROSS_PROP_SPIRV_MSL_VERSION, "1.2.0");
        int mslVersion = parse_version_number(_mslVersion);
        if (mslVersion == - 1) {
            SDL_SetError("failed to parse MSL version string \"%s\"", _mslVersion);
            spvc_context_destroy(context);
            return NULL;
        }
        spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION, mslVersion);
    }

    // MSL doesn't have descriptor sets, so we have to set up index remapping
    if (backend == SPVC_BACKEND_MSL && shaderStage != SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
        spvc_resources resources;
        spvc_reflected_resource *reflected_resources;
        size_t num_texture_samplers;
        size_t num_storage_textures;
        size_t num_storage_buffers;
        size_t num_uniform_buffers;
        size_t num_separate_samplers = 0;
        size_t num_separate_images = 0;

        spvc_msl_resource_binding_2 bufferBindings[32];
        SDL_zeroa(bufferBindings);
        Uint32 numBufferBindings = 0;

        spvc_msl_resource_binding_2 textureBindings[32];
        SDL_zeroa(textureBindings);
        Uint32 numTextureBindings = 0;

        result = spvc_compiler_create_shader_resources(compiler, &resources);
        if (result < 0) {
            SPVC_ERROR(spvc_compiler_create_shader_resources);
            spvc_context_destroy(context);
            return NULL;
        }

        // Combined texture-samplers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_texture_samplers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // If source is HLSL, we might have separate images and samplers
        if (num_texture_samplers == 0) {
            result = spvc_resources_get_resource_list_for_type(
                resources,
                SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
                (const spvc_reflected_resource **)&reflected_resources,
                &num_separate_samplers);
            if (result < 0) {
                SPVC_ERROR(spvc_resources_get_resource_list_for_type);
                spvc_context_destroy(context);
                return NULL;
            }
            num_texture_samplers = num_separate_samplers;
        }

        for (size_t i = 0; i < num_texture_samplers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 2)) {
                SDL_SetError("%s", "Descriptor set index for graphics texture-sampler must be 0 or 2!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // Storage textures
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_storage_textures);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_storage_textures; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 2)) {
                SDL_SetError("%s", "Descriptor set index for graphics storage texture must be 0 or 2!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // If source is HLSL, storage images might be marked as separate images
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_separate_images);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // We only want to iterate the images that don't have an associated sampler
        for (size_t i = num_separate_samplers; i < num_separate_images; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 2)) {
                SDL_SetError("%s", "Descriptor set index for graphics storage texture must be 0 or 2!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // Uniform buffers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_uniform_buffers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_uniform_buffers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 1 || descriptor_set_index == 3)) {
                SDL_SetError("%s", "Descriptor set index for graphics uniform buffer must be 1 or 3!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            bufferBindings[numBufferBindings].stage = executionModel;
            bufferBindings[numBufferBindings].desc_set = descriptor_set_index;
            bufferBindings[numBufferBindings].binding = binding_index;
            bufferBindings[numBufferBindings].count = 1;
            // assign binding index after we have collected all resources

            numBufferBindings += 1;
        }

        // Storage buffers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_storage_buffers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_storage_buffers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 2)) {
                SDL_SetError("%s", "Descriptor set index for graphics storage buffer must be 0 or 2!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            bufferBindings[numBufferBindings].stage = executionModel;
            bufferBindings[numBufferBindings].desc_set = descriptor_set_index;
            bufferBindings[numBufferBindings].binding = binding_index;
            bufferBindings[numBufferBindings].count = 1;
            // assign binding index after we have collected all resources

            numBufferBindings += 1;
        }

        // Textures come first so we can just use the binding slot
        for (Uint32 i = 0; i < numTextureBindings; i += 1) {
            textureBindings[i].msl_texture = textureBindings[i].binding;
            textureBindings[i].msl_sampler = textureBindings[i].binding;
            result = spvc_compiler_msl_add_resource_binding_2(compiler, &textureBindings[i]);
        }

        if (result < 0) {
            SPVC_ERROR(spvc_compiler_msl_add_resource_binding_2);
            spvc_context_destroy(context);
            return NULL;
        }

        // Calculate number of uniform buffers
        Uint32 uniformBufferCount = 0;

        for (Uint32 i = 0; i < numBufferBindings; i += 1) {
            if (bufferBindings[i].desc_set == 1 || bufferBindings[i].desc_set == 3) {
                uniformBufferCount += 1;
            }
        }

        // Calculate resource indices
        for (Uint32 i = 0; i < numBufferBindings; i += 1) {
            if (bufferBindings[i].desc_set == 1 || bufferBindings[i].desc_set == 3) {
                // Uniform buffers are alone in the descriptor set
                bufferBindings[i].msl_buffer = bufferBindings[i].binding;
            } else {
                // Subtract by the texture count because the textures precede the storage buffers in the descriptor set
                bufferBindings[i].msl_buffer = uniformBufferCount + (bufferBindings[i].binding - numTextureBindings);
            }

            result = spvc_compiler_msl_add_resource_binding_2(compiler, &bufferBindings[i]);

            if (result < 0) {
                SPVC_ERROR(spvc_compiler_msl_add_resource_binding_2);
                spvc_context_destroy(context);
                return NULL;
            }
        }
    }

    if (backend == SPVC_BACKEND_MSL && shaderStage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
        spvc_resources resources;
        spvc_reflected_resource *reflected_resources;
        size_t num_texture_samplers;
        size_t num_storage_textures; // total storage textures
        size_t num_storage_buffers; // total storage buffers
        size_t num_uniform_buffers;
        size_t num_separate_samplers = 0;
        size_t num_separate_images = 0;

        spvc_msl_resource_binding_2 bufferBindings[32];
        Uint32 numBufferBindings = 0;

        spvc_msl_resource_binding_2 textureBindings[32];
        Uint32 numTextureBindings = 0;

        result = spvc_compiler_create_shader_resources(compiler, &resources);
        if (result < 0) {
            SPVC_ERROR(spvc_compiler_create_shader_resources);
            spvc_context_destroy(context);
            return NULL;
        }

        // Combined texture-samplers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_texture_samplers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // If source is HLSL, we might have separate images and samplers
        if (num_texture_samplers == 0) {
            result = spvc_resources_get_resource_list_for_type(
                resources,
                SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
                (const spvc_reflected_resource **)&reflected_resources,
                &num_separate_samplers);
            if (result < 0) {
                SPVC_ERROR(spvc_resources_get_resource_list_for_type);
                spvc_context_destroy(context);
                return NULL;
            }
            num_texture_samplers = num_separate_samplers;
        }

        for (size_t i = 0; i < num_texture_samplers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (descriptor_set_index != 0) {
                SDL_SetError("%s", "Descriptor set index for compute texture-sampler must be 0!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // Readonly storage textures
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_storage_textures);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_storage_textures; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 1)) {
                SDL_SetError("%s", "Descriptor set index for compute storage texture must be 0 or 1!");
                spvc_context_destroy(context);
                return NULL;
            }

            // Skip readwrite textures
            if (descriptor_set_index != 0) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // If source is HLSL, storage images might be marked as separate images
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_separate_images);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // We only want to iterate the images that don't have an associated sampler
        for (size_t i = num_separate_samplers; i < num_separate_images; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0 || descriptor_set_index == 1)) {
                SDL_SetError("%s", "Descriptor set index for compute storage texture must be 0 or 1!");
                spvc_context_destroy(context);
                return NULL;
            }

            // Skip readwrite textures
            if (descriptor_set_index != 0) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // Readwrite storage textures
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_storage_textures);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_storage_textures; i += 1) {
            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);

            // Skip readonly textures
            if (descriptor_set_index != 1) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // If source is HLSL, storage images might be marked as separate images
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_separate_images);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // We only want to iterate the images that don't have an associated sampler
        for (size_t i = num_separate_samplers; i < num_separate_images; i += 1) {
            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);

            // Skip readonly textures
            if (descriptor_set_index != 1) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            textureBindings[numTextureBindings].stage = executionModel;
            textureBindings[numTextureBindings].desc_set = descriptor_set_index;
            textureBindings[numTextureBindings].binding = binding_index;
            textureBindings[numTextureBindings].count = 1;
            // assign binding index after we have collected all resources

            numTextureBindings += 1;
        }

        // Uniform buffers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_uniform_buffers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        for (size_t i = 0; i < num_uniform_buffers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (descriptor_set_index != 2) {
                SDL_SetError("%s", "Descriptor set index for compute uniform buffer must be 2!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            bufferBindings[numBufferBindings].stage = executionModel;
            bufferBindings[numBufferBindings].desc_set = descriptor_set_index;
            bufferBindings[numBufferBindings].binding = binding_index;
            bufferBindings[numBufferBindings].count = 1;
            // assign binding index after we have collected all resources

            numBufferBindings += 1;
        }

        // Storage buffers
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_storage_buffers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return NULL;
        }

        // Readonly storage buffers
        for (size_t i = 0; i < num_storage_buffers; i += 1) {
            if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
                SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
                spvc_context_destroy(context);
                return NULL;
            }

            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
            if (!(descriptor_set_index == 0|| descriptor_set_index == 1)) {
                SDL_SetError("%s", "Descriptor set index for compute storage buffer must be 0 or 1!");
                spvc_context_destroy(context);
                return NULL;
            }

            // Skip readwrite buffers
            if (descriptor_set_index != 0) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            bufferBindings[numBufferBindings].stage = executionModel;
            bufferBindings[numBufferBindings].desc_set = descriptor_set_index;
            bufferBindings[numBufferBindings].binding = binding_index;
            bufferBindings[numBufferBindings].count = 1;
            // assign binding index after we have collected all resources

            numBufferBindings += 1;
        }

        // Readwrite storage buffers
        for (size_t i = 0; i < num_storage_buffers; i += 1) {
            unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);

            // Skip readonly buffers
            if (descriptor_set_index != 1) { continue; }

            unsigned int binding_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding);

            bufferBindings[numBufferBindings].stage = executionModel;
            bufferBindings[numBufferBindings].desc_set = descriptor_set_index;
            bufferBindings[numBufferBindings].binding = binding_index;
            bufferBindings[numBufferBindings].count = 1;
            // assign binding index after we have collected all resources

            numBufferBindings += 1;
        }

        // Calculate binding offsets

        Uint32 readonlyTextureCount = 0;
        Uint32 readwriteTextureCount = 0;

        for (Uint32 i = 0; i < numTextureBindings; i += 1) {
            if (textureBindings[i].desc_set == 0) {
                readonlyTextureCount += 1;
            } else if (textureBindings[i].desc_set == 1) {
                readwriteTextureCount += 1;
            }
        }

        Uint32 uniformBufferCount = 0;
        Uint32 readonlyBufferCount = 0;

        for (Uint32 i = 0; i < numBufferBindings; i += 1) {
            if (bufferBindings[i].desc_set == 0) {
                readonlyBufferCount += 1;
            } else if (bufferBindings[i].desc_set == 2) {
                uniformBufferCount += 1;
            }
        }

        // Calculate resource indices

        for (Uint32 i = 0; i < numTextureBindings; i += 1) {
            if (textureBindings[i].desc_set == 0) {
                // readonly textures
                textureBindings[i].msl_texture = textureBindings[i].binding;
                textureBindings[i].msl_sampler = textureBindings[i].binding;
            } else {
                // readwrite textures
                textureBindings[i].msl_texture = readonlyTextureCount + textureBindings[i].binding;
                textureBindings[i].msl_sampler = readonlyTextureCount + textureBindings[i].binding;
            }
            result = spvc_compiler_msl_add_resource_binding_2(compiler, &textureBindings[i]);
            if (result < 0) {
                SPVC_ERROR(spvc_compiler_msl_add_resource_binding_2);
                spvc_context_destroy(context);
                return NULL;
            }
        }

        for (Uint32 i = 0; i < numBufferBindings; i += 1) {
            if (bufferBindings[i].desc_set == 0) {
                // Subtract by the readonly texture count because they precede readonly buffers in the descriptor set
                bufferBindings[i].msl_buffer = uniformBufferCount + (bufferBindings[i].binding - readonlyTextureCount);
            } else if (bufferBindings[i].desc_set == 1) {
                // Subtract by the readwrite texture count because they precede readwrite buffers in the descriptor set
                bufferBindings[i].msl_buffer = uniformBufferCount + readonlyBufferCount + (bufferBindings[i].binding - readwriteTextureCount);
            } else {
                // Uniform buffers are alone in the descriptor set
                bufferBindings[i].msl_buffer = bufferBindings[i].binding;
            }
            result = spvc_compiler_msl_add_resource_binding_2(compiler, &bufferBindings[i]);

            if (result < 0) {
                SPVC_ERROR(spvc_compiler_msl_add_resource_binding_2);
                spvc_context_destroy(context);
                return NULL;
            }
        }
    }

    result = spvc_compiler_install_compiler_options(compiler, options);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_install_compiler_options);
        spvc_context_destroy(context);
        return NULL;
    }

    /* Compile to the target shader language */
    result = spvc_compiler_compile(compiler, &translated_source);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_compile);
        spvc_context_destroy(context);
        return NULL;
    }

    if (backend == SPVC_BACKEND_MSL) {
        // Metal doesn't allow a "main" entrypoint, so determine the "cleansed" entrypoint name (e.g. main -> main0 on MSL)
        cleansed_entrypoint = spvc_compiler_get_cleansed_entry_point_name(
            compiler,
            entrypoint,
            spvc_compiler_get_execution_model(compiler));
    } else {
        cleansed_entrypoint = entrypoint;
    }

    transpileContext = SDL_malloc(sizeof(SPIRVTranspileContext));
    transpileContext->context = context;
    transpileContext->cleansed_entrypoint = cleansed_entrypoint;
    transpileContext->translated_source = translated_source;
    return transpileContext;
}

// Acquire metadata from SPIRV bytecode.
// TODO: validate descriptor sets
bool SDL_ShaderCross_ReflectGraphicsSPIRV(
    const Uint8 *code,
    size_t codeSize,
    SDL_ShaderCross_GraphicsShaderMetadata *metadata // filled in with reflected data
) {
    spvc_result result;
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    size_t num_texture_samplers = 0;
    size_t num_storage_textures = 0;
    size_t num_storage_buffers = 0;
    size_t num_uniform_buffers = 0;
    size_t num_separate_samplers = 0; // HLSL edge case
    size_t num_separate_images = 0; // HLSL edge case

    /* Create the SPIRV-Cross context */
    result = spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return false;
    }

    /* Parse the SPIR-V into IR */
    result = spvc_context_parse_spirv(context, (const SpvId *)code, codeSize / sizeof(SpvId), &ir);
    if (result < 0) {
        SPVC_ERROR(spvc_context_parse_spirv);
        spvc_context_destroy(context);
        return false;
    }

    /* Create a reflection-only compiler */
    result = spvc_context_create_compiler(context, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
    if (result < 0) {
        SPVC_ERROR(spvc_context_create_compiler);
        spvc_context_destroy(context);
        return false;
    }

    spvc_resources resources;
    spvc_reflected_resource *reflected_resources;

    result = spvc_compiler_create_shader_resources(compiler, &resources);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_create_shader_resources);
        spvc_context_destroy(context);
        return false;
    }

    // Combined texture-samplers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_texture_samplers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // If source is HLSL, we might have separate images and samplers
    if (num_texture_samplers == 0) {
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_separate_samplers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return false;
        }
        num_texture_samplers = num_separate_samplers;
    }

    // Storage textures
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_storage_textures);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // If source is HLSL, storage images might be marked as separate images
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_separate_images);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }
    // The number of storage textures is the number of separate images minus the number of samplers.
    num_storage_textures += (num_separate_images - num_separate_samplers);

    // Storage buffers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_storage_buffers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // Uniform buffers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_uniform_buffers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    spvc_context_destroy(context);

    metadata->num_samplers = (Uint32)num_texture_samplers;
    metadata->num_storage_textures = (Uint32)num_storage_textures;
    metadata->num_storage_buffers = (Uint32)num_storage_buffers;
    metadata->num_uniform_buffers = (Uint32)num_uniform_buffers;
    return true;
}

bool SDL_ShaderCross_ReflectComputeSPIRV(
    const Uint8 *bytecode,
    size_t bytecodeSize,
    SDL_ShaderCross_ComputePipelineMetadata *metadata // filled in with reflected data
) {
    spvc_result result;
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler = NULL;
    size_t num_texture_samplers = 0;
    size_t num_readonly_storage_textures = 0;
    size_t num_readonly_storage_buffers = 0;
    size_t num_readwrite_storage_textures = 0;
    size_t num_readwrite_storage_buffers = 0;
    size_t num_uniform_buffers = 0;

    size_t num_storage_textures = 0;
    size_t num_storage_buffers = 0;
    size_t num_separate_samplers = 0; // HLSL edge case
    size_t num_separate_images = 0; // HLSL edge case

    /* Create the SPIRV-Cross context */
    result = spvc_context_create(&context);
    if (result < 0) {
        SDL_SetError("spvc_context_create failed: %X", result);
        return false;
    }

    /* Parse the SPIR-V into IR */
    result = spvc_context_parse_spirv(context, (const SpvId *)bytecode, bytecodeSize / sizeof(SpvId), &ir);
    if (result < 0) {
        SPVC_ERROR(spvc_context_parse_spirv);
        spvc_context_destroy(context);
        return false;
    }

    /* Create a reflection-only compiler */
    result = spvc_context_create_compiler(context, SPVC_BACKEND_NONE, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler);
    if (result < 0) {
        SPVC_ERROR(spvc_context_create_compiler);
        spvc_context_destroy(context);
        return false;
    }

    spvc_resources resources;
    spvc_reflected_resource *reflected_resources;

    result = spvc_compiler_create_shader_resources(compiler, &resources);
    if (result < 0) {
        SPVC_ERROR(spvc_compiler_create_shader_resources);
        spvc_context_destroy(context);
        return false;
    }

    // Combined texture-samplers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_texture_samplers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // If source is HLSL, we might have separate images and samplers
    if (num_texture_samplers == 0) {
        result = spvc_resources_get_resource_list_for_type(
            resources,
            SPVC_RESOURCE_TYPE_SEPARATE_SAMPLERS,
            (const spvc_reflected_resource **)&reflected_resources,
            &num_separate_samplers);
        if (result < 0) {
            SPVC_ERROR(spvc_resources_get_resource_list_for_type);
            spvc_context_destroy(context);
            return false;
        }
        num_texture_samplers = num_separate_samplers;
    }

    // Storage textures
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_storage_textures);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    for (size_t i = 0; i < num_storage_textures; i += 1) {
        if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
            SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
            spvc_context_destroy(context);
            return false;
        }

        unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);

        if (descriptor_set_index == 0) {
            num_readonly_storage_textures += 1;
        } else if (descriptor_set_index == 1) {
            num_readwrite_storage_textures += 1;
        } else {
            SDL_SetError("%s", "Descriptor set index for compute storage texture must be 0 or 1!");
            spvc_context_destroy(context);
            return false;
        }
    }

    // If source is HLSL, readonly storage images might be marked as separate images
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_SEPARATE_IMAGE,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_separate_images);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // The number of storage textures is the number of separate images minus the number of samplers.
    num_storage_textures += (num_separate_images - num_separate_samplers);

    for (size_t i = num_separate_samplers; i < num_separate_images; i += 1) {
        if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
            SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
            spvc_context_destroy(context);
            return false;
        }

        unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);

        if (descriptor_set_index == 0) {
            num_readonly_storage_textures += 1;
        } else if (descriptor_set_index == 1) {
            num_readwrite_storage_textures += 1;
        } else {
            SDL_SetError("%s", "Descriptor set index for compute storage texture must be 0 or 1!");
            spvc_context_destroy(context);
            return false;
        }
    }

    // Storage buffers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_storage_buffers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // Readonly storage buffers
    for (size_t i = 0; i < num_storage_buffers; i += 1) {
        if (!spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet) || !spvc_compiler_has_decoration(compiler, reflected_resources[i].id, SpvDecorationBinding)) {
            SDL_SetError("%s", "Shader resources must have descriptor set and binding index!");
            spvc_context_destroy(context);
            return false;
        }

        unsigned int descriptor_set_index = spvc_compiler_get_decoration(compiler, reflected_resources[i].id, SpvDecorationDescriptorSet);
        if (!(descriptor_set_index == 0 || descriptor_set_index == 1)) {
            SDL_SetError("%s", "Descriptor set index for compute storage buffer must be 0 or 1!");
            spvc_context_destroy(context);
            return false;
        }

        if (descriptor_set_index == 0) {
            num_readonly_storage_buffers += 1;
        } else if (descriptor_set_index == 1) {
            num_readwrite_storage_buffers += 1;
        } else {
            SDL_SetError("%s", "Descriptor set index for compute storage buffer must be 0 or 1!");
            spvc_context_destroy(context);
            return false;
        }
    }

    // Uniform buffers
    result = spvc_resources_get_resource_list_for_type(
        resources,
        SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
        (const spvc_reflected_resource **)&reflected_resources,
        &num_uniform_buffers);
    if (result < 0) {
        SPVC_ERROR(spvc_resources_get_resource_list_for_type);
        spvc_context_destroy(context);
        return false;
    }

    // Threadcount
    metadata->threadcount_x = spvc_compiler_get_execution_mode_argument_by_index(compiler, SpvExecutionModeLocalSize, 0);
    metadata->threadcount_y = spvc_compiler_get_execution_mode_argument_by_index(compiler, SpvExecutionModeLocalSize, 1);
    metadata->threadcount_z = spvc_compiler_get_execution_mode_argument_by_index(compiler, SpvExecutionModeLocalSize, 2);

    spvc_context_destroy(context);

    metadata->num_samplers = (Uint32)num_texture_samplers;
    metadata->num_readonly_storage_textures = (Uint32)num_readonly_storage_textures;
    metadata->num_readonly_storage_buffers = (Uint32)num_readonly_storage_buffers;
    metadata->num_readwrite_storage_textures = (Uint32)num_readwrite_storage_textures;
    metadata->num_readwrite_storage_buffers = (Uint32)num_readwrite_storage_buffers;
    metadata->num_uniform_buffers = (Uint32)num_uniform_buffers;
    return true;
}

static void *SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_SPIRV_Info *info,
    SDL_GPUShaderFormat targetFormat,
    void *metadata
) {
    spvc_backend backend;
    unsigned shadermodel = 0;

    if (targetFormat == SDL_GPU_SHADERFORMAT_DXBC) {
        backend = SPVC_BACKEND_HLSL;
        shadermodel = 51;
    } else if (targetFormat == SDL_GPU_SHADERFORMAT_DXIL) {
        backend = SPVC_BACKEND_HLSL;
        shadermodel = 60;
    } else if (targetFormat == SDL_GPU_SHADERFORMAT_MSL) {
        backend = SPVC_BACKEND_MSL;
    } else {
        SDL_SetError("SDL_ShaderCross_INTERNAL_CompileFromSPIRV: Unexpected SDL_GPUBackend");
        return NULL;
    }

    SPIRVTranspileContext *transpileContext = SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
        backend,
        shadermodel,
        info->shader_stage,
        info->bytecode,
        info->bytecode_size,
        info->entrypoint,
        info->props);

    if (transpileContext == NULL) {
        return NULL;
    }

    void *shaderObject = NULL;

    if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
        SDL_GPUComputePipelineCreateInfo createInfo;
        SDL_ShaderCross_ComputePipelineMetadata *pipelineInfo = (SDL_ShaderCross_ComputePipelineMetadata *)metadata;
        SDL_ShaderCross_ReflectComputeSPIRV(
            info->bytecode,
            info->bytecode_size,
            pipelineInfo);
        createInfo.entrypoint = transpileContext->cleansed_entrypoint;
        createInfo.format = targetFormat;
        createInfo.num_samplers = pipelineInfo->num_samplers;
        createInfo.num_readonly_storage_textures = pipelineInfo->num_readonly_storage_textures;
        createInfo.num_readonly_storage_buffers = pipelineInfo->num_readonly_storage_buffers;
        createInfo.num_readwrite_storage_textures = pipelineInfo->num_readwrite_storage_textures;
        createInfo.num_readwrite_storage_buffers = pipelineInfo->num_readwrite_storage_buffers;
        createInfo.num_uniform_buffers = pipelineInfo->num_uniform_buffers;
        createInfo.threadcount_x = pipelineInfo->threadcount_x;
        createInfo.threadcount_y = pipelineInfo->threadcount_y;
        createInfo.threadcount_z = pipelineInfo->threadcount_z;

        createInfo.props = 0;
        if (info->name != NULL) {
            createInfo.props = SDL_CreateProperties();
            SDL_SetStringProperty(createInfo.props, SDL_PROP_GPU_COMPUTEPIPELINE_CREATE_NAME_STRING, info->name);
        }

        SDL_ShaderCross_HLSL_Info hlslInfo;
        hlslInfo.source = transpileContext->translated_source;
        hlslInfo.entrypoint = transpileContext->cleansed_entrypoint;
        hlslInfo.include_dir = NULL;
        hlslInfo.defines = NULL;
        hlslInfo.enable_debug = info->enable_debug;
        hlslInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
        hlslInfo.name = info->name;
        hlslInfo.props = 0;

        if (targetFormat == SDL_GPU_SHADERFORMAT_DXBC) {
            createInfo.code = SDL_ShaderCross_INTERNAL_CompileDXBCFromHLSL(
                &hlslInfo,
                false,
                &createInfo.code_size);
        } else if (targetFormat == SDL_GPU_SHADERFORMAT_DXIL) {
            createInfo.code = SDL_ShaderCross_CompileDXILFromHLSL(
                &hlslInfo,
                &createInfo.code_size);
        } else { // MSL
            createInfo.code = (const Uint8 *)transpileContext->translated_source;
            createInfo.code_size = SDL_strlen(transpileContext->translated_source) + 1;
        }

        shaderObject = SDL_CreateGPUComputePipeline(device, &createInfo);

        if (createInfo.props != 0) {
            SDL_DestroyProperties(createInfo.props);
        }
    } else {
        SDL_GPUShaderCreateInfo createInfo;
        SDL_ShaderCross_GraphicsShaderMetadata *shaderInfo = (SDL_ShaderCross_GraphicsShaderMetadata *)metadata;
        SDL_ShaderCross_ReflectGraphicsSPIRV(
            info->bytecode,
            info->bytecode_size,
            shaderInfo);
        createInfo.entrypoint = transpileContext->cleansed_entrypoint;
        createInfo.format = targetFormat;
        createInfo.stage = (SDL_GPUShaderStage)info->shader_stage;
        createInfo.num_samplers = shaderInfo->num_samplers;
        createInfo.num_storage_textures = shaderInfo->num_storage_textures;
        createInfo.num_storage_buffers = shaderInfo->num_storage_buffers;
        createInfo.num_uniform_buffers = shaderInfo->num_uniform_buffers;

        createInfo.props = 0;
        if (info->name != NULL) {
            createInfo.props = SDL_CreateProperties();
            SDL_SetStringProperty(createInfo.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, info->name);
        }

        SDL_ShaderCross_HLSL_Info hlslInfo;
        hlslInfo.source = transpileContext->translated_source;
        hlslInfo.entrypoint = transpileContext->cleansed_entrypoint;
        hlslInfo.include_dir = NULL;
        hlslInfo.defines = NULL;
        hlslInfo.enable_debug = info->enable_debug;
        hlslInfo.shader_stage = info->shader_stage;
        hlslInfo.name = info->name;
        hlslInfo.props = 0;

        if (targetFormat == SDL_GPU_SHADERFORMAT_DXBC) {
            createInfo.code = SDL_ShaderCross_INTERNAL_CompileDXBCFromHLSL(
                &hlslInfo,
                false,
                &createInfo.code_size);
        } else if (targetFormat == SDL_GPU_SHADERFORMAT_DXIL) {
            createInfo.code = SDL_ShaderCross_CompileDXILFromHLSL(
                &hlslInfo,
                &createInfo.code_size);
        } else { // MSL
            createInfo.code = (const Uint8 *)transpileContext->translated_source;
            createInfo.code_size = SDL_strlen(transpileContext->translated_source) + 1;
        }

        shaderObject = SDL_CreateGPUShader(device, &createInfo);

        if (createInfo.props != 0) {
            SDL_DestroyProperties(createInfo.props);
        }
    }

    SDL_ShaderCross_INTERNAL_DestroyTranspileContext(transpileContext);
    return shaderObject;
}

void *SDL_ShaderCross_TranspileMSLFromSPIRV(
    const SDL_ShaderCross_SPIRV_Info *info)
{
    SPIRVTranspileContext *context = SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
        SPVC_BACKEND_MSL,
        0,
        info->shader_stage,
        info->bytecode,
        info->bytecode_size,
        info->entrypoint,
        info->props
    );

    if (context == NULL) {
        return NULL;
    }

    size_t length = SDL_strlen(context->translated_source) + 1;
    char *result = SDL_malloc(length);
    SDL_strlcpy(result, context->translated_source, length);

    SDL_ShaderCross_INTERNAL_DestroyTranspileContext(context);
    return result;
}

void *SDL_ShaderCross_TranspileHLSLFromSPIRV(
    const SDL_ShaderCross_SPIRV_Info *info)
{
    SPIRVTranspileContext *context = SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
        SPVC_BACKEND_HLSL,
        SDL_GetBooleanProperty(info->props, SDL_SHADERCROSS_PROP_SPIRV_PSSL_COMPATIBILITY, false) ? 50 : 60,
        info->shader_stage,
        info->bytecode,
        info->bytecode_size,
        info->entrypoint,
        info->props
    );

    if (context == NULL) {
        return NULL;
    }

    size_t length = SDL_strlen(context->translated_source) + 1;
    char *result = SDL_malloc(length);
    SDL_strlcpy(result, context->translated_source, length);

    SDL_ShaderCross_INTERNAL_DestroyTranspileContext(context);
    return result;
}

void *SDL_ShaderCross_CompileDXBCFromSPIRV(
    const SDL_ShaderCross_SPIRV_Info *info,
    size_t *size)
{
    SPIRVTranspileContext *context = SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
        SPVC_BACKEND_HLSL,
        51,
        info->shader_stage,
        info->bytecode,
        info->bytecode_size,
        info->entrypoint,
        info->props);

    if (context == NULL) {
        return NULL;
    }

    SDL_ShaderCross_HLSL_Info hlslInfo;
    hlslInfo.source = context->translated_source;
    hlslInfo.entrypoint = context->cleansed_entrypoint;
    hlslInfo.include_dir = NULL;
    hlslInfo.defines = NULL;
    hlslInfo.shader_stage = info->shader_stage;
    hlslInfo.enable_debug = info->enable_debug;
    hlslInfo.name = info->name;
    hlslInfo.props = 0;

    void *result = SDL_ShaderCross_INTERNAL_CompileDXBCFromHLSL(
        &hlslInfo,
        false,
        size);

    SDL_ShaderCross_INTERNAL_DestroyTranspileContext(context);
    return result;
}

void *SDL_ShaderCross_CompileDXILFromSPIRV(
    const SDL_ShaderCross_SPIRV_Info *info,
    size_t *size)
{
#ifndef SDL_SHADERCROSS_DXC
    SDL_SetError("%s", "Shadercross was not compiled with DXC support, cannot compile to SPIR-V!");
    return NULL;
#endif

    SPIRVTranspileContext *context = SDL_ShaderCross_INTERNAL_TranspileFromSPIRV(
        SPVC_BACKEND_HLSL,
        60,
        info->shader_stage,
        info->bytecode,
        info->bytecode_size,
        info->entrypoint,
        info->props);

    if (context == NULL) {
        return NULL;
    }

    SDL_ShaderCross_HLSL_Info hlslInfo;
    hlslInfo.source = context->translated_source;
    hlslInfo.entrypoint = context->cleansed_entrypoint;
    hlslInfo.include_dir = NULL;
    hlslInfo.defines = NULL;
    hlslInfo.shader_stage = info->shader_stage;
    hlslInfo.enable_debug = info->enable_debug;
    hlslInfo.name = info->name;
    hlslInfo.props = 0;

    void *result = SDL_ShaderCross_CompileDXILFromHLSL(
        &hlslInfo,
        size);

    SDL_ShaderCross_INTERNAL_DestroyTranspileContext(context);
    return result;
}

static void *SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_SPIRV_Info *info,
    void *metadata)
{
    SDL_GPUShaderFormat format;

    SDL_GPUShaderFormat shader_formats = SDL_GetGPUShaderFormats(device);

    if (shader_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        if (info->shader_stage == SDL_SHADERCROSS_SHADERSTAGE_COMPUTE) {
            SDL_GPUComputePipelineCreateInfo createInfo;
            SDL_ShaderCross_ComputePipelineMetadata *pipelineMetadata = (SDL_ShaderCross_ComputePipelineMetadata *)metadata;
            SDL_ShaderCross_ReflectComputeSPIRV(
                info->bytecode,
                info->bytecode_size,
                pipelineMetadata);
            createInfo.code = info->bytecode;
            createInfo.code_size = info->bytecode_size;
            createInfo.entrypoint = info->entrypoint;
            createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            createInfo.num_samplers = pipelineMetadata->num_samplers;
            createInfo.num_readonly_storage_textures = pipelineMetadata->num_readonly_storage_textures;
            createInfo.num_readonly_storage_buffers = pipelineMetadata->num_readonly_storage_buffers;
            createInfo.num_readwrite_storage_textures = pipelineMetadata->num_readwrite_storage_textures;
            createInfo.num_readwrite_storage_buffers = pipelineMetadata->num_readwrite_storage_buffers;
            createInfo.num_uniform_buffers = pipelineMetadata->num_uniform_buffers;
            createInfo.threadcount_x = pipelineMetadata->threadcount_x;
            createInfo.threadcount_y = pipelineMetadata->threadcount_y;
            createInfo.threadcount_z = pipelineMetadata->threadcount_z;

            createInfo.props = 0;
            if (info->name != NULL) {
                createInfo.props = SDL_CreateProperties();
                SDL_SetStringProperty(createInfo.props, SDL_PROP_GPU_COMPUTEPIPELINE_CREATE_NAME_STRING, info->name);
            }

            SDL_GPUComputePipeline *result = SDL_CreateGPUComputePipeline(device, &createInfo);

            if (createInfo.props != 0) {
                SDL_DestroyProperties(createInfo.props);
            }

            return result;
        } else {
            SDL_GPUShaderCreateInfo createInfo;
            SDL_ShaderCross_GraphicsShaderMetadata *shaderMetadata = (SDL_ShaderCross_GraphicsShaderMetadata *)metadata;
            SDL_ShaderCross_ReflectGraphicsSPIRV(
                info->bytecode,
                info->bytecode_size,
                shaderMetadata);
            createInfo.code = info->bytecode;
            createInfo.code_size = info->bytecode_size;
            createInfo.entrypoint = info->entrypoint;
            createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            createInfo.stage = (SDL_GPUShaderStage)info->shader_stage;
            createInfo.num_samplers = shaderMetadata->num_samplers;
            createInfo.num_storage_textures = shaderMetadata->num_storage_textures;
            createInfo.num_storage_buffers = shaderMetadata->num_storage_buffers;
            createInfo.num_uniform_buffers = shaderMetadata->num_uniform_buffers;

            createInfo.props = 0;
            if (info->name != NULL) {
                createInfo.props = SDL_CreateProperties();
                SDL_SetStringProperty(createInfo.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, info->name);
            }

            SDL_GPUShader *result = SDL_CreateGPUShader(device, &createInfo);

            if (createInfo.props != 0) {
                SDL_DestroyProperties(createInfo.props);
            }

            return result;
        }
    } else if (shader_formats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        if ((shader_formats & SDL_GPU_SHADERFORMAT_DXBC) && SDL_D3DCompile != NULL) {
            format = SDL_GPU_SHADERFORMAT_DXBC;
        }
#ifdef SDL_SHADERCROSS_DXC
        else if (shader_formats & SDL_GPU_SHADERFORMAT_DXIL) {
            format = SDL_GPU_SHADERFORMAT_DXIL;
        }
#endif
        else {
            SDL_SetError("SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV: Unexpected SDL_GPUBackend");
            return NULL;
        }
    }

    return SDL_ShaderCross_INTERNAL_CompileFromSPIRV(
        device,
        info,
        format,
        metadata);
}

SDL_GPUShader *SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_SPIRV_Info *info,
    SDL_ShaderCross_GraphicsShaderMetadata *metadata)
{
    return (SDL_GPUShader *)SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(
        device,
        info,
        metadata);
}

SDL_GPUComputePipeline *SDL_ShaderCross_CompileComputePipelineFromSPIRV(
    SDL_GPUDevice *device,
    const SDL_ShaderCross_SPIRV_Info *info,
    SDL_ShaderCross_ComputePipelineMetadata *metadata)
{
    return (SDL_GPUComputePipeline *)SDL_ShaderCross_INTERNAL_CreateShaderFromSPIRV(
        device,
        info,
        metadata);
}

bool SDL_ShaderCross_Init(void)
{
    d3dcompiler_dll = SDL_LoadObject(D3DCOMPILER_DLL);

    if (d3dcompiler_dll != NULL) {
        SDL_D3DCompile = (pfn_D3DCompile)SDL_LoadFunction(d3dcompiler_dll, "D3DCompile");

        if (SDL_D3DCompile == NULL) {
            SDL_UnloadObject(d3dcompiler_dll);
            d3dcompiler_dll = NULL;
        }
    }

    return true;
}

void SDL_ShaderCross_Quit(void)
{
    if (d3dcompiler_dll != NULL) {
        SDL_UnloadObject(d3dcompiler_dll);
        d3dcompiler_dll = NULL;

        SDL_D3DCompile = NULL;
    }
}

SDL_GPUShaderFormat SDL_ShaderCross_GetSPIRVShaderFormats(void)
{
    /* SPIRV and MSL can always be output as-is with no preprocessing since we require SPIRV-Cross */
    SDL_GPUShaderFormat supportedFormats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL;

    /* SPIRV-Cross + DXC allows us to cross-compile to HLSL, then compile to DXIL */
#ifdef SDL_SHADERCROSS_DXC
    supportedFormats |= SDL_GPU_SHADERFORMAT_DXIL;
#endif

    /* SPIRV-Cross + FXC allows us to cross-compile to HLSL, then compile to DXBC */
    if (d3dcompiler_dll != NULL) {
        supportedFormats |= SDL_GPU_SHADERFORMAT_DXBC;
    }

    return supportedFormats;
}

SDL_GPUShaderFormat SDL_ShaderCross_GetHLSLShaderFormats(void)
{
    SDL_GPUShaderFormat supportedFormats = 0;

    /* DXC allows compilation from HLSL to SPIRV */
#ifdef SDL_SHADERCROSS_DXC
    supportedFormats |= SDL_ShaderCross_GetSPIRVShaderFormats();
#endif

    /* FXC allows compilation of HLSL to DXBC */
    if (d3dcompiler_dll != NULL) {
        supportedFormats |= SDL_GPU_SHADERFORMAT_DXBC;
    }

    return supportedFormats;
}
