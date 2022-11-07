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

#ifndef CUTE_ALLOC_H
#define CUTE_ALLOC_H

#if !defined(CUTE_ALLOC) && !defined(CUTE_FREE)
#	include <stdlib.h>
#	define CUTE_CALLOC(size) calloc(size, 1)
#	define CUTE_ALLOC(size) malloc(size)
#	define CUTE_FREE(ptr) free(ptr)
#	define CUTE_REALLOC(ptr, size) realloc(ptr, size)
#endif

#include "cute_defines.h" // for #define CUTE_CPP

#ifdef CUTE_CPP
	#ifdef _MSC_VER
	#	pragma warning(disable:4291)
	#endif

	enum cf_dummy_enum_t { CF_DUMMY_ENUM };
	inline void* operator new(size_t, cf_dummy_enum_t, void* ptr) { return ptr; }
	#define CUTE_PLACEMENT_NEW(ptr) new(CF_DUMMY_ENUM, ptr)
	#define CUTE_NEW(T) new(CF_DUMMY_ENUM, CUTE_ALLOC(sizeof(T))) T
#endif // CUTE_CPP

#endif // CUTE_ALLOC_H
