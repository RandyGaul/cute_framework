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

#include <physfs.h>

#ifdef CF_WINDOWS
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <time.h>
#else
#	include <dirent.h>
#	include <fcntl.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

#define CF_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * CF_MB)


//--------------------------------------------------------------------------------------------------

const char* cf_fs_get_base_directory()
{
	return PHYSFS_getBaseDir();
}

const char* cf_fs_get_working_directory()
{
	return SDL_GetCurrentDirectory();
}

CF_Result cf_fs_set_write_directory(const char* platform_dependent_directory)
{
	if (!PHYSFS_setWriteDir(platform_dependent_directory)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

// Bumped by the two calls below, which are the only ones in CF that can change the search path.
// See cf_fs_generation.
static uint64_t s_fs_generation = 1;

uint64_t cf_fs_generation()
{
	return s_fs_generation;
}

CF_Result cf_fs_mount(const char* archive, const char* mount_point, bool append_to_path)
{
	if (!PHYSFS_mount(archive, mount_point, (int)append_to_path)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		++s_fs_generation;
		return cf_result_success();
	}
}

CF_Result cf_fs_dismount(const char* archive)
{
	if (!PHYSFS_unmount(archive)) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		++s_fs_generation;
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

//--------------------------------------------------------------------------------------------------
// Native (non-PhysFS) bulk directory walk. See internal/cute_file_system_internal.h.

#ifdef CF_WINDOWS

// Mirrors PhysFS's FileTimeToPhysfsTime, which round-trips through local time rather than
// subtracting the epoch. Agreeing with PhysFS matters more than being right: the two disagree in
// the ambiguous DST fall-back hour, under a TZ override, and across historical rule changes, and a
// timestamp from here gets compared against one PHYSFS_stat produced.
static bool s_filetime_to_physfs_time(const FILETIME* ft, const TIME_ZONE_INFORMATION* tzi, uint64_t* out)
{
	SYSTEMTIME st_utc, st_local;
	if (!FileTimeToSystemTime(ft, &st_utc)) return false;
	if (!SystemTimeToTzSpecificLocalTime(tzi, &st_utc, &st_local)) return false;

	struct tm tm;
	tm.tm_sec = st_local.wSecond;
	tm.tm_min = st_local.wMinute;
	tm.tm_hour = st_local.wHour;
	tm.tm_mday = st_local.wDay;
	tm.tm_mon = st_local.wMonth - 1;
	tm.tm_year = st_local.wYear - 1900;
	tm.tm_wday = -1;
	tm.tm_yday = -1;
	tm.tm_isdst = -1;

	time_t t = mktime(&tm);
	if (t == (time_t)-1) return false;
	*out = (uint64_t)t;
	return true;
}

bool cf_fs_scan_native_directory(const char* platform_directory, Cute::Map<CF_DirEntry>* out)
{
	TIME_ZONE_INFORMATION tzi;
	if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID) return false;

	// A trailing separator would leave an empty path component ahead of the wildcard.
	Cute::String pattern = platform_directory;
	while (pattern.len() > 0 && (pattern.last() == '/' || pattern.last() == '\\')) {
		pattern.pop();
	}
	if (pattern.len() == 0) return false;
	pattern.append("\\*");

	int wlen = MultiByteToWideChar(CP_UTF8, 0, pattern.c_str(), -1, NULL, 0);
	if (!wlen) return false;
	Cute::Array<wchar_t> wpattern;
	wpattern.set_count(wlen);
	if (!MultiByteToWideChar(CP_UTF8, 0, pattern.c_str(), -1, wpattern.data(), wlen)) return false;

	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(wpattern.data(), &fd);
	if (h == INVALID_HANDLE_VALUE) return false;
	do {
		if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;

		// PhysFS types these through a *followed* GetFileAttributesExW, which FindFirstFileW cannot
		// reproduce: it reports the link, not the target. Omit them so the caller stats them.
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

		char utf8[1024];  // One path component: at most 255 wchars, 1020 UTF-8 bytes.
		if (!WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, utf8, sizeof(utf8), NULL, NULL)) continue;

		CF_DirEntry e;
		e.name = sintern(utf8);
		e.is_directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		e.size = e.is_directory ? 0 : ((((uint64_t)fd.nFileSizeHigh) << 32) | fd.nFileSizeLow);
		if (!s_filetime_to_physfs_time(&fd.ftLastWriteTime, &tzi, &e.modified_time)) continue;
		e.stat_ok = true;
		out->add(e.name, e);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
	return true;
}

#else

bool cf_fs_scan_native_directory(const char* platform_directory, Cute::Map<CF_DirEntry>* out)
{
	int dir_fd = open(platform_directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (dir_fd < 0) return false;
	DIR* d = fdopendir(dir_fd);  // Takes ownership of dir_fd.
	if (!d) { close(dir_fd); return false; }

	struct dirent* de;
	while ((de = readdir(d)) != NULL) {
		if (!CF_STRCMP(de->d_name, ".") || !CF_STRCMP(de->d_name, "..")) continue;

		CF_DirEntry e;
		e.name = sintern(de->d_name);
		e.stat_ok = true;

		struct stat st;
		// AT_SYMLINK_NOFOLLOW is lstat, matching PhysFS's DIR_stat. Following would give a symlink
		// the target's type and mtime where PhysFS reports the link's.
		if (fstatat(dirfd(d), de->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) continue;
		e.is_directory = S_ISDIR(st.st_mode);
		e.size = (uint64_t)st.st_size;
		e.modified_time = (uint64_t)st.st_mtime;
		out->add(e.name, e);
	}
	closedir(d);
	return true;
}

#endif // CF_WINDOWS

// PhysFS reports a mount point with a trailing slash and no leading one ("shaders/extra/"), or "/"
// for a root mount, while virtual paths arrive in any shape. Reducing both to a bare form makes
// subtracting one from the other a plain string compare, which is what verifyPath does internally.
static Cute::String s_bare_virtual_path(const char* path)
{
	int begin = 0;
	while (path[begin] == '/') { ++begin; }
	Cute::String bare = path + begin;
	while (bare.len() > 0 && bare.last() == '/') { bare.pop(); }
	return bare;
}

void cf_fs_mount_candidates(const char* virtual_directory, Cute::Array<Cute::String>* out,
                            Cute::Array<Cute::String>* out_virtual_names)
{
	char** search = PHYSFS_getSearchPath();
	if (!search) return;

	Cute::String vdir = s_bare_virtual_path(virtual_directory);
	for (char** it = search; *it; ++it) {
		const char* raw_mount_point = PHYSFS_getMountPoint(*it);
		if (!raw_mount_point) continue;
		Cute::String mount_point = s_bare_virtual_path(raw_mount_point);
		Cute::String candidate = *it;

		if (mount_point.len() == 0) {
			// Root mount: the whole virtual path is the path inside the archive.
			if (vdir.len() > 0) {
				candidate.append("/");
				candidate.append(vdir.c_str());
			}
		} else if (CF_STRCMP(mount_point.c_str(), vdir.c_str()) == 0) {
			// The mount point is this directory, so the archive root is the directory.
		} else if (vdir.len() > mount_point.len() &&
		           CF_STRNCMP(vdir.c_str(), mount_point.c_str(), (size_t)mount_point.len()) == 0 &&
		           vdir.c_str()[mount_point.len()] == '/') {
			candidate.append("/");
			candidate.append(vdir.c_str() + mount_point.len() + 1);
		} else if (mount_point.len() > vdir.len() &&
		           (vdir.len() == 0 ||
		            (CF_STRNCMP(mount_point.c_str(), vdir.c_str(), (size_t)vdir.len()) == 0 &&
		             mount_point.c_str()[vdir.len()] == '/'))) {
			// The mount point sits below this directory, so the mount serves no real directory here
			// -- it only makes the next component of its mount point appear as a subdirectory.
			const char* rest = mount_point.c_str() + (vdir.len() ? vdir.len() + 1 : 0);
			// A character at a time, not String(start, end): that constructor strncpy's exactly
			// `length` bytes and then claims a length one longer, leaving the string unterminated
			// whenever the source runs past `end`.
			Cute::String name;
			for (int k = 0; rest[k] && rest[k] != '/'; ++k) {
				name.add(rest[k]);
			}
			out_virtual_names->add(cf_move(name));
			continue;
		} else {
			continue;  // This mount cannot reach the path at all.
		}

		out->add(cf_move(candidate));
	}

	PHYSFS_freeList(search);
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
	if (!PHYSFS_init(argv0 ? argv0 : "")) {
		return cf_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	} else {
		return cf_result_success();
	}
}

void cf_fs_destroy()
{
	PHYSFS_deinit();
}
