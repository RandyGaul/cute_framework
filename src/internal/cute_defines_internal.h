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

#ifndef CUTE_DEFINES_INTERNAL_H
#define CUTE_DEFINES_INTERNAL_H

// TODO: Remove these.
#define CUTE_CHECK(x) do { if (x) goto cute_error; } while (0)
#define CUTE_CHECK_ASSERT(x) do { if (!(x)) goto cute_error; } while (0)
#define CUTE_CHECK_POINTER(x) do { if (!(x)) goto cute_error; } while (0)

#endif // CUTE_DEFINES_INTERNAL_H
