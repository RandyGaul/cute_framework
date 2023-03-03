/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#include "cute_defines.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void CF_SharedLibrary;

/**
 * @function cf_load_shared_library
 * @category utility
 * @brief    Loads a shared library from disk and returns a pointer to the library.
 * @param    path        Path to the shared library in platform-dependent notation.
 * @return   Returns `NULL` in the case of errors, and can be unloaded by calling `unload_shared_library`.
 * @remarks  Does not use the virtual file system (see TODO_LINK_VFS_TUTORIAL). Once loaded, individual functions can be loaded from the shared
 *           library be called `cf_load_function`.
 * @related  cf_load_shared_library cf_unload_shared_library cf_load_function
 */
CUTE_API CF_SharedLibrary* CUTE_CALL cf_load_shared_library(const char* path);

/**
 * @function cf_unload_shared_library
 * @category utility
 * @brief    Unloads a shared library previously loaded with `load_shared_library`.
 * @param    library      A library of functions from `load_shared_library`.
 * @related  cf_load_shared_library cf_unload_shared_library cf_load_function
 */
CUTE_API void cf_unload_shared_library(CF_SharedLibrary* library);

/**
 * @function cf_load_function
 * @category utility
 * @brief    Loads a function out of a shared library.
 * @param    library        A library of functions from `load_shared_library`.
 * @param    function_name  The name of the function.
 * @remarks  The function pointer is not valid after calling `unload_shared_library`. After obtaining the function pointer with `load_function`
 *           you must typecast it yourself. Returns `NULL` in the case of errors.
 * @related  cf_load_shared_library cf_unload_shared_library cf_load_function
 */
CUTE_API void* cf_load_function(CF_SharedLibrary* library, const char* function_name);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using shared_library_t = CF_SharedLibrary;

CUTE_INLINE shared_library_t* load_shared_library(const char* path) { return cf_load_shared_library(path); }
CUTE_INLINE void unload_shared_library(shared_library_t* library) { cf_unload_shared_library(library); }
CUTE_INLINE void* load_function(shared_library_t* library, const char* function_name) { return cf_load_function(library,function_name); }

}

#endif // CUTE_CPP
