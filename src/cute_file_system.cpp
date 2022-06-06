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

#include <cute_file_system.h>
#include <cute_error.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>

#include <physfs/physfs.h>

#define CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * CUTE_MB)

namespace cute
{

const char* cf_file_system_get_base_dir()
{
	return PHYSFS_getBaseDir();
}

cf_error_t cf_file_system_set_write_dir(const char* platform_dependent_directory)
{
	if (!PHYSFS_setWriteDir(platform_dependent_directory)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

cf_error_t cf_file_system_mount(const char* archive, const char* mount_point, bool append_to_path)
{
	if (!PHYSFS_mount(archive, mount_point, (int)append_to_path)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

cf_error_t cf_file_system_dismount(const char* archive)
{
	if (!PHYSFS_unmount(archive)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

static CUTE_INLINE cf_file_type_t cf_s_file_type(PHYSFS_FileType type)
{
	switch (type)
	{
	case PHYSFS_FILETYPE_REGULAR: return CF_FILE_TYPE_REGULAR;
	case PHYSFS_FILETYPE_DIRECTORY: return CF_FILE_TYPE_DIRECTORY;
	case PHYSFS_FILETYPE_SYMLINK: return CF_FILE_TYPE_SYMLINK;
	default: return CF_FILE_TYPE_OTHER;
	}
}

cf_error_t cf_file_system_stat(const char* virtual_path, cf_stat_t* stat)
{
	PHYSFS_Stat physfs_stat;
	if (!PHYSFS_stat(virtual_path, &physfs_stat)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		stat->type = cf_s_file_type(physfs_stat.filetype);
		stat->is_read_only = physfs_stat.readonly;
		stat->size = physfs_stat.filesize;
		stat->last_modified_time = physfs_stat.modtime;
		stat->created_time = physfs_stat.createtime;
		stat->last_accessed_time = physfs_stat.accesstime;
		return cf_error_success();
	}
}

cf_file_t* cf_file_system_create_file(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (cf_file_t*)file;
}

cf_file_t* cf_file_system_open_file_for_write(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (cf_file_t*)file;
}

cf_file_t* cf_file_system_open_file_for_append(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openAppend(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (cf_file_t*)file;
}

cf_file_t* cf_file_system_open_file_for_read(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openRead(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (cf_file_t*)file;
}

cf_error_t cf_file_system_close(cf_file_t* file)
{
	if (!PHYSFS_close((PHYSFS_file*)file)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

cf_error_t cf_file_system_delete(const char* virtual_path)
{
	if (!PHYSFS_delete(virtual_path)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

cf_error_t cf_file_system_create_dir(const char* virtual_path)
{
	if (!PHYSFS_mkdir(virtual_path)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

const char** cf_file_system_enumerate_directory(const char* virtual_path)
{
	const char** file_list = (const char**)PHYSFS_enumerateFiles(virtual_path);
	if (!file_list) {
		return NULL;
	}
	return file_list;
}

void cf_file_system_free_enumerated_directory(const char** directory_list)
{
	PHYSFS_freeList(directory_list);
}

const char* cf_file_system_get_backend_specific_error_message()
{
	return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

const char* cf_file_system_get_user_directory(const char* org, const char* app)
{
	return PHYSFS_getPrefDir(org, app);
}

const char* cf_file_system_get_actual_path(const char* virtual_path)
{
	return PHYSFS_getRealDir(virtual_path);
}

void cf_file_system_enable_symlinks()
{
	PHYSFS_permitSymbolicLinks(1);
}

bool cf_file_system_file_exists(const char* virtual_path)
{
	return PHYSFS_exists(virtual_path) ? true : false;
}

size_t cf_file_system_read(cf_file_t* file, void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_readBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

size_t cf_file_system_write(cf_file_t* file, const void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_writeBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

cf_error_t cf_file_system_eof(cf_file_t* file)
{
	if (!PHYSFS_eof((PHYSFS_file*)file)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

size_t cf_file_system_tell(cf_file_t* file)
{
	return (size_t)PHYSFS_tell((PHYSFS_file*)file);
}

cf_error_t cf_file_system_seek(cf_file_t* file, size_t position)
{
	if (!PHYSFS_seek((PHYSFS_file*)file, (PHYSFS_uint64)position)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

size_t cf_file_system_size(cf_file_t* file)
{
	return (size_t)PHYSFS_fileLength((PHYSFS_file*)file);
}

cf_error_t cf_file_system_flush(cf_file_t* file)
{
	if (!PHYSFS_flush((PHYSFS_file*)file)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

cf_error_t cf_file_system_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size, void* user_allocator_context)
{
	CUTE_ASSERT(data_ptr);
	cf_file_t* file = cf_file_system_open_file_for_read(virtual_path);
	void* data = NULL;
	if (!file) return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	size_t sz = cf_file_system_size(file);
	data = CUTE_ALLOC(sz, user_allocator_context);
	size_t sz_read = cf_file_system_read(file, data, sz);
	CUTE_ASSERT(sz == sz_read);
	*data_ptr = data;
	if (size) *size = sz_read;
	cf_file_system_close(file);
	return cf_error_success();
}

cf_error_t cf_file_system_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size, void* user_allocator_context)
{
	CUTE_ASSERT(data_ptr);
	cf_file_t* file = cf_file_system_open_file_for_read(virtual_path);
	void* data = NULL;
	if (!file) return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	size_t sz = cf_file_system_size(file) + 1;
	data = CUTE_ALLOC(sz, user_allocator_context);
	size_t sz_read = cf_file_system_read(file, data, sz);
	CUTE_ASSERT(sz == sz_read + 1);
	*data_ptr = data;
	((char*)data)[sz - 1] = 0;
	if (size) *size = sz;
	cf_file_system_close(file);
	return cf_error_success();
}

cf_error_t cf_file_system_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size)
{
	cf_file_t* file = cf_file_system_open_file_for_write(virtual_path);
	if (!file) return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	uint64_t sz = cf_file_system_write(file, data, (PHYSFS_uint64)size);
	if (sz != size) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
	cf_file_system_close(file);
	return cf_error_success();
}

cf_error_t cf_file_system_init(const char* argv0)
{
	if (!PHYSFS_init(argv0)) {
		return cf_error_failure(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_error_success();
	}
}

void cf_file_system_destroy()
{
	PHYSFS_deinit();
}

}
