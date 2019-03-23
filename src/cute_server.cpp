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
#include <cute_server.h>
#include <cute_alloc.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_net_internal.h>

namespace cute
{

struct server_t
{
	int running = 0;
	void* mem_ctx = NULL;
	endpoint_t endpoint;
	crypto_key_t public_key;
	crypto_key_t private_key;
};

server_t* server_alloc(void* user_allocator_context)
{
	server_t* server = (server_t*)CUTE_ALLOC(sizeof(server_t), user_allocator_context);
	CUTE_CHECK_POINTER(server);
	CUTE_PLACEMENT_NEW(server) server_t;
	server->mem_ctx = user_allocator_context;

cute_error:
	CUTE_FREE(server, user_allocator_context);
	return NULL;
}

void server_destroy(server_t* server)
{
	CUTE_FREE(server, server->mem_ctx);
}

int server_start(server_t* server, endpoint_t endpoint, const crypto_key_t* public_key, const crypto_key_t* private_key, const server_config_t* config)
{
	return -1;
}

void server_stop(server_t* server)
{
}

}
