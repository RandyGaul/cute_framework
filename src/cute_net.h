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

struct cute_t;
struct connection_t;
struct endpoint_t;
struct crypto_key_t;

endpoint_t endpoint_make(uint32_t address, int16_t port);
connection_t* connection_make(endpoint_t endpoint, const crypto_key_t* endpoint_public_key);
void connection_destroy(connection_t* dst);

// TODO:
// Client connect to server
// Client poll the handshake
// Server accept incoming client
	// Make new connection
	// Server perform handshake on thread
// Server broadcast to all connections

// OPEN QUESTIONS:
// Should there be cute_net_server.h and cute_net_server.cpp?
// What about cute_net_client.h and cute_net_client.cpp?
// Probably not client.
// Preference: Use cute_net.h for clients trivially, and create extra wrapper for server to listen + spawn many connections.
// Note: Server-ish code can probably go into utils header.

void send_data(cute_t* cute, connection_t* dst, void* data, int data_byte_count, int channel);
void send_data_unreliable(cute_t* cute, connection_t* dst, void* data, int data_byte_count);

}

#include <cute_net_utils.h>

#endif // CUTE_NET_H
