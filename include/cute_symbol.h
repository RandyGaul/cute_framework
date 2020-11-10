/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_defines.h>

namespace
{

using shared_library_t = void;

/**
 * Loads a shared library from disk and returns a pointer to the library.
 * Returns `NULL` in the case of errors, and can be unloaded by calling `unload_shared_library`.
 */
CUTE_API shared_library_t* CUTE_CALL load_shared_library(const char* path);

/**
 * Unloads a shared library previously loaded with `load_shared_library`.
 */
CUTE_API void unload_shared_library(shared_library_t* library);

/**
 * Loads a function pointer out of a shared library openened previously with `load_shared_library`.
 * The function pointer is not valid after calling `unload_shared_library`.
 * After obtaining the function pointer with `load_function` you must typecast it yourself.
 * Returns `NULL` in the case of errors.
 */
CUTE_API void* load_function(shared_library_t* library, const char* function_name);

}
