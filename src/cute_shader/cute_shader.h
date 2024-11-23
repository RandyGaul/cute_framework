#ifndef CUTE_SHADER_H
#define CUTE_SHADER_H

#include <stddef.h>

typedef struct cute_shader_define_t {
	const char* name;
	const char* value;
} cute_shader_define_t;

typedef struct cute_shader_file_t {
	const char* name;
	const char* content;
} cute_shader_file_t;

typedef struct cute_shader_vfs_t {
	char* (*read_file_content)(const char* path, size_t* len, void* fcontext);
	void (*free_file_content)(char* content, void* context);
	void* context;
} cute_shader_vfs_t;

typedef enum {
	CUTE_SHADER_STAGE_VERTEX,
	CUTE_SHADER_STAGE_FRAGMENT,
} cute_shader_stage_t;

typedef struct cute_shader_config_t {
	int num_builtin_defines;
	cute_shader_define_t* builtin_defines;

	int num_builtin_includes;
	cute_shader_file_t* builtin_includes;

	int num_include_dirs;
	const char** include_dirs;

	bool automatic_include_guard;
	bool return_preprocessed_source;

	cute_shader_vfs_t* vfs;
} cute_shader_config_t;

typedef struct cute_shader_result_t {
	bool success;

	void* bytecode;
	size_t bytecode_size;

	const char* preprocessed_source;
	size_t preprocessed_source_size;

	const char* error_message;
} cute_shader_result_t;

#ifdef __cplusplus
extern "C" {
#endif

void
cute_shader_init(void);

void
cute_shader_cleanup(void);

cute_shader_result_t
cute_shader_compile(
	const char* source,
	cute_shader_stage_t stage,
	cute_shader_config_t config
);

void
cute_shader_free_result(cute_shader_result_t result);

#ifdef __cplusplus
}
#endif

#endif
