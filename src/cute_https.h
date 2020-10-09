/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_defines.h>
#include <cute_error.h>

namespace cute
{

struct https_t;

CUTE_API https_t* CUTE_CALL https_make();
CUTE_API void https_destroy(https_t* https);

CUTE_API error_t CUTE_CALL https_connect(https_t* https, const char* host, const char* port, bool verify_cert);
CUTE_API void CUTE_CALL https_disconnect(https_t* https);

CUTE_API void CUTE_CALL https_get(https_t* https, const char* url);
CUTE_API const char* CUTE_CALL https_response(https_t* https);

enum https_status_t
{
	HTTPS_STATUS_NOT_CONNECTED,
	HTTPS_STATUS_PENDING,
	HTTPS_STATUS_COMPLETED,
	HTTPS_STATUS_FAILED,
};

CUTE_API https_status_t CUTE_CALL https_process(https_t* https);

}
