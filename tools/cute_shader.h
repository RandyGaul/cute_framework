#ifndef CUTE_SHADER_H
#define CUTE_SHADER_H

#include <cute_shader_bytecode.h>
#include <SDL3_shadercross/spirv.h>

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

void cute_shader_init(void);

void cute_shader_cleanup(void);

CF_ShaderCompilerResult cute_shader_compile(const char* source, CF_ShaderCompilerStage stage, CF_ShaderCompilerConfig config);

void cute_shader_free_result(CF_ShaderCompilerResult result);

#ifdef __cplusplus
}
#endif

#endif
