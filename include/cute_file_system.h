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
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// Path helper functions.
// These are implemented with the C-string API in "cute_string.h"; each function returns a fully
// mutable string you must free up with `sfree` or `cf_string_free` when done.

#ifndef CUTE_NO_SHORTHAND_API
/**
 * Returns the filename portion of a path.
 * 
 * Example:
 * 
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 * 
 * Would print:
 * 
 *     big_gem.txt
 */
#define spfname(s) cf_path_get_filename(s)

/**
 * Returns the filename portion of a path without the file extension.
 * 
 * Example:
 * 
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 * 
 * Would print:
 * 
 *     big_gem
 */
#define spfname_no_ext(s) cf_path_get_filename_no_ext(s)

/**
 * Returns the extension of the file for the given path.
 * 
 * Example:
 * 
 *     const char* ext = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", ext);
 * 
 * Would print:
 * 
 *     .txt
 */
#define spext(s) cf_path_get_ext(s)

/**
 * Removes the rightmost file or directory from the path.
 */
#define sppop(s) cf_path_pop(s)

/**
 * Squishes the path to be less than or equal to n characters in length. This will
 * insert ellipses "..." into the path as necessary. This function is useful for
 * displaying paths and visualizing them in small boxes or windows. n includes the
 * nul-byte.
 */
#define spcompact(s, n) cf_path_compact(s, n)

/**
 * Returns the directory of a given file or directory.
 * 
 * Example:
 * 
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 * 
 * Would print:
 * 
 *     /rare
 */
#define spdir_of(s) cf_path_directory_of(s)

/**
 * Returns the directory of a given file or directory.
 * 
 * Example:
 * 
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 * 
 * Would print:
 * 
 *     /data
 */
#define sptop_of(s) cf_path_top_directory(s)
#endif // CUTE_NO_SHORTHAND_API

CUTE_API char* CUTE_CALL cf_path_get_filename(const char* path);
CUTE_API char* CUTE_CALL cf_path_get_filename_no_ext(const char* path);
CUTE_API char* CUTE_CALL cf_path_get_ext(const char* path);
CUTE_API char* CUTE_CALL cf_path_pop(const char* path);
CUTE_API char* CUTE_CALL cf_path_compact(const char* path, int n);
CUTE_API char* CUTE_CALL cf_path_directory_of(const char* path);
CUTE_API char* CUTE_CALL cf_path_top_directory(const char* path);

//--------------------------------------------------------------------------------------------------
// Virtual file system.

/**
 * Cute Framework (CF) uses a virtual file system. This has a bunch of benefits over directly
 * accessing files. CF's file system is a wrap layer over PhysFS (https://icculus.org/physfs/).
 * 
 * - More safe + secure.
 * - More portable.
 * - More versatile.
 * 
 * Your game gets one directory to write files, and no other. This prevents the game from
 * accessing any other areas of the disk, keeping things simple and secure. You can set the
 * write directory with `cf_fs_set_write_directory`. During development you can set this
 * directory to wherever you like, but when shipping your game it's highly recommended to
 * set the write directory as `cf_fs_get_user_directory`. This directory is guaranteed to be
 * a write-enabled and safe place to store game-specific files for your player.
 * 
 * File paths such as "./" and "../ are not allowed as they open up a variety of security
 * holes. Additionally, Windows-slashes "\\" or colons ":" are not allowed in file paths
 * either. The top level folder can start with or without a slash. For example, these are
 * both valid paths.
 * 
 *     "/content/images/tree.png"
 *     "content/images/tree.png"
 * 
 * Drive letters are entirely hidden. Instead the file system uses a virtual path. You must
 * add any folders you want to access onto the path. Whenever CF searches for a file, it
 * searches the path in alphabetical order and returns the first match found. To add a
 * directory to the path use `cf_fs_mount`. For example, let's say our game's content files
 * are laid out something like this:
 * 
 *     /content
 *     --/images
 *     --/sounds
 *     --/scripts
 * 
 * A good method is to mount the content folder as a blank string, like this:
 * 
 *     cf_fs_mount("/content", "", true);
 * 
 * From here on whenever we want to find something in the content folder, it will be treated
 * as the top-level directory to search from. We can open one of the images or sounds like so:
 * 
 *     cf_fs_open_file_for_read("/images/flower.png");
 *     cf_fs_open_file_for_read("/sounds/laugh.wav");
 * 
 * This grants a lot of flexibility. We can move entire directories around on disk and then
 * rename the mount point without changing the rest of the code.
 * 
 * Mounting can reference archive files. Here's a list of supported archive formats.
 * 
 *    .ZIP (pkZip/WinZip/Info-ZIP compatible)
 *    .7Z  (7zip archives)
 *    .ISO (ISO9660 files, CD-ROM images)
 *    .GRP (Build Engine groupfile archives)
 *    .PAK (Quake I/II archive format)
 *    .HOG (Descent I/II HOG file archives)
 *    .MVL (Descent II movielib archives)
 *    .WAD (DOOM engine archives)
 *    .VDF (Gothic I/II engine archives)
 *    .SLB (Independence War archives)
 * 
 * Whenever an archive is mounted the file system treats it like a normal directory. No extra
 * work is needed. This lets us do really cool things, like deploy patches by downloading
 * new archive files and appending them to an earlier place in the search path. This also works
 * to add mod support to your game, and provides a simple way of storing multiple versions of
 * a single file without overwriting each other on the actual disk.
 * 
 * By default CF mounts the base directory when you call `make_app`. This can be disabled by
 * passing the `APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to `make_app`.
 */

typedef struct CF_File CF_File;

#define CF_FILE_TYPE_DEFS \
	CF_ENUM(FILE_TYPE_REGULAR, 0) \
	CF_ENUM(FILE_TYPE_DIRECTORY, 1) \
	CF_ENUM(FILE_TYPE_SYMLINK, 2) \
	CF_ENUM(FILE_TYPE_OTHER, 3) \

typedef enum CF_FileType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_FILE_TYPE_DEFS
	#undef CF_ENUM
} CF_FileType;

typedef struct CF_Stat
{
	CF_FileType type;
	int is_read_only;
	size_t size;
	uint64_t last_modified_time;
	uint64_t created_time;
	uint64_t last_accessed_time;
} CF_Stat;

/**
 * Returns the path of the base directory. This is not a virtual path, but the actual OS-path
 * where the executable was run from. This might not be the working directory, but probably is.
 * You should probably mount the base directory with `cf_fs_mount`.
 */
CUTE_API const char* CUTE_CALL cf_fs_get_base_directory();

/**
 * Sets a path safe to store game-specific files, such as save data or profiles. The path is in
 * platform-dependent notation. It's highly recommend to use `cf_fs_get_user_directory` and pass
 * it into this function when shipping your game. This function will fail if any files are from
 * the write directory are currently open.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_set_write_directory(const char* platform_dependent_directory);

/**
 * Returns a path safe to store game-specific files, such as save data or profiles. The path is
 * in platform-dependent notation. The location of this folder varies depending on the OS. You
 * should probably pass this into `cf_fs_set_write_directory` as well as `cf_fs_mount`.
 *
 *     Windows example:
 *     "C:\\Users\\OS_user_name\\AppData\\Roaming\\my_company\\my_game"
 *
 *     Linux example:
 *     "/home/OS_user_name/.local/share/my_game"
 *
 *     MacOS X example:
 *     "/Users/OS_user_name/Library/Application Support/my_game"
 * 
 * You should assume this directory is the only safe place to write files.
 */
CUTE_API const char* CUTE_CALL cf_fs_get_user_directory(const char* company_name, const char* game_name);

/**
 * Adds a new archive/directory onto the search path.
 * 
 * archive        - Platform-dependent notation. The archive or directory to mount.
 * mount_point    - The new virtual path for `archive`.
 * append_to_path - If true `mount_point` is appended onto the end of the path. If false it
 *                  will be prepended.
 * 
 * Each individual archive can only be mounted once. Duplicate mount attempts will be ignored.
 * 
 * You can mount multiple archives onto a single mount point. This is a great way to support
 * modding or download patches, as duplicate entries will be searched for on the path as normal,
 * without the need to overwrite each other on the actual disk.
 * 
 * You can mount an actual directory or an archive file. If it's an archive the vitrual file
 * system will treat it like a normal directory for you. There are a variety of archive file
 * formats supported (see top of file).
 * 
 * By default CF mounts the base directory when you call `make_app`. This can be disabled by
 * passing the `APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to `make_app`.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_mount(const char* archive, const char* mount_point, bool append_to_path /*= true*/);

/**
 * Removes an archive from the path, specified in platform-dependent notation. This function
 * does not remove a `mount_point` from the virtual file system, but only the actual archive
 * that was previously mounted.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_dismount(const char* archive);

/**
 * Fetches file information at the given virtual path, such as file size or creation time.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_stat(const char* virtual_path, CF_Stat* stat);

/**
 * Opens a file for writing relative to the write directory.
 * The write directory is specified by you when calling `cf_fs_set_write_directory`.
 */
CUTE_API CF_File* CUTE_CALL cf_fs_create_file(const char* virtual_path);

/**
 * Opens a file for writing relative to the write directory.
 * The write directory is specified by you when calling `cf_fs_set_write_directory`.
 */
CUTE_API CF_File* CUTE_CALL cf_fs_open_file_for_write(const char* virtual_path);

/**
 * Opens a file for appending relative to the write directory.
 * The write directory is specified by you when calling `cf_fs_set_write_directory`.
 */
CUTE_API CF_File* CUTE_CALL cf_fs_open_file_for_append(const char* virtual_path);

/**
 * Opens a file for reading. If you just want some basic information about the file (such as
 * it's size or when it was created), you can use `cf_fs_stat` instead.
 */
CUTE_API CF_File* CUTE_CALL cf_fs_open_file_for_read(const char* virtual_path);

/**
 * Close a file.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_close(CF_File* file);

/**
 * Deletes a file or directory. The directory must be empty, otherwise this function
 * will fail.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_delete(const char* virtual_path);

/**
 * Creates a directory at the path. All missing directories are also created.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_create_directory(const char* virtual_path);

/**
 * Returns a list of files and directories in the given directory. The list is sorted.
 * Results are collected by visiting the search path for all real directories mounted
 * on `virtual_path`. No duplicate file names will be reported. The list itself is
 * sorted alphabetically, though you can further sort it however you like. Free the list
 * up with `cf_fs_free_enumerated_directory` when done. The final element of the list
 * is NULL.
 * 
 * Example to loop over a list:
 * 
 *     const char** list = cf_fs_enumerate_directory("/data");
 *     for (const char** i = list; *i; ++i) {
 *         printf("Found %s\n", *i);
 *     }
 */
CUTE_API const char** CUTE_CALL cf_fs_enumerate_directory(const char* virtual_path);

/**
 * Frees a file list from `cf_fs_create_directory`.
 */
CUTE_API void CUTE_CALL cf_fs_free_enumerated_directory(const char** directory_list);

/**
 * Frees a file list from `cf_fs_create_directory`.
 */
CUTE_API bool CUTE_CALL cf_fs_file_exists(const char* virtual_path);

/**
 * Reads bytes from a file opened in read mode. The file must be opened in read mode
 * with `cf_fs_open_file_for_read`. Returns the number of bytes read. Returns -1 on failure.
 */
CUTE_API size_t CUTE_CALL cf_fs_read(CF_File* file, void* buffer, size_t bytes);

/**
 * Writes bytes from a file opened in write mode. The file must be opened in write mode
 * with `cf_fs_open_file_for_write`. Returns the number of bytes written. Returns -1 on failure.
 */
CUTE_API size_t CUTE_CALL cf_fs_write(CF_File* file, const void* buffer, size_t bytes);

/**
 * Check to see if the eof has been found after reading a file opened in read mode.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_eof(CF_File* file);

/**
 * Returns the current position within the file. This is an offset from the beginning of
 * the file. Returns -1 on failure.
 */
CUTE_API size_t CUTE_CALL cf_fs_tell(CF_File* file);

/**
 * Sets the current position within a file. This is an offset from the beginning of the file.
 * The next read or write will happen at this position.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_seek(CF_File* file, size_t position);

/**
 * Returns the size of a file in bytes. You might want to use `cf_fs_stat` instead.
 */
CUTE_API size_t CUTE_CALL cf_fs_size(CF_File* file);

/**
 * Reads an entire file into a buffer of memory, and returns the buffer to you. Call `CUTE_FREE`
 * on it when done.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size /*= NULL*/ /*= NULL*/);

/**
 * Reads an entire file into a buffer of memory, and returns the buffer to you as a nul-term-
 * inated C string. Call `CUTE_FREE` on it when done.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size /*= NULL*/ /*= NULL*/);

/**
 * Writes an entire buffer of data to a file as binary data.
 */
CUTE_API CF_Result CUTE_CALL cf_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);

/**
 * Feel free to call this whenever an error occurs in any of the file system functions
 * to try and get a detailed description on what might have happened. Often times this
 * string is already returned to you inside a `CF_Result`.
 */
CUTE_API const char* CUTE_CALL cf_fs_get_backend_specific_error_message();

/**
 * Converts a virtual path to an actual path. This can be useful for editors, asset
 * hotloading, or other similar development features. When shipping your game it's highly
 * recommended to not call this function at all, and only use it for development
 * purposes.
 * 
 * If the virtual path points to a completely fake directory this will return the first
 * archive found there.
 * 
 * This function can point to a directory, an archive, a file, or NULL if nothing
 * suitable was found at all.
 */
CUTE_API const char* CUTE_CALL cf_fs_get_actual_path(const char* virtual_path);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace cute
{

using Stat = CF_Stat;
using File = CF_File;

enum FileType : int
{
	#define CF_ENUM(K, V) K = V,
	CF_FILE_TYPE_DEFS
	#undef CF_ENUM
};

CUTE_INLINE const char* fs_get_base_dir() { return cf_fs_get_base_directory(); }
CUTE_INLINE Result fs_set_write_dir(const char* platform_dependent_directory) { return cf_fs_set_write_directory(platform_dependent_directory); }
CUTE_INLINE Result fs_mount(const char* archive, const char* mount_point, bool append_to_path = true) { return cf_fs_mount(archive, mount_point, append_to_path); }
CUTE_INLINE Result fs_dismount(const char* archive) { return cf_fs_dismount(archive); }
CUTE_INLINE Result fs_stat(const char* virtual_path, Stat* stat) { return cf_fs_stat(virtual_path, stat); }
CUTE_INLINE File* fs_create_file(const char* virtual_path) { return cf_fs_create_file(virtual_path); }
CUTE_INLINE File* fs_open_file_for_write(const char* virtual_path) { return cf_fs_open_file_for_write(virtual_path); }
CUTE_INLINE File* fs_open_file_for_append(const char* virtual_path) { return cf_fs_open_file_for_append(virtual_path); }
CUTE_INLINE File* fs_open_file_for_read(const char* virtual_path) { return cf_fs_open_file_for_read(virtual_path); }
CUTE_INLINE Result fs_close(File* file) { return cf_fs_close(file); }
CUTE_INLINE Result fs_delete(const char* virtual_path) { return cf_fs_delete(virtual_path); }
CUTE_INLINE Result fs_create_dir(const char* virtual_path) { return cf_fs_create_directory(virtual_path); }
CUTE_INLINE const char** fs_enumerate_directory(const char* virtual_path) { return cf_fs_enumerate_directory(virtual_path); }
CUTE_INLINE void fs_free_enumerated_directory(const char** directory_list) { cf_fs_free_enumerated_directory(directory_list); }
CUTE_INLINE bool fs_file_exists(const char* virtual_path) { return cf_fs_file_exists(virtual_path); }
CUTE_INLINE size_t fs_read(File* file, void* buffer, size_t bytes) { return cf_fs_read(file, buffer, bytes); }
CUTE_INLINE size_t fs_write(File* file, const void* buffer, size_t bytes) { return cf_fs_write(file, buffer, bytes); }
CUTE_INLINE Result fs_eof(File* file) { return cf_fs_eof(file); }
CUTE_INLINE size_t fs_tell(File* file) { return cf_fs_tell(file); }
CUTE_INLINE Result fs_seek(File* file, size_t position) { return cf_fs_seek(file, position); }
CUTE_INLINE size_t fs_size(File* file) { return cf_fs_size(file); }
CUTE_INLINE Result fs_read_entire_file_to_memory(const char* virtual_path, void** data_ptr, size_t* size = NULL) { return cf_fs_read_entire_file_to_memory(virtual_path, data_ptr, size); }
CUTE_INLINE Result fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, void** data_ptr, size_t* size = NULL) { return cf_fs_read_entire_file_to_memory_and_nul_terminate(virtual_path, data_ptr, size); }
CUTE_INLINE Result fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size) { return cf_fs_write_entire_buffer_to_file(virtual_path, data, size); }
CUTE_INLINE const char* fs_get_backend_specific_error_message() { return cf_fs_get_backend_specific_error_message(); }
CUTE_INLINE const char* fs_get_user_directory(const char* org, const char* app) { return cf_fs_get_user_directory(org, app); }
CUTE_INLINE const char* fs_get_actual_path(const char* virtual_path) { return cf_fs_get_actual_path(virtual_path); }

}

#endif // CUTE_CPP

#endif // CUTE_FILE_SYSTEM_H
