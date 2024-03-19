/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_ALLOC_INTERNAL_H
#define CF_ALLOC_INTERNAL_H

#include <cute_defines.h>
#include <cute_alloc.h>

#if !defined(CF_ALLOC) && !defined(CF_FREE)
#	define CF_CALLOC(size) cf_calloc(size, 1)
#	define CF_ALLOC(size) cf_alloc(size)
#	define CF_FREE(ptr) cf_free(ptr)
#	define CF_REALLOC(ptr, size) cf_realloc(ptr, size)
#endif

#endif // CF_ALLOC_INTERNAL_H
