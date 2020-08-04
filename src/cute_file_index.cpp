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

#include <cute_file_index.h>
#include <cute_string.h>
#include <cute_alloc.h>
#include <cute_file_system.h>

#define CUTE_PATH_IMPLEMENTATION
#include <cute/cute_path.h>

namespace cute
{

struct file_index_t
{
	array<string_t> paths;
	dictionary<string_t, int> indices;
	void* mem_ctx;
};

file_index_t* file_index_make(void* user_allocator_context)
{
	file_index_t* fi = CUTE_NEW(file_index_t, user_allocator_context);
	fi->mem_ctx = user_allocator_context;
	return fi;
}

void file_index_destroy(file_index_t* fi)
{
	fi->~file_index_t();
	CUTE_FREE(fi, fi->mem_ctx);
}

void file_index_add_file(file_index_t* fi, const char* path)
{
	string_t s = string_t(path);
	int index = fi->paths.count();
	fi->paths.add(s);
	fi->indices.insert(s, index);
}

void file_index_search_directory(file_index_t* fi, const char* path, const char* ext)
{
	const char** paths = cute::file_system_enumerate_directory(path);
	int i = 0;
	while (paths[i]) {
		char found_ext[CUTE_PATH_MAX_EXT];
		path_pop_ext(paths[i], NULL, found_ext);
		if (!strcmp(ext, found_ext)) {
			char found_path[CUTE_PATH_MAX_PATH];
			path_concat(path, paths[i], found_path, CUTE_PATH_MAX_PATH);

			stat_t stat;
			error_t err = file_system_stat(found_path, &stat);
			if (err.is_error()) {
				++i;
				continue;
			}

			if (stat.type == FILE_TYPE_REGULAR) {
				file_index_add_file(fi, found_path);
			} else if (stat.type == FILE_TYPE_DIRECTORY) {
				file_index_search_directory(fi, found_path, ext);
			}
		}
		++i;
	}
	cute::file_system_free_enumerated_directory(paths);
}

const char** file_index_get_paths(file_index_t* fi, int* count)
{
	array<const char*> paths;
	for (int i = 0; i < fi->paths.count(); ++i) {
		paths.add(CUTE_STRDUP(fi->paths[i].c_str()));
	}

	const char** paths_out = (const char**)CUTE_ALLOC(sizeof(const char*) * (paths.count() + 1), fi->mem_ctx);

	for (int i = 0; i < paths.count(); ++i) {
		paths_out[i] = paths[i];
	}
	paths_out[paths.count()] = NULL;
	if (count) *count = paths.count();

	return paths_out;
}

void file_index_free_paths(file_index_t* fi, const char** paths)
{
	int i = 0;
	while (paths[i]) {
		free((void*)paths[i]);
		++i;
	}

	CUTE_FREE(paths, fi->mem_ctx);
}

error_t file_index_find(file_index_t* fi, const char* path, uint64_t* index)
{
	int i;
	error_t err = fi->indices.find(string_t(path), &i);
	*index = (uint64_t)i;
	return err;
}

}
