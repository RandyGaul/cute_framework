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

#ifndef CUTE_SERVER_H
#define CUTE_SERVER_H

#include "cute_defines.h"
#include "cute_error.h"
#include "cute_handle_table.h"

#define CUTE_SERVER_MAX_CLIENTS 32

namespace cute
{

struct server_t;

struct server_config_t
{
	int application_id;
	int max_incoming_bytes_per_second = 0;
	int max_outgoing_bytes_per_second = 0;
	uint32_t connection_timeout;
	crypto_sign_public_t public_key;
	crypto_sign_secret_t secret_key;
};

CUTE_API server_t* CUTE_CALL server_create(server_config_t* config = NULL, void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL server_destroy(server_t* server);

CUTE_API error_t CUTE_CALL server_start(server_t* server, const char* address_and_port, const crypto_sign_public_t* public_key, const crypto_sign_secret_t* secret_key);
CUTE_API void CUTE_CALL server_stop(server_t* server);

enum server_event_type_t
{
	SERVER_EVENT_TYPE_NEW_CONNECTION,
	SERVER_EVENT_TYPE_DISCONNECTED,
	SERVER_EVENT_TYPE_USER_PACKET,
};

struct server_event_t
{
	server_event_type_t type;
	union
	{
		struct
		{
			int client_index;
			endpoint_t endpoint;
		} new_connection;

		struct
		{
			int client_index;
		} disconnected;

		struct
		{
			int client_index;
			const void* data;
			int size;
		} user_packet;
	} u;
};

CUTE_API bool CUTE_CALL server_pop_event(server_t* server, server_event_t* event);

CUTE_API void CUTE_CALL server_update(server_t* server, float dt);
CUTE_API void CUTE_CALL server_disconnect_client(server_t* server, int client_index, bool send_notification_to_client = true);
CUTE_API void CUTE_CALL server_find_and_disconnect_timed_out_clients(server_t* server, float timeout);
CUTE_API void CUTE_CALL server_send(server_t* server, const void* packet, int size, int client_index, bool send_reliably);
CUTE_API void CUTE_CALL server_send_to_all_clients(server_t* server, const void* packet, int size, bool send_reliably);
CUTE_API void CUTE_CALL server_send_to_all_but_one_client(server_t* server, const void* packet, int size, int client_index, bool send_reliably);

CUTE_API float CUTE_CALL server_time_of_last_packet_recieved_from_client(server_t* server, handle_t client_id);

}

#endif // CUTE_SERVER_H
