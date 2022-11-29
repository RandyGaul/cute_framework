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
#include <cute_result.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>

#include <physfs/physfs.h>

#define CUTE_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * CUTE_MB)

char* cf_path_get_filename(const char* path)
{
	int at = slast_index_of(path, '/');
	if (at == -1) return NULL;
	return smake(path + at + 1);
}

char* cf_path_get_filename_no_ext(const char* path)
{
	int at = slast_index_of(path, '.');
	if (at == -1) return NULL;
	char* s = (char*)cf_path_get_filename(path);
	at = slast_index_of(s, '.');
	if (at == -1 || at == 0) {
		sfree(s);
		return NULL;
	}
	serase(s, at, slen(s) - at);
	return s;
}

char* cf_path_get_ext(const char* path)
{
	int at = slast_index_of(path, '.');
	if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return NULL;
	return smake(path + at);
}

char* cf_path_pop(const char* path)
{
	int len = (int)CUTE_STRLEN(path);
	int at = slast_index_of(path, '/');
	if (at == -1 || at == 0) return smake("/");
	char* s = sdup(path);
	serase(s, at, slen(s) - at);
	return s;
}

char* cf_path_compact(const char* path, int n)
{
	int len = (int)CUTE_STRLEN(path);
	if (n <= 6) return NULL;
	if (len < n) return sdup(path);
	int at = slast_index_of(path, '/');
	if (at == -1 || at == 0) {
		char* s = sdup(path);
		serase(s, n, slen(s) - n);
		serase(s, n - 3, 3);
		return sappend(s, "...");
	}
	int remaining = len - at - 1;
	if (remaining >= n - 3) {
		char* s = smake("...");
		sappend_range(s, path, path + at - 6);
		return sappend(s, "...");
	} else {
		char* s = sdup(path);
		int len_s = slen(s);
		int to_erase = len_s - (remaining - 3);
		serase(s, remaining - 3, to_erase);
		sappend(s, "...");
		return sappend(s, path + at);
	}
}

char* cf_path_directory_of(const char* path)
{
	if (!*path || *path == '.' && CUTE_STRLEN(path) < 3) return NULL;
	if (sequ(path, "../")) return NULL;
	if (sequ(path, "/")) return NULL;
	int at = slast_index_of(path, '/');
	if (at == -1) return NULL;
	if (at == 0) return smake("/");
	char* s = smake(path);
	serase(s, at, slen(s) - at);
	at = slast_index_of(s, '/');
	if (at == -1) {
		int l = slen(s);
		if (slen(s) == 2) {
			return s;
		} else {
			s[0] = '/';
			return s;
		}
	}
	return serase(s, 0, at);
}

char* cf_path_top_directory(const char* path)
{
	int at = sfirst_index_of(path, '/');
	if (at == -1) return NULL;
	int next = sfirst_index_of(path + at + 1, '/');
	if (next == -1) return smake("/");
	char* s = sdup(path);
	return serase(s, next + 1, slen(s) - (next + 1));
}

//--------------------------------------------------------------------------------------------------

const char* cf_fs_get_base_directory()
{
	return PHYSFS_getBaseDir();
}

CF_Result cf_fs_set_write_directory(const char* platform_dependent_directory)
{
	if (!PHYSFS_setWriteDir(platform_dependent_directory)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_fs_mount(const char* archive, const char* mount_point, bool append_to_path)
{
	if (!PHYSFS_mount(archive, mount_point, (int)append_to_path)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_fs_dismount(const char* archive)
{
	if (!PHYSFS_unmount(archive)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

static CUTE_INLINE cf_file_type_t s_file_type(PHYSFS_FileType type)
{
	switch (type)
	{
	case PHYSFS_FILETYPE_REGULAR: return CF_FILE_TYPE_REGULAR;
	case PHYSFS_FILETYPE_DIRECTORY: return CF_FILE_TYPE_DIRECTORY;
	case PHYSFS_FILETYPE_SYMLINK: return CF_FILE_TYPE_SYMLINK;
	default: return CF_FILE_TYPE_OTHER;
	}
}

CF_Result cf_fs_stat(const char* virtual_path, cf_stat_t* stat)
{
	PHYSFS_Stat physfs_stat;
	if (!PHYSFS_stat(virtual_path, &physfs_stat)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		stat->type = s_file_type(physfs_stat.filetype);
		stat->is_read_only = physfs_stat.readonly;
		stat->size = physfs_stat.filesize;
		stat->last_modified_time = physfs_stat.modtime;
		stat->created_time = physfs_stat.createtime;
		stat->last_accessed_time = physfs_stat.accesstime;
		return cf_result_success();
	}
}

cf_file_t* cf_fs_create_file(const char* virtual_path)
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

cf_file_t* cf_fs_open_file_for_write(const char* virtual_path)
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

cf_file_t* cf_fs_open_file_for_append(const char* virtual_path)
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

cf_file_t* cf_fs_open_file_for_read(const char* virtual_path)
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

CF_Result cf_fs_close(cf_file_t* file)
{
	if (!PHYSFS_close((PHYSFS_file*)file)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_fs_delete(const char* virtual_path)
{
	if (!PHYSFS_delete(virtual_path)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_fs_create_directory(const char* virtual_path)
{
	if (!PHYSFS_mkdir(virtual_path)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

const char** cf_fs_enumerate_directory(const char* virtual_path)
{
	const char** file_list = (const char**)PHYSFS_enumerateFiles(virtual_path);
	if (!file_list) {
		return NULL;
	}
	return file_list;
}

void cf_fs_free_enumerated_directory(const char** directory_list)
{
	PHYSFS_freeList(directory_list);
}

const char* cf_fs_get_backend_specific_error_message()
{
	return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

const char* cf_fs_get_user_directory(const char* company_name, const char* game_name)
{
	return PHYSFS_getPrefDir(company_name, game_name);
}

const char* cf_fs_get_actual_path(const char* virtual_path)
{
	return PHYSFS_getRealDir(virtual_path);
}

bool cf_fs_file_exists(const char* virtual_path)
{
	return PHYSFS_exists(virtual_path) ? true : false;
}

size_t cf_fs_read(cf_file_t* file, void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_readBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

size_t cf_fs_write(cf_file_t* file, const void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_writeBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

CF_Result cf_fs_eof(cf_file_t* file)
{
	if (!PHYSFS_eof((PHYSFS_file*)file)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

size_t cf_fs_tell(cf_file_t* file)
{
	return (size_t)PHYSFS_tell((PHYSFS_file*)file);
}

CF_Result cf_fs_seek(cf_file_t* file, size_t position)
{
	if (!PHYSFS_seek((PHYSFS_file*)file, (PHYSFS_uint64)position)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

size_t cf_fs_size(cf_file_t* file)
{
	return (size_t)PHYSFS_fileLength((PHYSFS_file*)file);
}

CF_Result cf_fs_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size)
{
	CUTE_ASSERT(data_ptr);
	cf_file_t* file = cf_fs_open_file_for_read(virtual_path);
	void* data = NULL;
	if (!file) return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	size_t sz = cf_fs_size(file);
	data = CUTE_ALLOC(sz);
	size_t sz_read = cf_fs_read(file, data, sz);
	CUTE_ASSERT(sz == sz_read);
	*data_ptr = data;
	if (size) *size = sz_read;
	cf_fs_close(file);
	return cf_result_success();
}

CF_Result cf_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size)
{
	CUTE_ASSERT(data_ptr);
	cf_file_t* file = cf_fs_open_file_for_read(virtual_path);
	void* data = NULL;
	if (!file) return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	size_t sz = cf_fs_size(file) + 1;
	data = CUTE_ALLOC(sz);
	size_t sz_read = cf_fs_read(file, data, sz);
	CUTE_ASSERT(sz == sz_read + 1);
	*data_ptr = data;
	((char*)data)[sz - 1] = 0;
	if (size) *size = sz;
	cf_fs_close(file);
	return cf_result_success();
}

CF_Result cf_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size)
{
	cf_file_t* file = cf_fs_open_file_for_write(virtual_path);
	if (!file) return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	uint64_t sz = cf_fs_write(file, data, (PHYSFS_uint64)size);
	if (sz != size) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
	cf_fs_close(file);
	return cf_result_success();
}

CF_Result cf_fs_init(const char* argv0)
{
	if (!PHYSFS_init(argv0)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

void cf_fs_destroy()
{
	PHYSFS_deinit();
}

