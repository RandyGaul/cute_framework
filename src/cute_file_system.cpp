/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/
#include <cute_file_system.h>
#include <cute_result.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>

#include <internal/cute_alloc_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_file_system_internal.h>

#include <physfs/physfs.h>

#define CF_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * CF_MB)

char* cf_path_get_filename(const char* path)
{
	if (!path || path[0] == '\0') { return NULL; }

	int at = slast_index_of(path, '/');

	const char *f = path + at + 1;
	if (f[0] != '\0') {
		return smake(f);
	}

	// The last character was "/" so there was no filename
	return NULL;
}

char* cf_path_get_filename_no_ext(const char* path)
{
	char* s = cf_path_get_filename(path);
	
	if (!s) { return NULL; }

	int at = slast_index_of(s, '.');
	if (at == 0) {
		// The filename only has an extension
		sfree(s);
		return NULL;
	}

	if (at == -1) {
		// The filename has no extension
		return s;
	}

	// Remove the extension from the filename
	serase(s, at, slen(s) - at);
	return s;
}

char* cf_path_get_ext(const char* path)
{
	int at = slast_index_of(path, '.');
	if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return NULL;
	return smake(path + at);
}

bool cf_path_ext_equ(const char* path, const char* ext)
{
	int at = slast_index_of(path, '.');
	if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return false;
	return sequ(path + at, ext);
}

char* cf_path_pop(const char* path)
{
	char* in = (char*)path;
	if (sisdyna(in)) {
		if (slast(in) == '/') spop(in);
		int at = slast_index_of(path, '/');
		if (at == -1 || at == 0) return sset(in, "/");
		serase(in, at, slen(in) - at);
		return in;
	} else {
		char* s = sdup(path);
		if (slast(s) == '/') spop(s);
		int at = slast_index_of(path, '/');
		if (at == -1 || at == 0) return sset(s, "/");
		serase(s, at, slen(s) - at);
		return s;
	}
}

char* cf_path_pop_n(const char* path, int n)
{
	char* s = (char*)path;
	while (n--) {
		s = sppop(s);
	}
	return s;
}

char* cf_path_compact(const char* path, int n)
{
	int len = (int)CF_STRLEN(path);
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
	if (!*path || (*path == '.' && CF_STRLEN(path) < 3)) return NULL;
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

char* cf_path_normalize(const char* path)
{
	char* result = NULL;
	int len = (int)CF_STRLEN(path);
	if (*path != '\\' && *path != '/') {
		bool windows_drive = len >= 2 && path[1] == ':';
		if (!windows_drive) {
			spush(result, '/');
		}
	}
	bool prev_was_dot = false;
	bool prev_was_dotdot = false;
	for (int i = 0; i < len; ++i) {
		char c = path[i];
		int l = slen(result);
		if (c == '\\' || c == '/') {
			if (!prev_was_dot) {
				spush(result, '/');
			} else if (prev_was_dotdot) {
				sppop(result);
				spush(result, '/');
			}
			prev_was_dot = false;
			prev_was_dotdot = false;
		} else if (c == '.') {
			if (prev_was_dot) {
				prev_was_dotdot = true;
			}
			prev_was_dot = true;
		} else {
			if (prev_was_dot) {
				spush(result, '.');
			}
			spush(result, c);
			prev_was_dot = false;
			prev_was_dotdot = false;
		}
	}
	sreplace(result, "//", "/");
	if (slen(result) > 1 && slast(result) == '/') spop(result);
	return result;
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

static CF_INLINE CF_FileType s_file_type(PHYSFS_FileType type)
{
	switch (type)
	{
	case PHYSFS_FILETYPE_REGULAR: return CF_FILE_TYPE_REGULAR;
	case PHYSFS_FILETYPE_DIRECTORY: return CF_FILE_TYPE_DIRECTORY;
	case PHYSFS_FILETYPE_SYMLINK: return CF_FILE_TYPE_SYMLINK;
	default: return CF_FILE_TYPE_OTHER;
	}
}

CF_Result cf_fs_stat(const char* virtual_path, CF_Stat* stat)
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

CF_File* cf_fs_create_file(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CF_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (CF_File*)file;
}

CF_File* cf_fs_open_file_for_write(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CF_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (CF_File*)file;
}

CF_File* cf_fs_open_file_for_append(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openAppend(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CF_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (CF_File*)file;
}

CF_File* cf_fs_open_file_for_read(const char* virtual_path)
{
	PHYSFS_file* file = PHYSFS_openRead(virtual_path);
	if (!file) {
		return NULL;
	} else if (!PHYSFS_setBuffer(file, CF_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
		PHYSFS_close(file);
		return NULL;
	}
	return (CF_File*)file;
}

CF_Result cf_fs_close(CF_File* file)
{
	if (!PHYSFS_close((PHYSFS_file*)file)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

CF_Result cf_fs_remove(const char* virtual_path)
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

size_t cf_fs_read(CF_File* file, void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_readBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

size_t cf_fs_write(CF_File* file, const void* buffer, size_t bytes)
{
	return (size_t)PHYSFS_writeBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes);
}

CF_Result cf_fs_eof(CF_File* file)
{
	if (!PHYSFS_eof((PHYSFS_file*)file)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

size_t cf_fs_tell(CF_File* file)
{
	return (size_t)PHYSFS_tell((PHYSFS_file*)file);
}

CF_Result cf_fs_seek(CF_File* file, size_t position)
{
	if (!PHYSFS_seek((PHYSFS_file*)file, (PHYSFS_uint64)position)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

size_t cf_fs_size(CF_File* file)
{
	return (size_t)PHYSFS_fileLength((PHYSFS_file*)file);
}

void* cf_fs_read_entire_file_to_memory(const char* virtual_path, size_t* size)
{
	CF_File* file = cf_fs_open_file_for_read(virtual_path);
	if (!file) return NULL;
	size_t sz = cf_fs_size(file);
	void* data = CF_ALLOC(sz);
	size_t sz_read = cf_fs_read(file, data, sz);
	CF_ASSERT(sz == sz_read);
	if (size) *size = sz_read;
	cf_fs_close(file);
	return data;
}

char* cf_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size)
{
	CF_File* file = cf_fs_open_file_for_read(virtual_path);
	void* data = NULL;
	if (!file) return NULL;
	size_t sz = cf_fs_size(file) + 1;
	data = CF_ALLOC(sz);
	size_t sz_read = cf_fs_read(file, data, sz);
	CF_ASSERT(sz == sz_read + 1);
	((char*)data)[sz - 1] = 0;
	if (size) *size = sz;
	cf_fs_close(file);
	return (char*)data;
}

CF_Result cf_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size)
{
	CF_File* file = cf_fs_open_file_for_write(virtual_path);
	if (!file) return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	uint64_t sz = cf_fs_write(file, data, (PHYSFS_uint64)size);
	if (sz != size) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
	cf_fs_close(file);
	return cf_result_success();
}

CF_Result cf_fs_write_string_to_file(const char* virtual_path, const char* string)
{
	return cf_fs_write_entire_buffer_to_file(virtual_path, (void*)string, CF_STRLEN(string));
}

CF_Result cf_fs_write_string_range_to_file(const char* virtual_path, const char* begin, const char* end)
{
	return cf_fs_write_entire_buffer_to_file(virtual_path, (void*)begin, end - begin);
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
