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
#include <cute_crypto.h>
#include <cute_c_runtime.h>
#include <cute_protocol.h>

#include <internal/cute_defines_internal.h>
#include <internal/cute_net_internal.h>
#include <internal/cute_protocol_internal.h>

#include <cute/cute_serialize.h>

#define CUTE_SERVER_SEND_BUFFER_SIZE (20 * CUTE_MB)
#define CUTE_SERVER_RECEIVE_BUFFER_SIZE (20 * CUTE_MB)

#define CUTE_SERVER_CONNECTION_DENIED_MAX_COUNT (1024)

namespace cute
{

/*
struct server_t
{
	int running = 0;
	endpoint_t endpoint;
	crypto_key_t public_key;
	crypto_key_t secret_key;
	server_config_t config;
	socket_t socket;
	serialize_t* io = NULL;
	protocol::packet_queue_t packets;
	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];
	void* mem_ctx = NULL;

	int connection_denied_count = 0;
	handle_table_t client_handle_table;
	int client_count = 0;
	handle_t client_handle[CUTE_SERVER_MAX_CLIENTS];
	int client_is_connected[CUTE_SERVER_MAX_CLIENTS];
	int client_is_loopback[CUTE_SERVER_MAX_CLIENTS];
	float client_last_packet_recieved_time[CUTE_SERVER_MAX_CLIENTS];
	float client_last_packet_sent_time[CUTE_SERVER_MAX_CLIENTS];
	endpoint_t client_endpoint[CUTE_SERVER_MAX_CLIENTS];
	uint64_t client_sequence_offset[CUTE_SERVER_MAX_CLIENTS];
	uint64_t client_sequence[CUTE_SERVER_MAX_CLIENTS];
	protocol::replay_buffer_t client_nonce_buffer[CUTE_SERVER_MAX_CLIENTS];
	crypto_key_t client_session_key[CUTE_SERVER_MAX_CLIENTS];
	protocol::packet_queue_t client_packets[CUTE_SERVER_MAX_CLIENTS];

	int event_queue_can_grow = 0;
	int event_queue_size = 0;
	int event_queue_capacity = 0;
	int event_queue_index0 = 0;
	int event_queue_index1 = 0;
	server_event_t* event_queue = 0;
};

server_t* server_alloc(void* user_allocator_context)
{
	server_t* server = (server_t*)CUTE_ALLOC(sizeof(server_t), user_allocator_context);
	CUTE_CHECK_POINTER(server);
	CUTE_PLACEMENT_NEW(server) server_t;
	server->mem_ctx = user_allocator_context;
	return server;

cute_error:
	CUTE_FREE(server, user_allocator_context);
	return NULL;
}

void server_destroy(server_t* server)
{
	CUTE_FREE(server, server->mem_ctx);
}

int server_start(server_t* server, const char* address_and_port, const crypto_key_t* public_key, const crypto_key_t* secret_key, const server_config_t* config)
{
	CUTE_CHECK(endpoint_init(&server->endpoint, address_and_port));
	server->public_key = *public_key;
	server->secret_key = *secret_key;
	CUTE_CHECK(socket_init(&server->socket, address_and_port, CUTE_SERVER_SEND_BUFFER_SIZE, CUTE_SERVER_RECEIVE_BUFFER_SIZE));
	server->io = serialize_buffer_create(SERIALIZE_READ, NULL, 0, NULL);
	server->config = config ? *config : server_config_t();
	CUTE_CHECK(handle_table_init(&server->client_handle_table, 256, server->mem_ctx));
	server->connection_denied_count = 0;
	server->client_count = 0;

	config = &server->config;
	server->event_queue_can_grow = config->event_queue_can_grow;
	server->event_queue_size = 0;
	server->event_queue_capacity = config->event_queue_initial_capacity;
	server->event_queue_index0 = 0;
	server->event_queue = (server_event_t*)CUTE_ALLOC(sizeof(server_event_t) * config->event_queue_initial_capacity, server->mem_ctx);
	if (config->event_queue_initial_capacity) CUTE_CHECK_POINTER(server->event_queue);

	server->running = 1;

	server->client_count = 0;
	return 0;

cute_error:
	return -1;
}

void server_stop(server_t* server)
{
	while(server->client_count)
	{
		server_disconnect_client(server, server->client_handle[0]);
	}

	server->running = 0;
	socket_cleanup(&server->socket);
	handle_table_clean_up(&server->client_handle_table);
	serialize_destroy(server->io);
	CUTE_FREE(server->event_queue, server->mem_ctx);
	server->event_queue = NULL;
	server->io = NULL;
}

static uint32_t s_client_index_from_endpoint(server_t* server, endpoint_t endpoint)
{
	endpoint_t* endpoints = server->client_endpoint;
	int count = server->client_count;
	for (int i = 0; i < count; ++i)
	{
		if (endpoint_equals(endpoints[i], endpoint)) {
			return i;
		}
	}
	return UINT32_MAX;
}

static CUTE_INLINE uint32_t s_client_index_from_handle(server_t* server, handle_t h)
{
	return handle_table_get_index(&server->client_handle_table, h);
}

static server_event_t* s_push_event(server_t* server)
{
	CUTE_ASSERT(server->event_queue_size <= server->event_queue_capacity);
	if (server->event_queue_size == server->event_queue_capacity) {
		if (!server->event_queue_can_grow) return NULL;
		server_event_t* old_data = server->event_queue;
		int new_capacity = server->event_queue_capacity ? server->event_queue_capacity * 2 : 256;
		server_event_t* new_data = (server_event_t*)CUTE_ALLOC(sizeof(server_event_t) * new_capacity, server->mem_ctx);
		if (!new_data) return NULL;
		CUTE_MEMCPY(new_data, old_data, sizeof(server_event_t) * server->event_queue_capacity);
		CUTE_FREE(old_data, server->mem_ctx);
		server->event_queue = new_data;
		server->event_queue_capacity = new_capacity;
	}

	int index = server->event_queue_index0++;
	server->event_queue_index0 %= server->event_queue_capacity;
	server->event_queue_size++;
	return server->event_queue + index;
}

static uint32_t s_client_make(server_t* server, endpoint_t endpoint, crypto_key_t* key, int loopback)
{
	if (server->client_count == CUTE_SERVER_MAX_CLIENTS) {
		return UINT32_MAX;
	}

	uint64_t sequence_offset;
	crypto_random_bytes(&sequence_offset, sizeof(sequence_offset));

	uint32_t index = (uint32_t)server->client_count++;
	server->client_handle[index] = handle_table_alloc(&server->client_handle_table, index);
	CUTE_ASSERT(server->client_handle[index] != CUTE_INVALID_HANDLE);
	server->client_is_connected[index] = 1;
	server->client_is_loopback[index] = loopback;
	server->client_last_packet_recieved_time[index] = 0;
	server->client_last_packet_sent_time[index] = 0;
	server->client_endpoint[index] = endpoint;
	server->client_sequence_offset[index] = sequence_offset;
	server->client_sequence[index] = 0;
	replay_buffer_init(server->client_nonce_buffer + index);
	server->client_session_key[index] = *key;
	packet_queue_init(server->client_packets + index);

	server_event_t* event = s_push_event(server);
	event->type = SERVER_EVENT_TYPE_NEW_CONNECTION;
	event->u.new_connection.client_id = server->client_handle[index];
	event->u.new_connection.endpoint = endpoint;

	return index;
}

static int s_server_send_packet_no_payload(server_t* server, int client_index, protocol::packet_type_t packet_type)
{
	//int packet_size = packet_write_header(server->io, server->buffer, packet_type, server->client_sequence[client_index]);
	//if (packet_size <= 0) return -1;
	//return packet_encrypt_and_send(server->client_session_key + client_index, &server->socket, server->client_endpoint[client_index], server->buffer, packet_size, server->client_sequence_offset[client_index] + server->client_sequence[client_index]++);
	return -1;
}

// Attempt to record address to kindly report to the client their connection was denied.
// This can be good to help benign clients make an informed decision to not keep retrying
// the connection over and over, since this message means a configuration error prevented
// the encrypted tunnel from opening. Sending connection denied responses will not really
// help in the case of a harmful client attempting a DOS attack. For the case of DOS this
// buffer is capped to `CUTE_SERVER_CONNECTION_DENIED_MAX_COUNT`, and responses will be
// dropped if this buffer fills up.
static void s_server_connection_denied(server_t* server, endpoint_t from, const crypto_key_t* key)
{
	CUTE_ASSERT(server->connection_denied_count <= CUTE_SERVER_CONNECTION_DENIED_MAX_COUNT);
	if (server->connection_denied_count == CUTE_SERVER_CONNECTION_DENIED_MAX_COUNT) {
		return;
	}
	server->connection_denied_count++;

	//int packet_size = packet_write_header(server->io, server->buffer, PACKET_TYPE_CONNECTION_DENIED, 0);
	//packet_encrypt_and_send(key, &server->socket, from, server->buffer, packet_size, 0);
}

static int s_send_packet_to_client(server_t* server, uint32_t client_index, uint8_t* packet, int size, uint64_t sequence)
{
	int sequence_size = sizeof(uint64_t);
	int size_minus_sequence = size - sizeof(uint64_t);
	CUTE_ASSERT(size_minus_sequence > 0);
	//CUTE_CHECK(crypto_encrypt(server->client_session_key + client_index, packet + sizeof(uint64_t), size_minus_sequence, CUTE_PROTOCOL_PACKET_SIZE_MAX, sequence));
	//int size_along_with_encryption_bytes = size + CUTE_CRYPTO_SYMMETRIC_BYTES;
	//CUTE_ASSERT(size_along_with_encryption_bytes <= CUTE_PROTOCOL_PACKET_SIZE_MAX);
	//int bytes_sent = socket_send(&server->socket, server->client_endpoint[client_index], packet, size_along_with_encryption_bytes);
	//if (bytes_sent != size_along_with_encryption_bytes) {
		return -1;
	//}
	//return 0;

//cute_error:
//	return -1;
}

static void s_server_recieve_packets(server_t* server)
{
	uint8_t* buffer = server->buffer;

	while (1)
	{
		endpoint_t from;
		int bytes_read = socket_receive(&server->socket, &from, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);
		if (bytes_read <= 0) {
			// No more packets to receive for now.
			break;
		}

		//if (packet_validate_size(bytes_read)) {
		//	continue;
		//}

		// Find client by address.
		uint32_t client_index = s_client_index_from_endpoint(server, from);
		if (client_index == UINT32_MAX) {
			// Client address not found -- potential new connection.

			if (bytes_read < CUTE_PROTOCOL_PACKET_SIZE_MAX) {
				// New connections *must* be padded to `CUTE_PROTOCOL_PACKET_SIZE_MAX`, or will be dropped. This helps
				// to dissuade nefarious usage of the connection API from dubious users.
				continue;
			}

			// Decrypt packet.
			//if (crypto_decrypt_asymmetric(&server->public_key, &server->secret_key, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX)) {
			//	// Forged/tampered packet!
			//	continue;
			//}

			serialize_t* io = server->io;
			serialize_reset_buffer(io, SERIALIZE_READ, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);

			// Read version string.
			const uint8_t* version_string = CUTE_PROTOCOL_VERSION_STRING;
			char version_buffer[CUTE_PROTOCOL_VERSION_STRING_LEN];
			CUTE_CHECK(serialize_bytes(io, (unsigned char*)version_buffer, CUTE_PROTOCOL_VERSION_STRING_LEN));
			CUTE_CHECK(CUTE_STRNCMP((const char*)version_string, version_buffer, CUTE_PROTOCOL_VERSION_STRING_LEN));

			// Read symmetric key.
			crypto_key_t key;
			CUTE_CHECK(serialize_bytes(io, key.key, sizeof(key)));

			if (server->client_count == CUTE_SERVER_MAX_CLIENTS) {
				// Not accepting new connections; out of client slots.
				s_server_connection_denied(server, from, &key);
				continue;
			}

			// Make new client, store session key.
			client_index = s_client_make(server, from, &key, 0);
			if (client_index == UINT32_MAX) {
				// Failed to create new client for some reason (like out of memory).
				s_server_connection_denied(server, from, &key);
				continue;
			}

			// Send connection accepted packet.
			serialize_reset_buffer(io, SERIALIZE_WRITE, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);

			uint64_t first_sequence = 0;
			CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &first_sequence));

			uint64_t packet_typeu64 = (uint64_t)protocol::PACKET_TYPE_CONNECTION_ACCEPTED;
			CUTE_SERIALIZE_CHECK(serialize_uint64(io, &packet_typeu64, 0, protocol::PACKET_TYPE_MAX));
			CUTE_SERIALIZE_CHECK(serialize_flush(io));

			uint64_t sequence_offset = server->client_sequence_offset[client_index];
			CUTE_SERIALIZE_CHECK(serialize_uint64_full(io, &sequence_offset));

			int bytes_written = serialize_serialized_bytes(io);

			s_send_packet_to_client(server, client_index, buffer, bytes_written, first_sequence);
		} else {
			serialize_t* io = server->io;
			serialize_reset_buffer(io, SERIALIZE_READ, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);

			uint64_t sequence;
			CUTE_CHECK(serialize_uint64_full(io, &sequence));

			protocol::packet_type_t type = protocol::PACKET_TYPE_MAX;
			//int packet_size;
			//uint8_t* packet = packet_open(io, server->client_session_key + client_index, server->client_nonce_buffer + client_index, buffer, bytes_read, sequence, server->client_sequence_offset[client_index], &type, &packet_size);
			uint8_t* packet = NULL;
			CUTE_CHECK_POINTER(packet);

			int valid_packet = 0;

			switch (type)
			{
			case protocol::PACKET_TYPE_CONNECTION_ACCEPTED:
				break;

			case protocol::PACKET_TYPE_CONNECTION_DENIED:
				break;

			case protocol::PACKET_TYPE_KEEPALIVE:
				valid_packet = 1;
				break;

			case protocol::PACKET_TYPE_DISCONNECT:
				valid_packet = 1;
				server_disconnect_client(server, server->client_handle[client_index], 0);
				break;

			case protocol::PACKET_TYPE_USERDATA:
				valid_packet = 1;
				if (server->client_is_connected[client_index]) {
					CUTE_CHECK(packet_queue_push(server->client_packets + client_index, packet, type));
				}
				break;
			}

			if (valid_packet && server->client_is_connected[client_index]) {
				server->client_last_packet_recieved_time[client_index] = 0;
			}
		}

	cute_error:
		// Drop any packets that fail to serialize.
		continue;
	}
}

static void s_server_send_packets(server_t* server, float dt)
{
	CUTE_ASSERT(server->running);
	
	int client_count = server->client_count;
	float* last_sent_times = server->client_last_packet_sent_time;
	int* is_loopback = server->client_is_loopback;
	for (int i = 0; i < client_count; ++i)
	{
		if (!is_loopback[i]) {
			if (last_sent_times[i] >= CUTE_KEEPALIVE_RATE) {
				last_sent_times[i] = 0;
				s_server_send_packet_no_payload(server, i, protocol::PACKET_TYPE_KEEPALIVE);
			}
		}
	}
}

void server_update(server_t* server, float dt)
{
	server->connection_denied_count = 0;
	s_server_recieve_packets(server);
	s_server_send_packets(server, dt);

	int client_count = server->client_count;
	float* last_recieved_times = server->client_last_packet_recieved_time;
	float* last_sent_times = server->client_last_packet_sent_time;
	for (int i = 0; i < client_count; ++i)
	{
		last_recieved_times[i] += dt;
		last_sent_times[i] += dt;
	}
}

int server_poll_event(server_t* server, server_event_t* event)
{
	if (server->event_queue_size) {
		*event = server->event_queue[server->event_queue_index1++];
		server->event_queue_index1 %= server->event_queue_capacity;
		server->event_queue_size--;
		return 0;
	}
	return -1;
}

void server_disconnect_client(server_t* server, handle_t client_id, int send_notification_to_client)
{
	CUTE_ASSERT(server->client_count >= 1);
	uint32_t index = handle_table_get_index(&server->client_handle_table, client_id);
	if (send_notification_to_client) {
		s_server_send_packet_no_payload(server, index, protocol::PACKET_TYPE_DISCONNECT);
	}

	// Free client resources.
	server->client_is_connected[index] = 0;
	handle_table_free(&server->client_handle_table, client_id);

	// Move client in back to the empty slot.
	int last_index = --server->client_count;
	if (last_index) {
		handle_t h = server->client_handle[index];
		handle_table_update_index(&server->client_handle_table, h, index);

		server->client_handle[index]                    = server->client_handle[last_index];
		server->client_is_connected[index]              = server->client_is_connected[last_index];
		server->client_is_loopback[index]               = server->client_is_loopback[last_index];
		server->client_last_packet_recieved_time[index] = server->client_last_packet_recieved_time[last_index];
		server->client_last_packet_sent_time[index]     = server->client_last_packet_sent_time[last_index];
		server->client_endpoint[index]                  = server->client_endpoint[last_index];
		server->client_sequence_offset[index]           = server->client_sequence_offset[last_index];
		server->client_sequence[index]                  = server->client_sequence[last_index];
		server->client_nonce_buffer[index]              = server->client_nonce_buffer[last_index];
		server->client_session_key[index]               = server->client_session_key[last_index];
		server->client_packets[index]                   = server->client_packets[last_index];
	}
}

void server_look_for_and_disconnected_timed_out_clients(server_t* server)
{
	int client_count = server->client_count;
	float* last_recieved_times = server->client_last_packet_recieved_time;
	for (int i = 0; i < client_count;)
	{
		if (last_recieved_times[i] >= CUTE_KEEPALIVE_RATE * 3) {
			--client_count;
			server_disconnect_client(server, server->client_handle[i], 1);
		} else {
			 ++i;
		}
	}
}

void server_broadcast_to_all_clients(server_t* server, const void* packet, int size, int reliable)
{
}

void server_broadcast_to_all_but_one_client(server_t* server, const void* packet, int size, handle_t id, int reliable)
{
}

void server_send_to_client(server_t* server, const void* packet, int size, handle_t id, int reliable)
{
}

float server_get_last_packet_recieved_time_from_client(server_t* server, handle_t client_id)
{
	uint32_t index = s_client_index_from_handle(server, client_id);
	if (index == UINT32_MAX) return -1.0f;
	CUTE_ASSERT(server->client_is_connected[index]);
	return server->client_last_packet_recieved_time[index];
}
*/

}
