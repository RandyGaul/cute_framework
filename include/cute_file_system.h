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

#ifndef CUTE_FILE_SYSTEM_H
#define CUTE_FILE_SYSTEM_H

#include "cute_defines.h"
#include "cute_error.h"

typedef struct cf_file_t cf_file_t;

typedef enum cf_file_type_t //: int
{
	CF_FILE_TYPE_REGULAR,
	CF_FILE_TYPE_DIRECTORY,
	CF_FILE_TYPE_SYMLINK,
	CF_FILE_TYPE_OTHER,
} cf_file_type_t;

typedef struct cf_stat_t
{
	cf_file_type_t type;
	int is_read_only;
	size_t size;
	uint64_t last_modified_time;
	uint64_t created_time;
	uint64_t last_accessed_time;
} cf_stat_t;

CUTE_API const char* CUTE_CALL cf_file_system_get_base_dir();
CUTE_API cf_error_t CUTE_CALL cf_file_system_set_write_dir(const char* platform_dependent_directory);
CUTE_API cf_error_t CUTE_CALL cf_file_system_mount(const char* archive, const char* mount_point, bool append_to_path /*= true*/);
CUTE_API cf_error_t CUTE_CALL cf_file_system_dismount(const char* archive);
CUTE_API cf_error_t CUTE_CALL cf_file_system_stat(const char* virtual_path, cf_stat_t* stat);
CUTE_API cf_file_t* CUTE_CALL cf_file_system_create_file(const char* virtual_path);
CUTE_API cf_file_t* CUTE_CALL cf_file_system_open_file_for_write(const char* virtual_path);
CUTE_API cf_file_t* CUTE_CALL cf_file_system_open_file_for_append(const char* virtual_path);
CUTE_API cf_file_t* CUTE_CALL cf_file_system_open_file_for_read(const char* virtual_path);
CUTE_API cf_error_t CUTE_CALL cf_file_system_close(cf_file_t* file);
CUTE_API cf_error_t CUTE_CALL cf_file_system_delete(const char* virtual_path);
CUTE_API cf_error_t CUTE_CALL cf_file_system_create_dir(const char* virtual_path);
CUTE_API const char** CUTE_CALL cf_file_system_enumerate_directory(const char* virtual_path);
CUTE_API void CUTE_CALL cf_file_system_free_enumerated_directory(const char** directory_list);
CUTE_API bool CUTE_CALL cf_file_system_file_exists(const char* virtual_path);
CUTE_API size_t CUTE_CALL cf_file_system_read(cf_file_t* file, void* buffer, size_t bytes);
CUTE_API size_t CUTE_CALL cf_file_system_write(cf_file_t* file, const void* buffer, size_t bytes);
CUTE_API cf_error_t CUTE_CALL cf_file_system_eof(cf_file_t* file);
CUTE_API size_t CUTE_CALL cf_file_system_tell(cf_file_t* file);
CUTE_API cf_error_t CUTE_CALL cf_file_system_seek(cf_file_t* file, size_t position);
CUTE_API size_t CUTE_CALL cf_file_system_size(cf_file_t* file);
CUTE_API cf_error_t CUTE_CALL cf_file_system_flush(cf_file_t* file);
CUTE_API cf_error_t CUTE_CALL cf_file_system_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size /*= NULL*/, void* user_allocator_context /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_file_system_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size /*= NULL*/, void* user_allocator_context /*= NULL*/);
CUTE_API cf_error_t CUTE_CALL cf_file_system_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);

#ifdef CUTE_CPP

namespace cute
{
using file_type_t = cf_file_type_t;
using stat_t = cf_stat_t;
using file_t = cf_file_t;

CUTE_INLINE const char* file_system_get_base_dir() { return cf_file_system_get_base_dir(); }
CUTE_INLINE error_t file_system_set_write_dir(const char* platform_dependent_directory) { return cf_file_system_set_write_dir(platform_dependent_directory); }
CUTE_INLINE error_t file_system_mount(const char* archive, const char* mount_point, bool append_to_path = true) { return cf_file_system_mount(archive, mount_point, append_to_path); }
CUTE_INLINE error_t file_system_dismount(const char* archive) { return cf_file_system_dismount(archive); }
CUTE_INLINE error_t file_system_stat(const char* virtual_path, stat_t* stat) { return cf_file_system_stat(virtual_path, stat); }
CUTE_INLINE file_t* file_system_create_file(const char* virtual_path) { return cf_file_system_create_file(virtual_path); }
CUTE_INLINE file_t* file_system_open_file_for_write(const char* virtual_path) { return cf_file_system_open_file_for_write(virtual_path); }
CUTE_INLINE file_t* file_system_open_file_for_append(const char* virtual_path) { return cf_file_system_open_file_for_append(virtual_path); }
CUTE_INLINE file_t* file_system_open_file_for_read(const char* virtual_path) { return cf_file_system_open_file_for_read(virtual_path); }
CUTE_INLINE error_t file_system_close(file_t* file) { return cf_file_system_close(file); }
CUTE_INLINE error_t file_system_delete(const char* virtual_path) { return cf_file_system_delete(virtual_path); }
CUTE_INLINE error_t file_system_create_dir(const char* virtual_path) { return cf_file_system_create_dir(virtual_path); }
CUTE_INLINE const char** file_system_enumerate_directory(const char* virtual_path) { return cf_file_system_enumerate_directory(virtual_path); }
CUTE_INLINE void file_system_free_enumerated_directory(const char** directory_list) { cf_file_system_free_enumerated_directory(directory_list); }
CUTE_INLINE bool file_system_file_exists(const char* virtual_path) { return cf_file_system_file_exists(virtual_path); }
CUTE_INLINE size_t file_system_read(file_t* file, void* buffer, size_t bytes) { return cf_file_system_read(file, buffer, bytes); }
CUTE_INLINE size_t file_system_write(file_t* file, const void* buffer, size_t bytes) { return cf_file_system_write(file, buffer, bytes); }
CUTE_INLINE error_t file_system_eof(file_t* file) { return cf_file_system_eof(file); }
CUTE_INLINE size_t file_system_tell(file_t* file) { return cf_file_system_tell(file); }
CUTE_INLINE error_t file_system_seek(file_t* file, size_t position) { return cf_file_system_seek(file, position); }
CUTE_INLINE size_t file_system_size(file_t* file) { return cf_file_system_size(file); }
CUTE_INLINE error_t file_system_flush(file_t* file) { return cf_file_system_flush(file); }
CUTE_INLINE error_t file_system_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size = NULL, void* user_allocator_context = NULL) { return cf_file_system_read_entire_file_to_memory(virtual_path, data_ptr, size, user_allocator_context); }
CUTE_INLINE error_t file_system_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size = NULL, void* user_allocator_context = NULL) { return cf_file_system_read_entire_file_to_memory_and_nul_terminate(virtual_path, data_ptr, size, user_allocator_context); }
CUTE_INLINE error_t file_system_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size) { return cf_file_system_write_entire_buffer_to_file(virtual_path, data, size); }

}

#endif // CUTE_CPP

#include "cute_file_system_utils.h"

#endif // CUTE_FILE_SYSTEM_H
