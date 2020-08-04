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

#ifndef CUTE_FILE_INDEX_H
#define CUTE_FILE_INDEX_H

#include <cute_error.h>
#include <cute_array.h>
#include <cute_dictionary.h>

namespace cute
{

struct file_index_t;

extern CUTE_API file_index_t* CUTE_CALL file_index_make(void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL file_index_destroy(file_index_t* fi);

extern CUTE_API void CUTE_CALL file_index_add_file(file_index_t* fi, const char* path);
extern CUTE_API void CUTE_CALL file_index_search_directory(file_index_t* fi, const char* path, const char* ext);

extern CUTE_API const char** CUTE_CALL file_index_get_paths(file_index_t* fi, int* count = NULL);
extern CUTE_API void CUTE_CALL file_index_free_paths(file_index_t* fi, const char** paths);
extern CUTE_API error_t CUTE_CALL file_index_find(file_index_t* fi, const char* path, uint64_t* index);
}

#endif // CUTE_FILE_INDEX_H
