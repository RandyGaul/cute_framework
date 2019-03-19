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

#ifndef CUTE_NET_H
#define CUTE_NET_H

#include <cute_defines.h>

namespace cute
{

struct connection_t;
struct endpoint_t;
struct crypto_key_t;

int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string, const crypto_key_t* endpoint_public_key);
connection_t* connection_make(cute_t* cute, endpoint_t endpoint);
void connection_destroy(connection_t* dst);

enum connection_state_t : int
{
	CONNECTION_STATE_SENDING_HELLO,
	CONNECTION_STATE_SENDING_HELLO_RESPONSE,
	CONNECTION_STATE_SENDING_HELLO_OK,

	CONNECTION_STATE_CONNECTED,

	CONNECTION_STATE_HELLO_TIMED_OUT,
	CONNECTION_STATE_CONNECTION_DENIED,
	CONNECTION_STATE_DISCONNECTED,
	CONNECTION_STATE_TIMED_OUT,
};

connection_state_t connection_state_get(connection_t* connection);

int send_data(connection_t* dst, void* data, int data_byte_count);
int send_data_unreliable(connection_t* dst, void* data, int data_byte_count);

}

#include <cute_net_utils.h>

#endif // CUTE_NET_H
