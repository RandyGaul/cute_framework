#ifndef CUTE_SHADER_H
#define CUTE_SHADER_H

#include <cute_shader_bytecode.h>

typedef struct CF_ShaderCompilerDefine
{
	const char* name;
	const char* value;
} CF_ShaderCompilerDefine;

typedef struct CF_ShaderCompilerFile
{
	const char* name;
	const char* content;
} CF_ShaderCompilerFile;

typedef struct CF_ShaderCompilerVfs
{
	char* (*read_file_content)(const char* path, size_t* len, void* context);
	void (*free_file_content)(char* content, void* context);
	void* context;
} CF_ShaderCompilerVfs;

typedef enum CF_ShaderCompilerStage
{
	CUTE_SHADER_STAGE_VERTEX,
	CUTE_SHADER_STAGE_FRAGMENT,
	CUTE_SHADER_STAGE_COMPUTE,
} CF_ShaderCompilerStage;

typedef struct CF_ShaderCompilerConfig
{
	int num_builtin_defines;
	CF_ShaderCompilerDefine* builtin_defines;

	int num_builtin_includes;
	CF_ShaderCompilerFile* builtin_includes;

	int num_include_dirs;
	const char** include_dirs;

	bool automatic_include_guard;
	bool return_preprocessed_source;

	// Skip transpilation to GLSL ES 300. Used for shaders using features GLES3/WebGL2
	// cannot express (e.g. storage buffers in the tiled draw path), and for runtime
	// compilation on backends that never consume GLSL (D3D12/Vulkan/Metal). Offline
	// compilation (cute-shaderc) keeps it so bytecode works on every backend.
	bool skip_glsl300;

	// Skip transpilation to HLSL SM 5.1. Runtime compilation skips it on every
	// backend except D3D12-without-SPIR-V; offline compilation keeps it.
	bool skip_hlsl;

	// Skip transpilation to MSL. Runtime compilation skips it on every backend
	// except Metal-without-SPIR-V; offline compilation keeps it.
	bool skip_msl;

	// Optional: when a user draw shader is injected as shader_stub.shd, report its
	// errors under this name (usually the user's shader path) instead.
	const char* shader_stub_display_name;

	CF_ShaderCompilerVfs* vfs;
} CF_ShaderCompilerConfig;

typedef struct CF_ShaderCompilerResult
{
	bool success;
	const char* error_message;

	CF_ShaderBytecode bytecode;

	const char* preprocessed_source;
	size_t preprocessed_source_size;
} CF_ShaderCompilerResult;

#ifdef __cplusplus
extern "C" {
#endif

CF_ShaderCompilerResult cute_shader_compile(const char* source, CF_ShaderCompilerStage stage, CF_ShaderCompilerConfig config);

// Runs only the preprocessor (comments stripped, macros expanded, includes
// resolved). Returns a cf_alloc'd string (free with cf_free()), or NULL on error.
// Useful for scanning declared resources without a hand-rolled parser.
char* cute_shader_preprocess(const char* source, CF_ShaderCompilerConfig config);

void cute_shader_free_result(CF_ShaderCompilerResult result);

#ifdef __cplusplus
}
#endif

#endif
