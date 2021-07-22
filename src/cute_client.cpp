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

#include <cute_net.h>
#include <cute_client.h>
#include <cute_error.h>
#include <cute_c_runtime.h>
#include <cute_alloc.h>
#include <cute_crypto.h>
#include <cute_circular_buffer.h>
#include <cute_protocol.h>

#include <internal/cute_net_internal.h>
#include <internal/cute_app_internal.h>
#include <internal/cute_protocol_internal.h>

#include <time.h>

#define CUTE_CLIENT_SEND_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_CLIENT_RECEIVE_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_CLIENT_MAX_RECONNECT_TRIES 3

namespace cute
{

}
