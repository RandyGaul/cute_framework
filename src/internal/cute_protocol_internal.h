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

namespace cute
{
namespace protocol
{

enum packet_type_t : uint8_t
{
	PACKET_TYPE_CONNECT_TOKEN,
	PACKET_TYPE_CONNECTION_ACCEPTED,
	PACKET_TYPE_CONNECTION_DENIED,
	PACKET_TYPE_KEEPALIVE,
	PACKET_TYPE_DISCONNECT,
	PACKET_TYPE_CHALLENGE_REQUEST,
	PACKET_TYPE_CHALLENGE_RESPONSE,
	PACKET_TYPE_PAYLOAD,
};

struct packet_queue_t
{
	int count = 0;
	int index0 = 0;
	int index1 = 0;
	packet_type_t types[CUTE_PACKET_QUEUE_MAX_ENTRIES];
	void* packets[CUTE_PACKET_QUEUE_MAX_ENTRIES];
};

extern CUTE_API void CUTE_CALL packet_queue_init(packet_queue_t* q);
extern CUTE_API int CUTE_CALL packet_queue_push(packet_queue_t* q, void* packet, packet_type_t type);
extern CUTE_API int CUTE_CALL packet_queue_pop(packet_queue_t* q, void** packet, packet_type_t* type);

// -------------------------------------------------------------------------------------------------

struct replay_buffer_t
{
	uint64_t max;
	uint64_t entries[CUTE_REPLAY_BUFFER_SIZE];
};

extern CUTE_API void CUTE_CALL replay_buffer_init(replay_buffer_t* replay_buffer);
extern CUTE_API int CUTE_CALL replay_buffer_cull_duplicate(replay_buffer_t* replay_buffer, uint64_t sequence);
extern CUTE_API void CUTE_CALL replay_buffer_update(replay_buffer_t* replay_buffer, uint64_t sequence);

// -------------------------------------------------------------------------------------------------

struct packet_allocator_t;

extern CUTE_API packet_allocator_t* CUTE_CALL packet_allocator_make(void* user_allocator_context = NULL);
extern CUTE_API void CUTE_CALL packet_allocator_destroy(packet_allocator_t* packet_allocator);
extern CUTE_API void* CUTE_CALL packet_allocator_alloc(packet_allocator_t* packet_allocator, packet_type_t type);
extern CUTE_API void CUTE_CALL packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet);

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
	uint64_t client_handle;
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

extern CUTE_API int CUTE_CALL packet_write(void* packet_ptr, uint8_t* buffer, uint64_t application_id, uint64_t sequence, const crypto_key_t* key);
extern CUTE_API void* CUTE_CALL packet_open(uint8_t* buffer, int size, const crypto_key_t* key, uint64_t application_id, packet_allocator_t* pa, replay_buffer_t* replay_buffer);

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
	uint8_t hmac_bytes[CUTE_CRYPTO_HMAC_BYTES];
};

extern CUTE_API uint8_t* CUTE_CALL client_read_connect_token_from_web_service(uint8_t* buffer, uint64_t application_id, uint64_t current_time, connect_token_t* token);
extern CUTE_API int CUTE_CALL server_decrypt_connect_token_packet(uint8_t* packet_buffer, const crypto_key_t* secret_key, uint64_t application_id, uint64_t current_time, connect_token_decrypted_t* token);

// -------------------------------------------------------------------------------------------------

#define CUTE_PROTOCOL_HASHTABLE_KEY_BYTES (crypto_shorthash_KEYBYTES)
#define CUTE_PROTOCOL_HASHTABLE_HASH_BYTES (crypto_shorthash_BYTES)

CUTE_STATIC_ASSERT(CUTE_PROTOCOL_HASHTABLE_HASH_BYTES == 8, "The hash output must be 8 in order to fit nicely into a `uint64_t` hash.");

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

extern CUTE_API int CUTE_CALL hashtable_init(hashtable_t* table, int key_size, int item_size, int capacity, void* mem_ctx);
extern CUTE_API void CUTE_CALL hashtable_cleanup(hashtable_t* table);

extern CUTE_API void* CUTE_CALL hashtable_insert(hashtable_t* table, const void* key, const void* item);
extern CUTE_API void CUTE_CALL hashtable_remove(hashtable_t* table, const void* key);
extern CUTE_API void CUTE_CALL hashtable_clear(hashtable_t* table);
extern CUTE_API void* CUTE_CALL hashtable_find(const hashtable_t* table, const void* key);
extern CUTE_API int CUTE_CALL hashtable_count(const hashtable_t* table);
extern CUTE_API void* CUTE_CALL hashtable_items(const hashtable_t* table);
extern CUTE_API void* CUTE_CALL hashtable_keys(const hashtable_t* table);
extern CUTE_API void CUTE_CALL hashtable_swap(hashtable_t* table, int index_a, int index_b);

// -------------------------------------------------------------------------------------------------

#define CUTE_PROTOCOL_CONNECT_TOKEN_ENTRIES_MAX (CUTE_PROTOCOL_CLIENT_MAX * 8)

struct connect_token_cache_entry_t
{
	uint64_t entry_creation_time;
	uint64_t token_expire_time;
	endpoint_t endpoint;
	uint8_t hmac_bytes[CUTE_CRYPTO_HMAC_BYTES];
	list_node_t* node;
};

struct connect_token_cache_node_t
{
	uint8_t hmac_bytes[CUTE_CRYPTO_HMAC_BYTES];
	list_node_t node;
};

struct connect_token_cache_t
{
	hashtable_t table;
	list_t list;
	list_t free_list;
	connect_token_cache_node_t* node_memory;
	void* mem_ctx;
};

extern CUTE_API int CUTE_CALL connect_token_cache_init(connect_token_cache_t* cache, void* mem_ctx);
extern CUTE_API void CUTE_CALL connect_token_cache_cleanup(connect_token_cache_t* cache);

extern CUTE_API connect_token_cache_entry_t* CUTE_CALL connect_token_cache_find(connect_token_cache_t* cache, const uint8_t* hmac_bytes);
extern CUTE_API void CUTE_CALL connect_token_cache_add(connect_token_cache_t* cache, uint64_t entry_creation_time, uint64_t token_expire_time, endpoint_t endpoint, const uint8_t* hmac_bytes);

// -------------------------------------------------------------------------------------------------

#define CUTE_ENCRYPTION_STATES_MAX (CUTE_PROTOCOL_CLIENT_MAX * 2)

struct encryption_state_t
{
	uint64_t sequence;
	uint32_t expiration_timestamp;
	uint32_t handshake_timeout;
	float last_handshake_access_time;
	crypto_key_t client_to_server_key;
	crypto_key_t server_to_client_key;
};

struct encryption_map_t
{
	hashtable_t table;
};

extern CUTE_API int CUTE_CALL encryption_map_init(encryption_map_t* map, void* mem_ctx);
extern CUTE_API void CUTE_CALL encryption_map_cleanup(encryption_map_t* map);
extern CUTE_API void CUTE_CALL encryption_map_clear(encryption_map_t* map);
extern CUTE_API int CUTE_CALL encryption_map_count(encryption_map_t* map);

extern CUTE_API void CUTE_CALL encryption_map_insert(encryption_map_t* map, endpoint_t endpoint, const encryption_state_t* state);
extern CUTE_API encryption_state_t* CUTE_CALL encryption_map_find(encryption_map_t* map, endpoint_t endpoint);
extern CUTE_API void CUTE_CALL encryption_map_remove(encryption_map_t* map, endpoint_t endpoint);
extern CUTE_API endpoint_t* CUTE_CALL encryption_map_get_endpoints(encryption_map_t* map);
extern CUTE_API encryption_state_t* CUTE_CALL encryption_map_get_states(encryption_map_t* map);

extern CUTE_API void CUTE_CALL encryption_map_look_for_timeouts_or_expirations(encryption_map_t* map, float dt, uint64_t time);

}
}

#endif // CUTE_PROTOCOL_INTERNAL_H
