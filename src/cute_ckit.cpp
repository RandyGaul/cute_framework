/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_alloc.h>

#define CK_ALLOC(sz) cf_alloc(sz)
#define CK_REALLOC(p, sz) cf_realloc(p, sz)
#define CK_FREE(p) cf_free(p)
#define CKIT_IMPLEMENTATION
#include <cute/ckit.h>
