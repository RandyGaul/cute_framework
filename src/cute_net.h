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

#define CUTE_PACKET_SIZE_MAX (CUTE_MB + 256)

namespace cute
{

struct client_t;
struct endpoint_t;
struct crypto_key_t;

int endpoint_init(endpoint_t* endpoint, const char* address_and_port_string, const crypto_key_t* endpoint_public_key);
client_t* client_make(app_t* app, endpoint_t endpoint);
void client_destroy(client_t* client);

enum client_state_t : int
{
	CLIENT_STATE_CONNECTING,
	CLIENT_STATE_CONNECTED,
	CLIENT_STATE_DISCONNECTED,
};

client_state_t client_state_get(const client_t* client);
void client_update(client_t* client, float dt);

int client_send_data(client_t* client, const void* data, int data_byte_count);
int client_send_data_unreliable(client_t* client, const void* data, int data_byte_count);

}

#include <cute_net_utils.h>

#endif // CUTE_NET_H
