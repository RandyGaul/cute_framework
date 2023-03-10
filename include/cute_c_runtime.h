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

#ifndef CF_C_RUNTIME_H
#define CF_C_RUNTIME_H

#ifndef CF_ASSERT
#	include <assert.h>
#	ifdef _MSC_VER
#		define CF_ASSERT(...) (!(__VA_ARGS__) ? __debugbreak(), assert(__VA_ARGS__) : assert(__VA_ARGS__))
#	else
#		define CF_ASSERT assert
#	endif
#endif

#ifndef CF_MEMCPY
#	include <string.h>
#	define CF_MEMCPY memcpy
#endif

#ifndef CF_MEMMOVE
#	include <string.h>
#	define CF_MEMMOVE memmove
#endif

#ifndef CF_MEMSET
#	include <string.h>
#	define CF_MEMSET memset
#endif

#ifndef CF_STRCPY
#	include <string.h>
#	define CF_STRCPY strcpy
#endif

#ifndef CF_STRNCPY
#	include <string.h>
#	define CF_STRNCPY strncpy
#endif

#ifndef CF_STRLEN
#	include <string.h>
#	define CF_STRLEN strlen
#endif

#ifndef CF_MEMCHR
#	include <string.h>
#	define CF_MEMCHR memchr
#endif

#ifndef CF_STRSTR
#	include <string.h>
#	define CF_STRSTR strstr
#endif

#ifndef CF_MEMCMP
#	include <string.h>
#	define CF_MEMCMP memcmp
#endif

#ifndef CF_STRCMP
#	include <string.h>
#	define CF_STRCMP strcmp
#endif

#ifndef CF_STRICMP
#	include <string.h>
#	ifdef _WIN32
#		define CF_STRICMP stricmp
#	else
#		define CF_STRICMP strcasecmp
#	endif
#endif

#ifndef CF_STRDUP
#	include <string.h>
#	define CF_STRDUP strdup
#endif

#ifndef CF_STRNCMP
#	include <string.h>
#	define CF_STRNCMP strncmp
#endif

#ifndef CF_STRNICMP
#	include <string.h>
#	ifdef _WIN32
#		define CF_STRNICMP strnicmp
#	else
#		define CF_STRNICMP strncasecmp
#	endif
#endif

#ifndef CF_SNPRINTF
#	include <stdio.h>
#	define CF_SNPRINTF snprintf
#endif

#ifndef CF_STRTOLL
#	include <stdlib.h>
#	define CF_STRTOLL strtoll
#endif

#ifndef CF_STRTOD
#	include <stdlib.h>
#	define CF_STRTOD strtod
#endif

#ifndef CF_TOLOWER
#	include <ctype.h>
#	define CF_TOLOWER tolower
#endif

#ifndef CF_TOUPPER
#	include <ctype.h>
#	define CF_TOUPPER toupper
#endif

#ifndef CF_STRCHR
#	include <string.h>
#	define CF_STRCHR strchr
#endif

#ifndef CF_STRRCHR
#	include <string.h>
#	define CF_STRRCHR strrchr
#endif

#endif // CF_C_RUNTIME_H
