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

#ifndef CUTE_PROTOCOL_INTERNAL_H
#define CUTE_PROTOCOL_INTERNAL_H

#include <cute_protocol.h>
#include <cute_doubly_list.h>
#include <cute_circular_buffer.h>

#include <internal/cute_net_internal.h>

#include <hydrogen.h>

namespace cute
{
namespace protocol
{

struct replay_buffer_t
{
	uint64_t max;
	uint64_t entries[CUTE_REPLAY_BUFFER_SIZE];
};

CUTE_API void CUTE_CALL replay_buffer_init(replay_buffer_t* replay_buffer);
CUTE_API int CUTE_CALL replay_buffer_cull_duplicate(replay_buffer_t* replay_buffer, uint64_t sequence);
CUTE_API void CUTE_CALL replay_buffer_update(replay_buffer_t* replay_buffer, uint64_t sequence);

// -------------------------------------------------------------------------------------------------

struct packet_connect_token_t
{
	uint8_t packet_type;
	uint64_t expiration_timestamp;
	uint32_t handshake_timeout;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_ENDPOINT_MAX];
};

struct packet_connection_accepted_t
{
	uint8_t packet_type;
	uint64_t client_id;
	uint32_t max_clients;
	uint32_t connection_timeout;
};

struct packet_connection_denied_t
{
	uint8_t packet_type;
};

struct packet_keepalive_t
{
	uint8_t packet_type;
};

struct packet_disconnect_t
{
	uint8_t packet_type;
};

struct packet_challenge_t
{
	uint8_t packet_type;
	uint64_t challenge_nonce;
	uint8_t challenge_data[CUTE_CHALLENGE_DATA_SIZE];
};

struct packet_payload_t
{
	uint8_t packet_type;
	uint16_t payload_size;
	uint8_t payload[CUTE_PROTOCOL_PACKET_PAYLOAD_MAX];
};

CUTE_API int CUTE_CALL packet_write(void* packet_ptr, uint8_t* buffer, uint64_t sequence, const crypto_key_t* key);
CUTE_API void* CUTE_CALL packet_open(uint8_t* buffer, int size, const crypto_key_t* key, packet_allocator_t* pa, replay_buffer_t* replay_buffer = NULL, uint64_t* sequence_ptr = NULL);

// -------------------------------------------------------------------------------------------------

struct connect_token_t
{
	uint64_t creation_timestamp;
	crypto_key_t client_to_server_key;
	crypto_key_t server_to_client_key;
	
	uint64_t expiration_timestamp;
	uint32_t handshake_timeout;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_ENDPOINT_MAX];
};

struct connect_token_decrypted_t
{
	uint64_t expiration_timestamp;
	uint32_t handshake_timeout;
	uint16_t endpoint_count;
	endpoint_t endpoints[CUTE_CONNECT_TOKEN_ENDPOINT_MAX];

	uint64_t client_id;
	crypto_key_t client_to_server_key;
	crypto_key_t server_to_client_key;
	uint8_t user_data[CUTE_CONNECT_TOKEN_USER_DATA_SIZE];
	crypto_signature_t signature;
};

CUTE_API uint8_t* CUTE_CALL client_read_connect_token_from_web_service(uint8_t* buffer, uint64_t application_id, uint64_t current_time, connect_token_t* token);
CUTE_API error_t CUTE_CALL server_decrypt_connect_token_packet(uint8_t* packet_buffer, const crypto_sign_public_t* public_key, const crypto_sign_secret_t* secret_key, uint64_t application_id, uint64_t current_time, connect_token_decrypted_t* token);

// -------------------------------------------------------------------------------------------------

#define CUTE_PROTOCOL_HASHTABLE_KEY_BYTES (hydro_hash_KEYBYTES)
#define CUTE_PROTOCOL_HASHTABLE_HASH_BYTES (hydro_hash_BYTES)

struct hashtable_slot_t
{
	uint64_t key_hash;
	int item_index;
	int base_count;
};

struct hashtable_t
{
	int count;
	int slot_capacity;
	hashtable_slot_t* slots;

	uint8_t secret_key[CUTE_PROTOCOL_HASHTABLE_KEY_BYTES];

	int key_size;
	int item_size;
	int item_capacity;
	void* items_key;
	int* items_slot_index;
	void* items_data;

	void* temp_key;
	void* temp_item;
	void* mem_ctx;
};

CUTE_API void CUTE_CALL hashtable_init(hashtable_t* table, int key_size, int item_size, int capacity, void* mem_ctx);
CUTE_API void CUTE_CALL hashtable_cleanup(hashtable_t* table);

CUTE_API void* CUTE_CALL hashtable_insert(hashtable_t* table, const void* key, const void* item);
CUTE_API void CUTE_CALL hashtable_remove(hashtable_t* table, const void* key);
CUTE_API void CUTE_CALL hashtable_clear(hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_find(const hashtable_t* table, const void* key);
CUTE_API int CUTE_CALL hashtable_count(const hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_items(const hashtable_t* table);
CUTE_API void* CUTE_CALL hashtable_keys(const hashtable_t* table);
CUTE_API void CUTE_CALL hashtable_swap(hashtable_t* table, int index_a, int index_b);

// -------------------------------------------------------------------------------------------------

#define CUTE_PROTOCOL_CONNECT_TOKEN_ENTRIES_MAX (CUTE_PROTOCOL_SERVER_MAX_CLIENTS * 8)

struct connect_token_cache_entry_t
{
	list_node_t* node;
};

struct connect_token_cache_node_t
{
	crypto_signature_t signature;
	list_node_t node;
};

struct connect_token_cache_t
{
	int capacity;
	hashtable_t table;
	list_t list;
	list_t free_list;
	connect_token_cache_node_t* node_memory;
	void* mem_ctx;
};

CUTE_API void CUTE_CALL connect_token_cache_init(connect_token_cache_t* cache, int capacity, void* mem_ctx);
CUTE_API void CUTE_CALL connect_token_cache_cleanup(connect_token_cache_t* cache);

CUTE_API connect_token_cache_entry_t* CUTE_CALL connect_token_cache_find(connect_token_cache_t* cache, const uint8_t* hmac_bytes);
CUTE_API void CUTE_CALL connect_token_cache_add(connect_token_cache_t* cache, const uint8_t* hmac_bytes);

// -------------------------------------------------------------------------------------------------

#define CUTE_ENCRYPTION_STATES_MAX (CUTE_PROTOCOL_SERVER_MAX_CLIENTS * 2)

struct encryption_state_t
{
	uint64_t sequence;
	uint64_t expiration_timestamp;
	uint32_t handshake_timeout;
	double last_packet_recieved_time;
	double last_packet_sent_time;
	crypto_key_t client_to_server_key;
	crypto_key_t server_to_client_key;
	uint64_t client_id;
	crypto_signature_t signature;
};

struct encryption_map_t
{
	hashtable_t table;
};

CUTE_API void CUTE_CALL encryption_map_init(encryption_map_t* map, void* mem_ctx);
CUTE_API void CUTE_CALL encryption_map_cleanup(encryption_map_t* map);
CUTE_API void CUTE_CALL encryption_map_clear(encryption_map_t* map);
CUTE_API int CUTE_CALL encryption_map_count(encryption_map_t* map);

CUTE_API void CUTE_CALL encryption_map_insert(encryption_map_t* map, endpoint_t endpoint, const encryption_state_t* state);
CUTE_API encryption_state_t* CUTE_CALL encryption_map_find(encryption_map_t* map, endpoint_t endpoint);
CUTE_API void CUTE_CALL encryption_map_remove(encryption_map_t* map, endpoint_t endpoint);
CUTE_API endpoint_t* CUTE_CALL encryption_map_get_endpoints(encryption_map_t* map);
CUTE_API encryption_state_t* CUTE_CALL encryption_map_get_states(encryption_map_t* map);

CUTE_API void CUTE_CALL encryption_map_look_for_timeouts_or_expirations(encryption_map_t* map, double dt, uint64_t time);

// -------------------------------------------------------------------------------------------------

struct net_simulator_t;

struct client_t
{
	bool use_ipv6;
	client_state_t state;
	double last_packet_recieved_time;
	double last_packet_sent_time;
	uint64_t application_id;
	uint64_t current_time;
	uint64_t client_id;
	int max_clients;
	double connection_timeout;
	int has_sent_disconnect_packets;
	connect_token_t connect_token;
	uint64_t challenge_nonce;
	uint8_t challenge_data[CUTE_CHALLENGE_DATA_SIZE];
	int goto_next_server;
	client_state_t goto_next_server_tentative_state;
	int server_endpoint_index;
	endpoint_t web_service_endpoint;
	socket_t socket;
	uint64_t sequence;
	circular_buffer_t packet_queue;
	replay_buffer_t replay_buffer;
	net_simulator_t* sim;
	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];
	uint8_t connect_token_packet[CUTE_CONNECT_TOKEN_PACKET_SIZE];
	void* mem_ctx;
};

// -------------------------------------------------------------------------------------------------

struct server_t
{
	bool running;
	uint64_t application_id;
	uint64_t current_time;
	socket_t socket;
	protocol::packet_allocator_t* packet_allocator;
	crypto_sign_public_t public_key;
	crypto_sign_secret_t secret_key;
	uint32_t connection_timeout;
	circular_buffer_t event_queue;
	net_simulator_t* sim;

	uint64_t challenge_nonce;
	encryption_map_t encryption_map;
	connect_token_cache_t token_cache;

	int client_count;
	hashtable_t client_endpoint_table;
	hashtable_t client_id_table;
	uint64_t client_id[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	bool client_is_connected[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	bool client_is_confirmed[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	double client_last_packet_received_time[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	double client_last_packet_sent_time[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	endpoint_t client_endpoint[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	uint64_t client_sequence[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	crypto_key_t client_client_to_server_key[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	crypto_key_t client_server_to_client_key[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];
	protocol::replay_buffer_t client_replay_buffer[CUTE_PROTOCOL_SERVER_MAX_CLIENTS];

	uint8_t buffer[CUTE_PROTOCOL_PACKET_SIZE_MAX];
	void* mem_ctx;
};

}
}

#endif // CUTE_PROTOCOL_INTERNAL_H
