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

#include <cute_datalibrary.h>
#include <cute_c_runtime.h>
#include <cute_log.h>
#include <cute_alloc.h>

#include <dl/dl.h>
#include <dl/dl_typelib.h>

#define CUTE_FILES_IMPLEMENTATION
#include <cute/cute_files.h>

namespace cute
{

typedef error_t (s_traverse_fn)(cf_file_t* file, void* udata);

static error_t s_traverse(const char* path, s_traverse_fn* cb, void* udata)
{
	cf_dir_t dir;
	cf_dir_open(&dir, path);

	while (dir.has_next)
	{
		cf_file_t file;
		cf_read_file(&dir, &file);

		if (file.is_dir && file.name[0] != '.') {
			char path2[CUTE_FILES_MAX_PATH];
			int n = cf_safe_strcpy(path2, path, 0, CUTE_FILES_MAX_PATH);
			n = cf_safe_strcpy(path2, "/", n - 1, CUTE_FILES_MAX_PATH);
			cf_safe_strcpy(path2, file.name, n -1, CUTE_FILES_MAX_PATH);
			s_traverse(path2, cb, udata);
		}

		if (file.is_reg) {
			error_t err = cb(&file, udata);
			if (err.is_error()) return err;
		}

		cf_dir_next(&dir);
	}

	cf_dir_close(&dir);

	return error_success();
}

// -------------------------------------------------------------------------------------------------

static void* s_read_file_to_memory(const char* path, int* size)
{
	void* data = 0;
	FILE* fp = fopen(path, "rb");
	int sz = 0;

	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		sz = (int)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		data = CUTE_ALLOC(sz, NULL);
		fread(data, sz, 1, fp);
		fclose(fp);
	}

	if (size) *size = sz;
	return data;
}

static error_t s_load_typelib(cf_file_t* file, void* udata)
{
	dl_ctx_t dl = (dl_ctx_t)udata;

	if (CUTE_STRCMP(cf_get_ext(file), ".tld")) {
		log(CUTE_LOG_LEVEL_WARNING, "Found file of format %s instead of .tld when running `cute::compile_typelibs`.", file->ext);
		return error_success();
	}

	int size;
	void* data = s_read_file_to_memory(file->path, &size);

	dl_error_t err = dl_context_load_txt_type_library(dl, (const char*)data, size);
	if (err != DL_ERROR_OK) {
		return error_failure("Failed to load typelib file (.tld) when running `cute::compile_typelibs`.");
	}

	CUTE_FREE(data, NULL);

	return error_success();
}

static error_t s_write_typelib_binary(dl_ctx_t dl, const char* out_path)
{
	size_t binary_size;
	dl_error_t dl_err = dl_context_write_type_library(dl, NULL, 0, &binary_size);
	if (dl_err != DL_ERROR_OK) {
		return error_failure("Unable to query Data Library typelib binary size.");
	}

	void* binary_data = CUTE_ALLOC(binary_size, NULL);
	CUTE_ASSERT(binary_data);

	size_t bytes_written;
	dl_err = dl_context_write_type_library(dl, (unsigned char*)binary_data, binary_size, &bytes_written);
	if (dl_err != DL_ERROR_OK || bytes_written != binary_size) {
		return error_failure("Unable to write typelib binary.");
	}

	char path[1024];
	sprintf(path, "%s/typelib_binary.bin", out_path);

	FILE* fp = fopen(path, "wb");
	if (!fp) {
		return error_failure("Unable to open \"out_path/typelib_binary.bin\".");
	}

	fwrite(binary_data, binary_size, 1, fp);
	fclose(fp);
	CUTE_FREE(binary_data, NULL);
}

static error_t s_write_typelib_c_header(dl_ctx_t dl, const char* out_path)
{
	size_t binary_size;
	dl_error_t dl_err = dl_context_write_type_library_c_header(dl, "cute_typelib.h", NULL, 0, &binary_size);
	if (dl_err != DL_ERROR_OK) {
		return error_failure("Unable to query Data Library typelib binary size.");
	}

	void* binary_data = CUTE_ALLOC(binary_size, NULL);
	CUTE_ASSERT(binary_data);

	size_t bytes_written;
	dl_err = dl_context_write_type_library_c_header(dl, "cute_typelib.h", (char*)binary_data, binary_size, &bytes_written);
	if (dl_err != DL_ERROR_OK || bytes_written != binary_size) {
		return error_failure("Unable to write typelib binary.");
	}

	char path[1024];
	sprintf(path, "%s/cute_typelib.h", out_path);

	FILE* fp = fopen(path, "wb");
	if (!fp) {
		return error_failure("Unable to open \"out_path/cute_typelib.h\".");
	}

	fwrite(binary_data, binary_size, 1, fp);
	fclose(fp);
	CUTE_FREE(binary_data, NULL);
}

// -------------------------------------------------------------------------------------------------

error_t compile_typelibs(const char* in_path, const char* out_path)
{
	dl_create_params_t dl_params;
	dl_ctx_t dl;

	DL_CREATE_PARAMS_SET_DEFAULT(dl_params);

	dl_error_t dl_err = dl_context_create(&dl, &dl_params);
	if (dl_err != DL_ERROR_OK) {
		return error_failure("Unable to create Data Library context.");
	}

	error_t err = s_traverse(in_path, s_load_typelib, dl);
	if (err.is_error()) {
		return err;
	}

	err = s_write_typelib_binary(dl, out_path);
	if (err.is_error()) return err;

	err = s_write_typelib_c_header(dl, out_path);
	if (err.is_error()) return err;

	dl_err = dl_context_destroy(dl);
	if (dl_err != DL_ERROR_OK) {
		return error_failure("Failed to destroy Data Library context.");
	}

	return error_success();
}

}
