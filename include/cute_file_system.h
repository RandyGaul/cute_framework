/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_FILE_SYSTEM_H
#define CF_FILE_SYSTEM_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_string.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// Path helper functions.
// These are implemented with the C-string API in "cute_string.h"; each function returns a fully
// mutable string you must free up with `sfree` or `cf_string_free` when done.

/**
 * @function spfname
 * @category path
 * @brief    Returns the filename portion of a path. Returns a new string.
 * @param    s          The path string.
 * @example  > Example fetching a filename from a path.
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *     // Prints: big_gem.txt
 * @remarks  Call `sfree` on the return value when done. `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spfname(s) cf_path_get_filename(s)

/**
 * @function spfname_no_ext
 * @category path
 * @brief    Returns the filename portion of a path without the file extension. Returns a new string.
 * @param    s          The path string.
 * @example  > Example fetching a filename from a path without the extension attached.
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *     // Prints: big_gem
 * @remarks  Call `sfree` on the return value when done. `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spfname_no_ext(s) cf_path_get_filename_no_ext(s)

/**
 * @function spext
 * @category path
 * @brief    Returns the extension of the file for the given path. Returns a new string.
 * @param    s          The path string.
 * @example  > Example fetching a filename from a path without the extension attached.
 *     const char* ext = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", ext);
 *     // Prints: .txt
 * @remarks  Call `sfree` on the return value when done. `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spext(s) cf_path_get_ext(s)

/**
 * @function spext_equ
 * @category path
 * @brief    Returns true if the file's extension matches, false otherwise.
 * @param    s          The path string.
 * @param    ext        The file extension.
 * @remarks  `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spext_equ(s, ext) cf_path_ext_equ(s, ext)

/**
 * @function sppop
 * @category path
 * @brief    Removes the rightmost file or directory from the path.
 * @param    s          The path string.
 * @return   If the string is not a dynamic string from CF's string API, a new string is returned. Otherwise the
 *           string is modified in-place. You must call `sfree` if a new dynamic string is returned, when done.
 * @remarks  `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define sppop(s) cf_path_pop(s)

/**
 * @function sppopn
 * @category path
 * @brief    Removes the rightmost n files or directories from the path.
 * @param    s          The path string.
 * @param    n          The number of files to pop from the directory path.
 * @return   If the string is not a dynamic string from CF's string API, a new string is returned. Otherwise the
 *           string is modified in-place. You must call `sfree` if a new dynamic string is returned, when done.
 * @remarks  `sp` stands for "sting path".
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define sppopn(s, n) cf_path_pop_n(s, n)

/**
 * @function spcompact
 * @category path
 * @brief    Squishes the path to be less than or equal to n characters in length.
 * @param    s          The path string.
 * @param    n          The number of files to pop from the directory path.
 * @return   If the string is not a dynamic string from CF's string API, a new string is returned. Otherwise the
 *           string is modified in-place. You must call `sfree` if a new dynamic string is returned, when done.
 * @remarks  This will insert ellipses "..." into the path as necessary. This function is useful for displaying paths
 *           and visualizing them in small boxes or windows. n includes the nul-byte. Returns a new string.
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spcompact(s, n) cf_path_compact(s, n)

/**
 * @function spdir_of
 * @category path
 * @brief    Returns the directory of a given file or directory. Returns a new string.
 * @param    s          The path string.
 * @example  > Example fetching a directory a file sits within.
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *     // Prints: /rare
 * @remarks  `sp` stands for "sting path". Call `sfree` on the return value when done.
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spdir_of(s) cf_path_directory_of(s)

/**
 * @function sptop_of
 * @category path
 * @brief    Returns the top-level directory of a given file or directory. Returns a new string.
 * @param    s          The path string.
 * @example  > Example fetching a the top-level directory a file sits within.
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *     // Prints: /data
 * @remarks  `sp` stands for "sting path". Call `sfree` on the return value when done.
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define sptop_of(s) cf_path_top_directory(s)

/**
 * @function spnorm
 * @category path
 * @brief    Normalizes a path as a new string.
 * @param    s          The path string.
 * @remarks  All '\\' are replaced with '/'. Any duplicate '////' are replaced with a single '/'. Trailing '/' are removed. Dot folders are resolved, e.g.
 *           ```
 *           spnorm("/a/b/./c") -> "/a/b/c"
 *           spnorm("/a/b/../c") -> "/a/c"
 *           ```
 *           The first character is always '/', unless it's a windows drive, e.g.
 *           ```
 *           spnorm("C:\\Users\\Randy\\Documents") -> "C:/Users/Randy/Documents"
 *           ```
 * @related  spfname spfname_no_ext spext spext_equ sppop sppopn spcompact spdir_of sptop_of spnorm
 */
#define spnorm(s) cf_path_normalize(s)

CF_API char* CF_CALL cf_path_get_filename(const char* path);
CF_API char* CF_CALL cf_path_get_filename_no_ext(const char* path);
CF_API char* CF_CALL cf_path_get_ext(const char* path);
CF_API bool CF_CALL cf_path_ext_equ(const char* path, const char* ext);
CF_API char* CF_CALL cf_path_pop(const char* path);
CF_API char* CF_CALL cf_path_pop_n(const char* path, int n);
CF_API char* CF_CALL cf_path_compact(const char* path, int n);
CF_API char* CF_CALL cf_path_directory_of(const char* path);
CF_API char* CF_CALL cf_path_top_directory(const char* path);
CF_API char* CF_CALL cf_path_normalize(const char* path);

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

/**
 * @struct   CF_File
 * @category file
 * @brief    An opaque pointer for representing a file.
 * @remarks  [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system)
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
typedef struct CF_File CF_File;
// @end

/**
 * @enum     File Types
 * @category file
 * @brief    The various kinds of files that can be opened.
 * @related  CF_File CF_Stat cf_file_type_to_string
 */
#define CF_FILE_TYPE_DEFS \
	/* @entry A reguler file, such as a .txt or .pdf file. */ \
	CF_ENUM(FILE_TYPE_REGULAR, 0)                             \
	/* @entry A directory/folder. */                          \
	CF_ENUM(FILE_TYPE_DIRECTORY, 1)                           \
	/* @entry A symlink. Symlinks are not supported. */       \
	CF_ENUM(FILE_TYPE_SYMLINK, 2)                             \
	/* @entry An unknown file type. */                        \
	CF_ENUM(FILE_TYPE_OTHER, 3)                               \
// @end

typedef enum CF_FileType
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_FILE_TYPE_DEFS
	#undef CF_ENUM
} CF_FileType;

/**
 * @function cf_file_type_to_string
 * @category file
 * @brief    Returns a `CF_FileType` converted to a c-string.
 * @related  CF_File CF_Stat cf_file_type_to_string
 */
CF_INLINE const char* cf_file_type_to_string(CF_FileType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_FILE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @struct   CF_Stat
 * @category file
 * @brief    A structure containing information about a file.
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
typedef struct CF_Stat
{
	/* @member The type of the file. */
	CF_FileType type;

	/* @member True if the file is read-only, false otherwise. */
	bool is_read_only;

	/* @member The size of the file in bytes. */
	size_t size;

	/* @member The last time the file was written to. */
	uint64_t last_modified_time;

	/* @member The time of file creation. */
	uint64_t created_time;

	/* @member The last time this file was accessed (not necessarily modified). */
	uint64_t last_accessed_time;
} CF_Stat;
// @end

/**
 * @function cf_fs_get_base_directory
 * @category file
 * @brief    Returns the path of the base directory.
 * @remarks  This is not a virtual path, but the actual OS-path where the executable was run from. This might not be the working directory,
 *           but probably is. You should probably mount the base directory with `cf_fs_mount`. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system) for an overview.
 * @related  cf_fs_get_base_directory cf_fs_set_write_directory cf_fs_get_user_directory cf_fs_mount cf_fs_dismount
 */
CF_API const char* CF_CALL cf_fs_get_base_directory();

/**
 * @function cf_fs_set_write_directory
 * @category file
 * @brief    Sets a path safe to store game-specific files, such as save data or profiles.
 * @param    platform_dependent_directory  The write directory in platform-dependent notation (use `cf_fs_get_user_directory`, see remarks for
             more details).
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  The path is in platform-dependent notation. It's highly recommend to use `cf_fs_get_user_directory` and pass it into this function
 *           when shipping your game. This function will fail if any files are from the write directory are currently open.
 *           See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system) for an overview.
 * @related  cf_fs_get_base_directory cf_fs_set_write_directory cf_fs_get_user_directory cf_fs_mount cf_fs_dismount
 */
CF_API CF_Result CF_CALL cf_fs_set_write_directory(const char* platform_dependent_directory);

/**
 * @function cf_fs_get_user_directory
 * @category file
 * @brief    Returns a path safe to store game-specific files, such as save data or profiles.
 * @param    company_name    The name of your org or company.
 * @param    game_name       The name of your game.
 * @remarks  The path is in platform-dependent notation. The location of this folder varies depending on the OS. You
 *           should probably pass this into `cf_fs_set_write_directory` as well as `cf_fs_mount`. Windows example:
 *           ```
 *           "C:\\Users\\OS_user_name\\AppData\\Roaming\\my_company\\my_game"
 *           ```
 *           Linux example:
 *           ```
 *           "/home/OS_user_name/.local/share/my_game"
 *           ```
 *           MacOS X example:
 *           ```
 *           "/Users/OS_user_name/Library/Application Support/my_game"
 *           ```
 *           You should assume this directory is the only safe place to write files. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_get_base_directory cf_fs_set_write_directory cf_fs_get_user_directory cf_fs_mount cf_fs_dismount
 */
CF_API const char* CF_CALL cf_fs_get_user_directory(const char* company_name, const char* game_name);

/**
 * @function cf_fs_mount
 * @category file
 * @brief    Adds a new archive/directory onto the search path.
 * @param    archive         Platform-dependent notation. The archive or directory to mount.
 * @param    mount_point     The new virtual path for `archive`.
 * @param    append_to_path  If true `mount_point` is appended onto the end of the path. If false it will be prepended.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  Each individual archive can only be mounted once. Duplicate mount attempts will be ignored.
 *           
 *           You can mount multiple archives onto a single mount point. This is a great way to support
 *           modding or download patches, as duplicate entries will be searched for on the path as normal,
 *           without the need to overwrite each other on the actual disk.
 *           
 *           You can mount an actual directory or an archive file. If it's an archive the vitrual file
 *           system will treat it like a normal directory for you. There are a variety of archive file
 *           formats supported (see top of file).
 *           
 *           By default CF mounts the base directory when you call `cf_make_app`. This can be disabled by
 *           passing the `CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to `cf_make_app`. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_get_base_directory cf_fs_set_write_directory cf_fs_get_user_directory cf_fs_mount cf_fs_dismount
 */
CF_API CF_Result CF_CALL cf_fs_mount(const char* archive, const char* mount_point, bool append_to_path);

/**
 * @function cf_fs_dismount
 * @category file
 * @brief    Removes an archive from the path specified in platform-dependent notation.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  This function does not remove a `mount_point` from the virtual file system, but only the actual archive that was previously mounted. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_get_base_directory cf_fs_set_write_directory cf_fs_get_user_directory cf_fs_mount cf_fs_dismount
 */
CF_API CF_Result CF_CALL cf_fs_dismount(const char* archive);

/**
 * @function cf_fs_stat
 * @category file
 * @brief    Returns file information at the given virtual path, such as file size or creation time.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  This doesn't open the file itself, and is a fairly light-weight operation in comparison. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_Result CF_CALL cf_fs_stat(const char* virtual_path, CF_Stat* stat);

/**
 * @function cf_fs_create_file
 * @category file
 * @brief    Opens a file for writing relative to the write directory.
 * @return   Returns a `CF_File` pointer representing the file.
 * @remarks  The write directory is specified by you when calling `cf_fs_set_write_directory`. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_File* CF_CALL cf_fs_create_file(const char* virtual_path);

/**
 * @function cf_fs_open_file_for_write
 * @category file
 * @brief    Opens a file for writing relative to the write directory.
 * @return   Returns a `CF_File` pointer representing the file.
 * @remarks  The write directory is specified by you when calling `cf_fs_set_write_directory`. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_File* CF_CALL cf_fs_open_file_for_write(const char* virtual_path);

/**
 * @function cf_fs_open_file_for_append
 * @category file
 * @brief    Opens a file for appending relative to the write directory.
 * @return   Returns a `CF_File` pointer representing the file.
 * @remarks  The write directory is specified by you when calling `cf_fs_set_write_directory`. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_File* CF_CALL cf_fs_open_file_for_append(const char* virtual_path);

/**
 * @function cf_fs_open_file_for_read
 * @category file
 * @brief    Opens a file for reading.
 * @param    virtual_path  The virtual path to the file.
 * @return   Returns a `CF_File` pointer representing the file.
 * @remarks  If you just want some basic information about the file (such as it's size or when it was created), you can use `cf_fs_stat` instead. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_File* CF_CALL cf_fs_open_file_for_read(const char* virtual_path);

/**
 * @function cf_fs_close
 * @category file
 * @brief    Closes a file.
 * @param    file        The file.
 * @return   Returns any errors as a `CF_Result`.
 * @related  CF_File CF_Stat cf_fs_create_file cf_fs_open_file_for_write cf_fs_open_file_for_read cf_fs_close
 */
CF_API CF_Result CF_CALL cf_fs_close(CF_File* file);

/**
 * @function cf_fs_remove
 * @category file
 * @brief    Removes a file or directory.
 * @param    virtual_path  The virtual path to the file or directory.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_remove_directory cf_fs_create_directory cf_fs_enumerate_directory cf_fs_free_enumerated_directory
 */
CF_API CF_Result CF_CALL cf_fs_remove(const char* virtual_path);

/**
 * @function cf_fs_create_directory
 * @category file
 * @brief    Creates a directory at the path.
 * @param    virtual_path  The virtual path to the directory.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  All missing directories are also created. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_remove_directory cf_fs_create_directory cf_fs_enumerate_directory cf_fs_free_enumerated_directory
 */
CF_API CF_Result CF_CALL cf_fs_create_directory(const char* virtual_path);

/**
 * @function cf_fs_enumerate_directory
 * @category file
 * @brief    Returns a sorted list of files and directories in the given directory.
 * @param    virtual_path  The virtual path to the directory.
 * @return   Returns any errors as a `CF_Result`.
 * @example > Loop over a list of all files in a directory.
 *     const char** list = cf_fs_enumerate_directory("/data");
 *     for (const char** i = list; *i; ++i) {
 *         printf("Found %s\n", *i);
 *     }
 *     cf_fs_free_enumerated_directory(list);
 * @remarks  Results are collected by visiting the search path for all real directories mounted on `virtual_path`. No duplicate file
 *           names will be reported. The list itself is sorted alphabetically, though you can further sort it however you like. Free
 *           the list up with `cf_fs_free_enumerated_directory` when done. The final element of the list is NULL. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_remove_directory cf_fs_create_directory cf_fs_enumerate_directory cf_fs_free_enumerated_directory
 */
CF_API const char** CF_CALL cf_fs_enumerate_directory(const char* virtual_path);

/**
 * @function cf_fs_free_enumerated_directory
 * @category file
 * @brief    Frees a file list from `cf_fs_create_directory`.
 * @param    directory_list  The directory list returned from `cf_fs_create_directory`.
 * @related  cf_fs_remove_directory cf_fs_create_directory cf_fs_enumerate_directory cf_fs_free_enumerated_directory
 */
CF_API void CF_CALL cf_fs_free_enumerated_directory(const char** directory_list);

/**
 * @function cf_fs_file_exists
 * @category file
 * @brief    Returns true if a file exists, false otherwise.
 * @param    virtual_path  A path to the file.
 * @remarks  [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API bool CF_CALL cf_fs_file_exists(const char* virtual_path);

/**
 * @function cf_fs_read
 * @category file
 * @brief    Reads bytes from a file opened in read mode.
 * @param    CF_File    The file.
 * @param    buffer     Pointer to a buffer of bytes.
 * @param    size       The size in bytes of `buffer`.
 * @remarks  The file must be opened in read mode with `cf_fs_open_file_for_read`. Returns the number of bytes read. Returns -1 on
 *           failure. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API size_t CF_CALL cf_fs_read(CF_File* file, void* buffer, size_t size);

/**
 * @function cf_fs_write
 * @category file
 * @brief    Writes bytes from a file opened in write mode.
 * @param    CF_File    The file.
 * @param    buffer     Pointer to a buffer of bytes.
 * @param    size       The size in bytes of `buffer`.
 * @remarks  The file must be opened in write mode with `cf_fs_open_file_for_write`. Returns the number of bytes written. Returns -1 on
 *           failure. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API size_t CF_CALL cf_fs_write(CF_File* file, const void* buffer, size_t size);

/**
 * @function cf_fs_eof
 * @category file
 * @brief    Check to see if the eof has been found after reading a file opened in read mode.
 * @param    CF_File    The file.
 * @remarks  [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API CF_Result CF_CALL cf_fs_eof(CF_File* file);

/**
 * @function cf_fs_tell
 * @category file
 * @brief    Returns the current position within the file.
 * @param    CF_File    The file.
 * @remarks  This is an offset from the beginning of the file. Returns -1 on failure. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API size_t CF_CALL cf_fs_tell(CF_File* file);

/**
 * @function cf_fs_seek
 * @category file
 * @brief    Sets the current position within a file.
 * @param    CF_File    The file.
 * @param    position   The read/write position.
 * @return   Returns any errors as a `CF_Result`.
 * @remarks  This is an offset from the beginning of the file. The next read or write will happen at this position. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API CF_Result CF_CALL cf_fs_seek(CF_File* file, size_t position);

/**
 * @function cf_fs_size
 * @category file
 * @brief    Returns the size of a file in bytes.
 * @param    CF_File    The file.
 * @remarks  You might want to use `cf_fs_stat` instead to avoid needing to fully open the file first. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  CF_File cf_fs_file_exists cf_fs_read cf_fs_write cf_fs_eof cf_fs_tell cf_fs_seek cf_fs_size
 */
CF_API size_t CF_CALL cf_fs_size(CF_File* file);

/**
 * @function cf_fs_read_entire_file_to_memory
 * @category file
 * @brief    Reads an entire file into a buffer of memory and returns it.
 * @param    virtual_path  A path to the file.
 * @param    size          If the file exists the size of the file is stored here.
 * @remarks  Call `cf_free` on it when done. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file cf_fs_write_string_file
 */
CF_API void* CF_CALL cf_fs_read_entire_file_to_memory(const char* virtual_path, size_t* size);

/**
 * @function cf_fs_read_entire_file_to_memory_and_nul_terminate
 * @category file
 * @brief    Reads an entire file into a buffer of memory and returns it as a nul-terminated C string.
 * @param    virtual_path  A path to the file.
 * @param    size          If the file exists the size of the file is stored here.
 * @remarks  Call `cf_free` on it when done. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file cf_fs_write_string_file
 */
CF_API char* CF_CALL cf_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size);

/**
 * @function cf_fs_write_entire_buffer_to_file
 * @category file
 * @brief    Writes an entire buffer of data to a file as binary data.
 * @param    virtual_path  A path to the file.
 * @param    data          A pointer to the data to write to the file.
 * @param    size          The size in bytes of `data`.
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file cf_fs_write_string_file cf_fs_write_string_range_to_file
 */
CF_API CF_Result CF_CALL cf_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);

/**
 * @function cf_fs_write_string_to_file
 * @category file
 * @brief    Writes a string to a file.
 * @param    virtual_path  A path to the file.
 * @param    string        A string to write.
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file cf_fs_write_string_file cf_fs_write_string_range_to_file
 */
CF_API CF_Result CF_CALL cf_fs_write_string_to_file(const char* virtual_path, const char* string);

/**
 * @function cf_fs_write_string_range_to_file
 * @category file
 * @brief    Writes a string to a file.
 * @param    virtual_path  A path to the file.
 * @param    begin         Beginning of the string.
 * @param    end           Pointer to one passed the end of the string's contents.
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file cf_fs_write_string_file cf_fs_write_string_range_to_file
 */
CF_API CF_Result CF_CALL cf_fs_write_string_range_to_file(const char* virtual_path, const char* begin, const char* end);

/**
 * @function cf_fs_get_backend_specific_error_message
 * @category file
 * @brief    Returns a backend specific error message in the case of any file system errors.
 * @remarks  Feel free to call this whenever an error occurs in any of the file system functions to try and get a detailed description
 *           on what might have happened. Often times this string is already returned to you inside a `CF_Result`.
 */
CF_API const char* CF_CALL cf_fs_get_backend_specific_error_message();

/**
 * @function cf_fs_get_actual_path
 * @category file
 * @brief    Converts a virtual path to an actual path.
 * @param    virtual_path  A path to the file.
 * @remarks  This can be useful for editors, asset hotloading, or other similar development features. When shipping your game it's highly
 *           recommended to not call this function at all, and only use it for development purposes. If the virtual path points to a completely
 *           fake directory this will return the first archive found there. This function can return a directory, an archive, a file, or `NULL`
 *           if nothing suitable was found at all. [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_fs_read_entire_file_to_memory cf_fs_read_entire_file_to_memory_and_nul_terminate cf_fs_write_entire_buffer_to_file
 */
CF_API const char* CF_CALL cf_fs_get_actual_path(const char* virtual_path);

/**
 * @function cf_fs_init
 * @category file
 * @brief    Initializes the [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @param    argv0       The first command-line argument passed into your `main` function.
 * @remarks  This function is automatically called by `cf_app_make`; for most use cases you do not
 *           need to call this function. However, sometimes it's convenient to make tools that crawl
 *           over files without the need for a full application window. In this case simply call this
 *           function to enable all the `cf_fs_***` functions.
 * @related  cf_fs_init cf_fs_destroy
 */
CF_API CF_Result CF_CALL cf_fs_init(const char* argv0);

/**
 * @function cf_fs_destroy
 * @category file
 * @brief    Destroys the [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @param    argv0       The first command-line argument passed into your `main` function.
 * @remarks  Cleans up all static memory used by `cf_fs_init`. You probably don't need to call this function,
 *           as `cf_app_destroy` already does this for you.
 * @related  cf_fs_init cf_fs_destroy
 */
CF_API void CF_CALL cf_fs_destroy();

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using Stat = CF_Stat;
using File = CF_File;

using FileType = CF_FileType;
#define CF_ENUM(K, V) CF_INLINE constexpr FileType K = CF_##K;
CF_FILE_TYPE_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(FileType type)
{
	switch (type) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_FILE_TYPE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE const char* fs_get_base_directory() { return cf_fs_get_base_directory(); }
CF_INLINE Result fs_set_write_directory(const char* platform_dependent_directory) { return cf_fs_set_write_directory(platform_dependent_directory); }
CF_INLINE Result fs_mount(const char* archive, const char* mount_point, bool append_to_path = true) { return cf_fs_mount(archive, mount_point, append_to_path); }
CF_INLINE Result fs_dismount(const char* archive) { return cf_fs_dismount(archive); }
CF_INLINE Result fs_stat(const char* virtual_path, Stat* stat) { return cf_fs_stat(virtual_path, stat); }
CF_INLINE File* fs_create_file(const char* virtual_path) { return cf_fs_create_file(virtual_path); }
CF_INLINE File* fs_open_file_for_write(const char* virtual_path) { return cf_fs_open_file_for_write(virtual_path); }
CF_INLINE File* fs_open_file_for_append(const char* virtual_path) { return cf_fs_open_file_for_append(virtual_path); }
CF_INLINE File* fs_open_file_for_read(const char* virtual_path) { return cf_fs_open_file_for_read(virtual_path); }
CF_INLINE Result fs_close(File* file) { return cf_fs_close(file); }
CF_INLINE Result fs_remove(const char* virtual_path) { return cf_fs_remove(virtual_path); }
CF_INLINE Result fs_create_directory(const char* virtual_path) { return cf_fs_create_directory(virtual_path); }
CF_INLINE const char** fs_enumerate_directory(const char* virtual_path) { return cf_fs_enumerate_directory(virtual_path); }
CF_INLINE void fs_free_enumerated_directory(const char** directory_list) { cf_fs_free_enumerated_directory(directory_list); }
CF_INLINE bool fs_file_exists(const char* virtual_path) { return cf_fs_file_exists(virtual_path); }
CF_INLINE size_t fs_read(File* file, void* buffer, size_t size) { return cf_fs_read(file, buffer, size); }
CF_INLINE size_t fs_write(File* file, const void* buffer, size_t size) { return cf_fs_write(file, buffer, size); }
CF_INLINE Result fs_eof(File* file) { return cf_fs_eof(file); }
CF_INLINE size_t fs_tell(File* file) { return cf_fs_tell(file); }
CF_INLINE Result fs_seek(File* file, size_t position) { return cf_fs_seek(file, position); }
CF_INLINE size_t fs_size(File* file) { return cf_fs_size(file); }
CF_INLINE void* fs_read_entire_file_to_memory(const char* virtual_path, size_t* size = NULL) { return cf_fs_read_entire_file_to_memory(virtual_path, size); }
CF_INLINE char* fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size = NULL) { return cf_fs_read_entire_file_to_memory_and_nul_terminate(virtual_path, size); }
CF_INLINE Result fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size) { return cf_fs_write_entire_buffer_to_file(virtual_path, data, size); }
CF_INLINE const char* fs_get_backend_specific_error_message() { return cf_fs_get_backend_specific_error_message(); }
CF_INLINE const char* fs_get_user_directory(const char* org, const char* app) { return cf_fs_get_user_directory(org, app); }
CF_INLINE const char* fs_get_actual_path(const char* virtual_path) { return cf_fs_get_actual_path(virtual_path); }

struct Path
{
	CF_INLINE Path() { }
	CF_INLINE Path(const char* s) { sset(m_path, s); }
	CF_INLINE Path(const Path& p) { *this = p; }
	CF_INLINE Path(Path&& p) { *this = p; }
	CF_INLINE ~Path() { sfree(m_path); m_path = NULL; }

	static CF_INLINE Path steal_from(const char* cute_c_api_string) { CF_ACANARY(cute_c_api_string); Path p; p.m_path = (char*)cute_c_api_string; return p; }

	CF_INLINE String filename() const { return String::steal_from(spfname(m_path)); }
	CF_INLINE String filename_no_ext() const { return String::steal_from(spfname_no_ext(m_path)); }
	CF_INLINE String ext() const { return String::steal_from(spext(m_path)); }
	CF_INLINE bool has_ext(const char* ext) const { return spext_equ(m_path, ext); }
	CF_INLINE void pop() { sppop(m_path); }
	CF_INLINE void pop(int n) { sppopn(m_path, n); }
	CF_INLINE void popn(int n) { sppopn(m_path, n); }
	CF_INLINE Path compact(int n) const { return Path::steal_from(spcompact(m_path, n)); }
	CF_INLINE Path my_directory() const { return Path::steal_from(spdir_of(m_path)); }
	CF_INLINE Path my_top() const { return Path::steal_from(sptop_of(m_path)); }
	CF_INLINE Path& normalize() { char* result = spnorm(m_path); sfree(m_path); m_path = result; return *this; }
	CF_INLINE Path normalized() const { return Path::steal_from(spnorm(m_path)); }
	CF_INLINE bool is_directory() const { Stat s; fs_stat(m_path, &s); return s.type == FILE_TYPE_DIRECTORY; }
	CF_INLINE bool is_file() const { Stat s; fs_stat(m_path, &s); return s.type == FILE_TYPE_REGULAR; }
	static CF_INLINE bool is_directory(const char* path) { Stat s; fs_stat(path, &s); return s.type == FILE_TYPE_DIRECTORY; }
	static CF_INLINE bool is_file(const char* path) { Stat s; fs_stat(path, &s); return s.type == FILE_TYPE_REGULAR; }

	CF_INLINE Path& add(const char* path) { if (sfirst(path) != '/' && slast(m_path) != '/') sappend(m_path, "/"); scat(m_path, path); return *this; }
	CF_INLINE Path& cat(const char* path) { return add(path); }
	CF_INLINE Path operator+(const Path& p) { Path result = *this; result.add(p.m_path); return result; }
	CF_INLINE Path& operator=(const Path& p) { sset(m_path, p.m_path); return *this; }
	CF_INLINE Path& operator+=(const Path& p) { *this = *this + p; return *this; }
	CF_INLINE Path& operator=(Path&& p) { m_path = p.m_path; p.m_path = NULL; return *this; }

	CF_INLINE const char* c_str() const { return m_path; }
	CF_INLINE operator char*() { return m_path; }
	CF_INLINE operator const char*() const { return m_path; }

private:
	char* m_path = NULL;
};

struct Directory
{
	CF_INLINE ~Directory() { if (m_dirs) fs_free_enumerated_directory(m_dirs); m_path = NULL; m_list = m_dirs = NULL; }
	CF_INLINE Directory(const Directory& d) { *this = d; }
	CF_INLINE Directory(Directory&& d) { *this = d; }

	static CF_INLINE Directory open(const char* virtual_path) { return Directory(virtual_path); }
	static CF_INLINE Result create(const char* virtual_path) { return fs_create_directory(virtual_path); }
	static CF_INLINE Result remove(const char* virtual_path) { return fs_remove(virtual_path); }
	static CF_INLINE Array<Path> enumerate(const char* virtual_path) {
		Array<Path> files;
		const char** paths = fs_enumerate_directory(virtual_path);
		const char** paths_ptr = paths;
		while (*paths) {
			const char* file = *paths++;
			files.add(file);
		}
		fs_free_enumerated_directory(paths_ptr);
		return files;
	}

	CF_INLINE bool has_next() { return *m_list ? true : false; }
	CF_INLINE const char* next() { if (*m_list) { const char* result = *m_list++; return result; } else { return NULL; } }

	CF_INLINE Directory& operator=(const Directory& d) { m_path = d.m_path; m_list = m_dirs = fs_enumerate_directory(m_path); return *this; }
	CF_INLINE Directory& operator=(Directory&& d) { m_path = d.m_path; m_list = m_dirs = d.m_dirs; d.m_path = NULL; d.m_dirs = d.m_list = NULL; return *this; }

private:
	const char* m_path;
	const char** m_dirs;
	const char** m_list;

	CF_INLINE Directory(const char* virtual_path) { m_path = virtual_path; m_list = m_dirs = fs_enumerate_directory(virtual_path); }
};

}

using CF_Path = Cute::Path;
using CF_Directory = Cute::Directory;

#endif // CF_CPP

#endif // CF_FILE_SYSTEM_H
