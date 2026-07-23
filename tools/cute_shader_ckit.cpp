/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info

	ckit implementation TU for standalone shader tools (cute-shaderc). CF proper
	provides the implementation via src/cute_ckit.cpp instead, so this file must
	only be linked into executables that do not link the cute library.
*/

#include <cute_alloc.h>

#define CKIT_IMPLEMENTATION
#include "cute/ckit.h"
