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

#include <cute_defines.h>
#include <cute_net.h>
#include <cute_crypto.h>
#include <cute_handle_table.h>

#define CUTE_SERVER_MAX_CLIENTS 256

namespace cute
{

struct server_t;
struct server_event_t;

struct server_config_t
{
	int max_clients = 64;
	float client_timeout_time = 20.0f;
	int max_incoming_bytes_per_second = 0;
	int max_outgoing_bytes_per_second = 0;
	int event_queue_initial_capacity = 1024;
	int event_queue_can_grow = 1;
};

CUTE_API server_t* CUTE_CALL server_alloc(void* user_allocator_context = NULL);
CUTE_API void CUTE_CALL server_destroy(server_t* server);

CUTE_API int CUTE_CALL server_start(server_t* server, const char* address_and_port, const crypto_key_t* public_key, const crypto_key_t* secret_key, const server_config_t* config = NULL);
CUTE_API void CUTE_CALL server_stop(server_t* server);

CUTE_API void CUTE_CALL server_update(server_t* server, float dt);
CUTE_API int CUTE_CALL server_poll_event(server_t* server, server_event_t* event);
CUTE_API void CUTE_CALL server_disconnect_client(server_t* server, handle_t client_id, int send_notification_to_client = 1);
CUTE_API void CUTE_CALL server_look_for_and_disconnected_timed_out_clients(server_t* server);
CUTE_API void CUTE_CALL server_broadcast_to_all_clients(server_t* server, const void* packet, int size, int reliable);
CUTE_API void CUTE_CALL server_broadcast_to_all_but_one_client(server_t* server, const void* packet, int size, handle_t client_id, int reliable);
CUTE_API void CUTE_CALL server_send_to_client(server_t* server, const void* packet, int size, handle_t client_id, int reliable);

CUTE_API float CUTE_CALL server_get_last_packet_recieved_time_from_client(server_t* server, handle_t client_id);

CUTE_API handle_t CUTE_CALL server_connect_loopback_client();

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
			handle_t client_id;
			endpoint_t endpoint;
		} new_connection;

		struct
		{
			handle_t client_id;
		} disconnected;

		struct
		{
			handle_t client_id;
			void* data;
			int size;
		} user_packet;
	} u;
};

/*
	Server API - WIP
	server update func
	poll event func (dequeue)
		get user packet from event
		accept new connections via event
	look for timed out clients feature
	disconnect client function
	thread to receive packets from socket
	broadcast funcs - to all, to all but one, to one

	get client data by id
		endpoint
		is loopback

	loopback functions
		send packet
		recieve packet
		client userdata
*/

}

#endif // CUTE_SERVER_H
