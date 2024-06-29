/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
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
 * @remarks  Does not use the virtual file system. Once loaded, individual functions can be loaded from the shared
 *           library be called `cf_load_function`. See [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).
 * @related  cf_load_shared_library cf_unload_shared_library cf_load_function
 */
CF_API CF_SharedLibrary* CF_CALL cf_load_shared_library(const char* path);

/**
 * @function cf_unload_shared_library
 * @category utility
 * @brief    Unloads a shared library previously loaded with `load_shared_library`.
 * @param    library      A library of functions from `load_shared_library`.
 * @related  cf_load_shared_library cf_unload_shared_library cf_load_function
 */
CF_API void cf_unload_shared_library(CF_SharedLibrary* library);

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
CF_API void* cf_load_function(CF_SharedLibrary* library, const char* function_name);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using shared_library_t = CF_SharedLibrary;

CF_INLINE shared_library_t* load_shared_library(const char* path) { return cf_load_shared_library(path); }
CF_INLINE void unload_shared_library(shared_library_t* library) { cf_unload_shared_library(library); }
CF_INLINE void* load_function(shared_library_t* library, const char* function_name) { return cf_load_function(library,function_name); }

}

#endif // CF_CPP
