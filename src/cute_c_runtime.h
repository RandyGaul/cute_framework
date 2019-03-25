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

#ifndef CUTE_C_RUNTIME_H
#define CUTE_C_RUNTIME_H

#ifndef CUTE_ASSERT
#	include <assert.h>
#	define CUTE_ASSERT assert
#endif

#ifndef CUTE_MEMCPY
#	include <string.h>
#	define CUTE_MEMCPY memcpy
#endif

#ifndef CUTE_MEMSET
#	include <string.h>
#	define CUTE_MEMSET memset
#endif

#ifndef CUTE_STRCPY
#	include <string.h>
#	define CUTE_STRCPY strcpy
#endif

#ifndef CUTE_STRNCPY
#	include <string.h>
#	define CUTE_STRNCPY strncpy
#endif

#ifndef CUTE_STRLEN
#	include <string.h>
#	define CUTE_STRLEN strlen
#endif

#ifndef CUTE_MEMCMP
#	include <string.h>
#	define CUTE_MEMCMP memcmp
#endif

#ifndef CUTE_STRCMP
#	include <string.h>
#	define CUTE_STRCMP strcmp
#endif

#ifndef CUTE_SNPRINTF
#	include <stdio.h>
#	define CUTE_SNPRINTF snprintf
#endif

#endif // CUTE_C_RUNTIME_H
