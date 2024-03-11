/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifdef CF_DEBUG_PRINTF
#undef CF_DEBUG_PRINTF
#endif
#include <stdio.h>
#define CF_DEBUG_PRINTF(...) printf(__VA_ARGS__)
