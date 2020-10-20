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

#include <cute_defines.h>
#include <cute_error.h>

namespace cute
{

struct file_t;

enum file_type_t : int
{
	FILE_TYPE_REGULAR,
	FILE_TYPE_DIRECTORY,
	FILE_TYPE_SYMLINK,
	FILE_TYPE_OTHER,
};

struct stat_t
{
	file_type_t type;
	int is_read_only;
	size_t size;
	uint64_t last_modified_time;
	uint64_t created_time;
	uint64_t last_accessed_time;
};

CUTE_API const char* CUTE_CALL file_system_get_base_dir();
CUTE_API error_t CUTE_CALL file_system_set_write_dir(const char* platform_dependent_directory);
CUTE_API error_t CUTE_CALL file_system_mount(const char* archive, const char* mount_point, bool append_to_path = true);
CUTE_API error_t CUTE_CALL file_system_dismount(const char* archive);
CUTE_API error_t CUTE_CALL file_system_stat(const char* virtual_path, stat_t* stat);
CUTE_API file_t* CUTE_CALL file_system_create_file(const char* virtual_path);
CUTE_API file_t* CUTE_CALL file_system_open_file_for_write(const char* virtual_path);
CUTE_API file_t* CUTE_CALL file_system_open_file_for_append(const char* virtual_path);
CUTE_API file_t* CUTE_CALL file_system_open_file_for_read(const char* virtual_path);
CUTE_API error_t CUTE_CALL file_system_close(file_t* file);
CUTE_API error_t CUTE_CALL file_system_delete(const char* virtual_path);
CUTE_API error_t CUTE_CALL file_system_create_dir(const char* virtual_path);
CUTE_API const char** CUTE_CALL file_system_enumerate_directory(const char* virtual_path);
CUTE_API void CUTE_CALL file_system_free_enumerated_directory(const char** directory_list);
CUTE_API error_t CUTE_CALL file_system_file_exists(const char* virtual_path);
CUTE_API size_t CUTE_CALL file_system_read(file_t* file, void* buffer, size_t bytes);
CUTE_API size_t CUTE_CALL file_system_write(file_t* file, const void* buffer, size_t bytes);
CUTE_API error_t CUTE_CALL file_system_eof(file_t* file);
CUTE_API size_t CUTE_CALL file_system_tell(file_t* file);
CUTE_API error_t CUTE_CALL file_system_seek(file_t* file, size_t position);
CUTE_API size_t CUTE_CALL file_system_size(file_t* file);
CUTE_API error_t CUTE_CALL file_system_flush(file_t* file);
CUTE_API error_t CUTE_CALL file_system_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size = NULL, void* user_allocator_context = NULL);
CUTE_API error_t CUTE_CALL file_system_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size = NULL, void* user_allocator_context = NULL);
CUTE_API error_t CUTE_CALL file_system_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);

}

#include <cute_file_system_utils.h>

#endif // CUTE_FILE_SYSTEM_H
