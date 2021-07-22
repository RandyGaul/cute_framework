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

#include <cute_protocol.h>
#include <cute_c_runtime.h>
#include <cute_error.h>
#include <cute_alloc.h>
#include <cute_net.h>
#include <cute_handle_table.h>
#include <cute_memory_pool.h>

#include <internal/cute_serialize_internal.h>
#include <internal/cute_protocol_internal.h>
#include <internal/cute_net_internal.h>

#include <inttypes.h>

#include <hydrogen.h>

#define CUTE_PROTOCOL_CLIENT_SEND_BUFFER_SIZE (2 * CUTE_MB)
#define CUTE_PROTOCOL_CLIENT_RECEIVE_BUFFER_SIZE (2 * CUTE_MB)

#define CUTE_PROTOCOL_CONTEXT "CUTE_CTX"

namespace cute
{

namespace protocol
{

CUTE_STATIC_ASSERT(sizeof(crypto_key_t) == 32, "Cute Protocol standard calls for encryption keys to be 32 bytes.");
CUTE_STATIC_ASSERT(CUTE_PROTOCOL_VERSION_STRING_LEN == 10, "Cute Protocol standard calls for the version string to be 10 bytes.");
CUTE_STATIC_ASSERT(CUTE_CONNECT_TOKEN_PACKET_SIZE == 1024, "Cute Protocol standard calls for connect token packet to be exactly 1024 bytes.");
CUTE_STATIC_ASSERT(CUTE_PROTOCOL_SIGNATURE_SIZE == sizeof(crypto_signature_t), "Must be equal.");

#define CUTE_CHECK(X) if (X) ret = -1;

error_t generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const crypto_key_t* client_to_server_key,
	const crypto_key_t* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const crypto_sign_secret_t* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	CUTE_ASSERT(address_count >= 1 && address_count <= 32);
	CUTE_ASSERT(creation_timestamp < expiration_timestamp);

	uint8_t** p = &token_ptr_out;

	// Write the REST SECTION.
	write_bytes(p, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(p, application_id);
	write_uint64(p, creation_timestamp);
	write_key(p, client_to_server_key);
	write_key(p, server_to_client_key);

	// Write the PUBLIC SECTION.
	uint8_t* public_section = *p;
	write_uint8(p, 0);
	write_bytes(p, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(p, application_id);
	write_uint64(p, expiration_timestamp);
	write_uint32(p, handshake_timeout);
	write_uint32(p, (uint32_t)address_count);
	for (int i = 0; i < address_count; ++i)
	{
		endpoint_t endpoint;
		if (endpoint_init(&endpoint, address_list[i])) return error_failure("Unable to initialize endpoint.");
		write_endpoint(p, endpoint);
	}

	int bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written <= 568);
	int zeroes = 568 - bytes_written;
	for (int i = 0; i < zeroes; ++i)
		write_uint8(p, 0);

	bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written == 568);

	// Write the SECRET SECTION.
	uint8_t* secret_section = *p;
	CUTE_MEMSET(*p, 0, CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES);
	*p += CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES;
	write_uint64(p, client_id);
	write_key(p, client_to_server_key);
	write_key(p, server_to_client_key);
	if (user_data) {
		CUTE_MEMCPY(*p, user_data, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);
	} else {
		CUTE_MEMSET(*p, 0, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);
	}
	*p += CUTE_CONNECT_TOKEN_USER_DATA_SIZE;

	// Encrypt the SECRET SECTION.
	crypto_encrypt((const crypto_key_t*)shared_secret_key, secret_section, CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE - CUTE_CRYPTO_HEADER_BYTES);
	*p += CUTE_CRYPTO_HEADER_BYTES;

	// Compute the signature.
	crypto_signature_t signature;
	crypto_sign_create(shared_secret_key, &signature, public_section, 1024 - CUTE_PROTOCOL_SIGNATURE_SIZE);

	// Write the signature.
	CUTE_MEMCPY(*p, signature.bytes, sizeof(signature));
	*p += sizeof(signature);
	bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written == CUTE_CONNECT_TOKEN_PACKET_SIZE);

	return error_success();
}

// -------------------------------------------------------------------------------------------------

void replay_buffer_init(replay_buffer_t* replay_buffer)
{
	replay_buffer->max = 0;
	CUTE_MEMSET(replay_buffer->entries, ~0, sizeof(uint64_t) * CUTE_REPLAY_BUFFER_SIZE);
}

int replay_buffer_cull_duplicate(replay_buffer_t* replay_buffer, uint64_t sequence)
{
	if (sequence + CUTE_REPLAY_BUFFER_SIZE < replay_buffer->max) {
		// This is UDP - just drop old packets.
		return -1;
	}

	int index = (int)(sequence % CUTE_REPLAY_BUFFER_SIZE);
	uint64_t val = replay_buffer->entries[index];
	int empty_slot = val == ~0ULL;
	int outdated = val >= sequence;
	if (empty_slot | !outdated) {
		return 0;
	} else {
		// Duplicate or replayed packet detected.
		return -1;
	}
}

void replay_buffer_update(replay_buffer_t* replay_buffer, uint64_t sequence)
{
	if (replay_buffer->max < sequence) {
		replay_buffer->max = sequence;
	}

	int index = (int)(sequence % CUTE_REPLAY_BUFFER_SIZE);
	uint64_t val = replay_buffer->entries[index];
	int empty_slot = val == ~0ULL;
	int outdated = val >= sequence;
	if (empty_slot | !outdated) {
		replay_buffer->entries[index] = sequence;
	}
}

// -------------------------------------------------------------------------------------------------

error_t read_connect_token_packet_public_section(uint8_t* buffer, uint64_t application_id, uint64_t current_time, packet_connect_token_t* packet)
{
	uint8_t* buffer_start = buffer;

	// Read public section.
	packet->packet_type = (packet_type_t)read_uint8(&buffer);
	if (packet->packet_type != PACKET_TYPE_CONNECT_TOKEN) return error_failure("Expected packet type to be PACKET_TYPE_CONNECT_TOKEN.");
	if (CUTE_STRNCMP((const char*)buffer, (const char*)CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN)) {
		return error_failure("Unable to find `CUTE_PROTOCOL_VERSION_STRING` string.");
	}
	buffer += CUTE_PROTOCOL_VERSION_STRING_LEN;
	if (read_uint64(&buffer) != application_id) return error_failure("Found invalid application id.");
	packet->expiration_timestamp = read_uint64(&buffer);
	if (packet->expiration_timestamp < current_time) return error_failure("Packet has expired.");
	packet->handshake_timeout = read_uint32(&buffer);
	packet->endpoint_count = read_uint32(&buffer);
	int count = (int)packet->endpoint_count;
	if (count <= 0 || count > 32) return error_failure("Invalid endpoint count.");
	for (int i = 0; i < count; ++i)
		packet->endpoints[i] = read_endpoint(&buffer);
	int bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read <= 568);
	buffer += 568 - bytes_read;
	bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read == 568);

	return error_success();
}

static CUTE_INLINE uint8_t* s_header(uint8_t** p, uint8_t type, uint64_t sequence)
{
	write_uint8(p, type);
	write_uint64(p, sequence);
	CUTE_MEMSET(*p, 0, CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES);
	*p += CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES;
	return *p;
}

int packet_write(void* packet_ptr, uint8_t* buffer, uint64_t sequence, const crypto_key_t* key)
{
	uint8_t type = *(uint8_t*)packet_ptr;

	if (type == PACKET_TYPE_CONNECT_TOKEN) {
		CUTE_MEMCPY(buffer, packet_ptr, CUTE_CONNECT_TOKEN_PACKET_SIZE);
		return CUTE_CONNECT_TOKEN_PACKET_SIZE;
	}

	uint8_t* buffer_start = buffer;
	uint8_t* payload = s_header(&buffer, type, sequence);
	int payload_size = 0;

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED:
	{
		packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
		write_uint64(&buffer, packet->client_id);
		write_uint32(&buffer, packet->max_clients);
		write_uint32(&buffer, packet->connection_timeout);
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 8 + 4 + 4);
	}	break;

	case PACKET_TYPE_CONNECTION_DENIED:
	{
		packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_ptr;
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 0);
	}	break;

	case PACKET_TYPE_KEEPALIVE:
	{
		packet_keepalive_t* packet = (packet_keepalive_t*)packet_ptr;
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 0);
	}	break;

	case PACKET_TYPE_DISCONNECT:
	{
		packet_disconnect_t* packet = (packet_disconnect_t*)packet_ptr;
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 0);
	}	break;

	case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
	case PACKET_TYPE_CHALLENGE_RESPONSE:
	{
		packet_challenge_t* packet = (packet_challenge_t*)packet_ptr;
		write_uint64(&buffer, packet->challenge_nonce);
		CUTE_MEMCPY(buffer, packet->challenge_data, CUTE_CHALLENGE_DATA_SIZE);
		buffer += CUTE_CHALLENGE_DATA_SIZE;
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 8 + CUTE_CHALLENGE_DATA_SIZE);
	}	break;

	case PACKET_TYPE_PAYLOAD:
	{
		packet_payload_t* packet = (packet_payload_t*)packet_ptr;
		write_uint16(&buffer, packet->payload_size);
		CUTE_MEMCPY(buffer, packet->payload, packet->payload_size);
		buffer += packet->payload_size;
		payload_size = (int)(buffer - payload);
		CUTE_ASSERT(payload_size == 2 + packet->payload_size);
	}	break;
	}

	crypto_encrypt(key, payload, payload_size, sequence);

	size_t written = buffer - buffer_start;
	return (int)(written) + CUTE_CRYPTO_HEADER_BYTES;
}

void* packet_open(uint8_t* buffer, int size, const crypto_key_t* key, packet_allocator_t* pa, replay_buffer_t* replay_buffer, uint64_t* sequence_ptr)
{
	int ret = 0;
	uint8_t* buffer_start = buffer;
	uint8_t type = read_uint8(&buffer);

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED: CUTE_CHECK(size != 16 + 73); if (ret) return NULL; break;
	case PACKET_TYPE_CONNECTION_DENIED: CUTE_CHECK(size != 73); if (ret) return NULL; break;
	case PACKET_TYPE_KEEPALIVE: CUTE_CHECK(size != 73); if (ret) return NULL; break;
	case PACKET_TYPE_DISCONNECT: CUTE_CHECK(size != 73); if (ret) return NULL; break;
	case PACKET_TYPE_CHALLENGE_REQUEST: CUTE_CHECK(size != 264 + 73); if (ret) return NULL; break;
	case PACKET_TYPE_CHALLENGE_RESPONSE: CUTE_CHECK(size != 264 + 73); if (ret) return NULL; break;
	case PACKET_TYPE_PAYLOAD: CUTE_CHECK((size - 73 < 1) | (size - 73 > 1255)); if (ret) return NULL; break;
	}

	uint64_t sequence = read_uint64(&buffer);
	int bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read == 1 + 8);

	if (replay_buffer) {
		CUTE_CHECK(replay_buffer_cull_duplicate(replay_buffer, sequence));
		if (ret) return NULL;
	}

	buffer += CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES;
	bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read == 1 + 8 + CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES);
	if (crypto_decrypt(key, buffer, size - 37, sequence).is_error()) return NULL;

	if (replay_buffer) {
		replay_buffer_update(replay_buffer, sequence);
	}

	if (sequence_ptr) {
		*sequence_ptr = sequence;
	}

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED:
	{
		packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		packet->client_id = read_uint64(&buffer);
		packet->max_clients = read_uint32(&buffer);
		packet->connection_timeout = read_uint32(&buffer);
		return packet;
	}	break;

	case PACKET_TYPE_CONNECTION_DENIED:
	{
		packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		return packet;
	}	break;

	case PACKET_TYPE_KEEPALIVE:
	{
		packet_keepalive_t* packet = (packet_keepalive_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		return packet;
	}	break;

	case PACKET_TYPE_DISCONNECT:
	{
		packet_disconnect_t* packet = (packet_disconnect_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		return packet;
	}	break;

	case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
	case PACKET_TYPE_CHALLENGE_RESPONSE:
	{
		packet_challenge_t* packet = (packet_challenge_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		packet->challenge_nonce = read_uint64(&buffer);
		CUTE_MEMCPY(packet->challenge_data, buffer, CUTE_CHALLENGE_DATA_SIZE);
		return packet;
	}	break;

	case PACKET_TYPE_PAYLOAD:
	{
		packet_payload_t* packet = (packet_payload_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		packet->payload_size = read_uint16(&buffer);
		CUTE_MEMCPY(packet->payload, buffer, packet->payload_size);
		return packet;
	}	break;
	}

	return NULL;
}

// -------------------------------------------------------------------------------------------------

static int s_packet_size(packet_type_t type)
{
	int size = 0;

	switch (type)
	{
	case PACKET_TYPE_CONNECT_TOKEN:
		size = sizeof(packet_connect_token_t);
		break;

	case PACKET_TYPE_CONNECTION_ACCEPTED:
		size = sizeof(packet_connection_accepted_t);
		break;

	case PACKET_TYPE_CONNECTION_DENIED:
		size = sizeof(packet_connection_denied_t);
		break;

	case PACKET_TYPE_KEEPALIVE:
		size = sizeof(packet_keepalive_t);
		break;

	case PACKET_TYPE_DISCONNECT:
		size = sizeof(packet_disconnect_t);
		break;

	case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
	case PACKET_TYPE_CHALLENGE_RESPONSE:
		size = sizeof(packet_challenge_t);
		break;

	case PACKET_TYPE_PAYLOAD:
		size = sizeof(packet_payload_t);
		break;
	}

	return size;
}

struct packet_allocator_t
{
	memory_pool_t* pools[PACKET_TYPE_COUNT];
	void* user_allocator_context;
};

packet_allocator_t* packet_allocator_create(void* user_allocator_context)
{
	packet_allocator_t* packet_allocator = (packet_allocator_t*)CUTE_ALLOC(sizeof(packet_allocator_t), user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_CONNECT_TOKEN] = memory_pool_make(s_packet_size(PACKET_TYPE_CONNECT_TOKEN), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_CONNECTION_ACCEPTED] = memory_pool_make(s_packet_size(PACKET_TYPE_CONNECTION_ACCEPTED), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_CONNECTION_DENIED] = memory_pool_make(s_packet_size(PACKET_TYPE_CONNECTION_DENIED), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_KEEPALIVE] = memory_pool_make(s_packet_size(PACKET_TYPE_KEEPALIVE), 1024, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_DISCONNECT] = memory_pool_make(s_packet_size(PACKET_TYPE_DISCONNECT), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_CHALLENGE_REQUEST] = memory_pool_make(s_packet_size(PACKET_TYPE_CHALLENGE_REQUEST), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_CHALLENGE_RESPONSE] = memory_pool_make(s_packet_size(PACKET_TYPE_CHALLENGE_RESPONSE), 256, user_allocator_context);
	packet_allocator->pools[PACKET_TYPE_PAYLOAD] = memory_pool_make(s_packet_size(PACKET_TYPE_PAYLOAD), 1024 * 10, user_allocator_context);
	packet_allocator->user_allocator_context = user_allocator_context;
	return packet_allocator;
}

void packet_allocator_destroy(packet_allocator_t* packet_allocator)
{
	for (int i = 0; i < PACKET_TYPE_COUNT; ++i) {
		memory_pool_destroy(packet_allocator->pools[i]);
	}
	CUTE_FREE(packet_allocator, packet_allocator->user_allocator_context);
}

void* packet_allocator_alloc(packet_allocator_t* packet_allocator, packet_type_t type)
{
	if (!packet_allocator) {
		return CUTE_ALLOC(s_packet_size(type), NULL);
	} else {
		void* packet = memory_pool_alloc(packet_allocator->pools[type]);
		return packet;
	}
}

void packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet)
{
	if (!packet_allocator) {
		return CUTE_FREE(packet, NULL);
	} else {
		memory_pool_free(packet_allocator->pools[type], packet);
	}
}

// -------------------------------------------------------------------------------------------------

uint8_t* client_read_connect_token_from_web_service(uint8_t* buffer, uint64_t application_id, uint64_t current_time, connect_token_t* token)
{
	int ret = 0;
	uint8_t* buffer_start = buffer;

	// Read rest section.
	CUTE_CHECK(CUTE_STRNCMP((const char*)buffer, (const char*)CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN));
	buffer += CUTE_PROTOCOL_VERSION_STRING_LEN;
	CUTE_CHECK(read_uint64(&buffer) != application_id);
	token->creation_timestamp = read_uint64(&buffer);
	token->client_to_server_key = read_key(&buffer);
	token->server_to_client_key = read_key(&buffer);

	// Read public section.
	uint8_t* connect_token_packet = buffer;
	packet_connect_token_t packet;
	if (read_connect_token_packet_public_section(buffer, application_id, current_time, &packet).is_error()) return NULL;
	token->expiration_timestamp = packet.expiration_timestamp;
	token->handshake_timeout = packet.handshake_timeout;
	token->endpoint_count = packet.endpoint_count;
	CUTE_MEMCPY(token->endpoints, packet.endpoints, sizeof(endpoint_t) * token->endpoint_count);

	return ret ? NULL : connect_token_packet;
}

error_t CUTE_CALL server_decrypt_connect_token_packet(uint8_t* packet_buffer, const crypto_sign_public_t* pk, const crypto_sign_secret_t* sk, uint64_t application_id, uint64_t current_time, connect_token_decrypted_t* token)
{
	// Read public section.
	packet_connect_token_t packet;
	error_t err;
	err = read_connect_token_packet_public_section(packet_buffer, application_id, current_time, &packet);
	if (err.is_error()) return err;
	if (packet.expiration_timestamp <= current_time) return error_failure("Invalid timestamp.");
	token->expiration_timestamp = packet.expiration_timestamp;
	token->handshake_timeout = packet.handshake_timeout;
	token->endpoint_count = packet.endpoint_count;
	CUTE_MEMCPY(token->endpoints, packet.endpoints, sizeof(endpoint_t) * token->endpoint_count);
	CUTE_MEMCPY(token->signature.bytes, packet_buffer + 1024 - CUTE_PROTOCOL_SIGNATURE_SIZE, CUTE_PROTOCOL_SIGNATURE_SIZE);

	// Verify signature.
	if (crypto_sign_verify(pk, &token->signature, packet_buffer, 1024 - CUTE_PROTOCOL_SIGNATURE_SIZE).is_error()) return error_failure("Failed authentication.");

	// Decrypt the secret section.
	uint8_t* secret_section = packet_buffer + 568;
	uint8_t* additional_data = packet_buffer;

	if (crypto_decrypt((crypto_key_t*)sk, secret_section, CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE).is_error()) {
		return error_failure("Failed decryption.");
	}

	// Read secret section.
	secret_section += CUTE_PROTOCOL_SIGNATURE_SIZE - CUTE_CRYPTO_HEADER_BYTES;
	token->client_id = read_uint64(&secret_section);
	token->client_to_server_key = read_key(&secret_section);
	token->server_to_client_key = read_key(&secret_section);
	uint8_t* user_data = secret_section + CUTE_CRYPTO_HEADER_BYTES;
	CUTE_MEMCPY(token->user_data, user_data, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);

	return error_success();
}

// -------------------------------------------------------------------------------------------------

// TODO - Rename this hash table so it's obviously different than cute_hashtable.h -- the difference
// being one uses an unpredictable hash primitive from libhydrogen while the other uses a simple and
// fast hash function.

static CUTE_INLINE uint32_t s_is_prime(uint32_t x)
{
	if ((x == 2) | (x == 3)) return 1;
	if ((x % 2 == 0) | (x % 3 == 0)) return 0;

	uint32_t divisor = 6;
	while (divisor * divisor - 2 * divisor + 1 <= x)
	{
		if (x % (divisor - 1) == 0) return 0;
		if (x % (divisor + 1) == 0) return 0;
		divisor += 6;
	}

	return 1;
}

static CUTE_INLINE uint32_t s_next_prime(uint32_t a)
{
	while (1)
	{
		if (s_is_prime(a)) return a;
		else ++a;
	}
}

void hashtable_init(hashtable_t* table, int key_size, int item_size, int capacity, void* mem_ctx)
{
	CUTE_ASSERT(capacity);
	CUTE_MEMSET(table, 0, sizeof(hashtable_t));

	table->count = 0;
	table->slot_capacity = s_next_prime(capacity);
	table->key_size = key_size;
	table->item_size = item_size;
	int slots_size = (int)(table->slot_capacity * sizeof(*table->slots));
	table->slots = (hashtable_slot_t*)CUTE_ALLOC((size_t)slots_size, mem_ctx);
	CUTE_MEMSET(table->slots, 0, (size_t) slots_size);

	hydro_hash_keygen(table->secret_key);

	table->item_capacity = s_next_prime(capacity + capacity / 2);
	table->items_key = CUTE_ALLOC(table->item_capacity * (table->key_size + sizeof(*table->items_slot_index) + table->item_size) + table->item_size + table->key_size, mem_ctx);
	table->items_slot_index = (int*)((uint8_t*)table->items_key + table->item_capacity * table->key_size);
	table->items_data = (void*)(table->items_slot_index + table->item_capacity);
	table->temp_key = (void*)(((uintptr_t)table->items_data) + table->item_size * table->item_capacity);
	table->temp_item = (void*)(((uintptr_t)table->temp_key) + table->key_size);
	table->mem_ctx = mem_ctx;
}

void hashtable_cleanup(hashtable_t* table)
{
	CUTE_FREE(table->slots, mem_ctx);
	CUTE_FREE(table->items_key, mem_ctx);
}

static CUTE_INLINE int s_keys_equal(const hashtable_t* table, const void* a, const void* b)
{
	return !CUTE_MEMCMP(a, b, table->key_size);
}

static CUTE_INLINE void* s_get_key(const hashtable_t* table, int index)
{
	uint8_t* keys = (uint8_t*)table->items_key;
	return keys + index * table->key_size;
}

static CUTE_INLINE void* s_get_item(const hashtable_t* table, int index)
{
	uint8_t* items = (uint8_t*)table->items_data;
	return items + index * table->item_size;
}

static CUTE_INLINE uint64_t s_calc_hash(const hashtable_t* table, const void* key)
{
	uint8_t hash_bytes[CUTE_PROTOCOL_HASHTABLE_HASH_BYTES];
	if (hydro_hash_hash(hash_bytes, CUTE_PROTOCOL_HASHTABLE_HASH_BYTES, (const uint8_t*)key, table->key_size, CUTE_PROTOCOL_CONTEXT, table->secret_key) != 0) {
		CUTE_ASSERT(0);
		return -1;
	}

	uint64_t hash = *(uint64_t*)hash_bytes;
	return hash;
}

static int hashtable_internal_find_slot(const hashtable_t* table, const void* key)
{
	uint64_t hash = s_calc_hash(table, key);
	int base_slot = (int)(hash % (uint64_t)table->slot_capacity);
	int base_count = table->slots[base_slot].base_count;
	int slot = base_slot;

	while (base_count > 0)
	{
		uint64_t slot_hash = table->slots[slot].key_hash;

		if (slot_hash) {
			int slot_base = (int)(slot_hash % (uint64_t)table->slot_capacity);
			if (slot_base == base_slot) 
			{
				CUTE_ASSERT(base_count > 0);
				--base_count;
				const void* found_key = s_get_key(table, table->slots[slot].item_index);
				if (slot_hash == hash && s_keys_equal(table, found_key, key))
					return slot;
			}
		}
		slot = (slot + 1) % table->slot_capacity;
	}

	return -1;
}

void* hashtable_insert(hashtable_t* table, const void* key, const void* item)
{
	CUTE_ASSERT(hashtable_internal_find_slot(table, key) < 0);
	uint64_t hash = s_calc_hash(table, key);

	CUTE_ASSERT(table->count < table->slot_capacity);

	int base_slot = (int)(hash % (uint64_t)table->slot_capacity);
	int base_count = table->slots[base_slot].base_count;
	int slot = base_slot;
	int first_free = slot;
	while (base_count)
	{
		uint64_t slot_hash = table->slots[slot].key_hash;
		if (slot_hash == 0 && table->slots[first_free].key_hash != 0) first_free = slot;
		int slot_base = (int)(slot_hash % (uint64_t)table->slot_capacity);
		if (slot_base == base_slot) 
			--base_count;
		slot = (slot + 1) % table->slot_capacity;
	}

	slot = first_free;
	while (table->slots[slot].key_hash)
		slot = (slot + 1) % table->slot_capacity;

	CUTE_ASSERT(table->count < table->item_capacity);

	CUTE_ASSERT(!table->slots[slot].key_hash && (hash % (uint64_t)table->slot_capacity) == (uint64_t)base_slot);
	CUTE_ASSERT(hash);
	table->slots[slot].key_hash = hash;
	table->slots[slot].item_index = table->count;
	++table->slots[base_slot].base_count;

	void* item_dst = s_get_item(table, table->count);
	void* key_dst = s_get_key(table, table->count);
	CUTE_MEMCPY(item_dst, item, table->item_size);
	CUTE_MEMCPY(key_dst, key, table->key_size);
    table->items_slot_index[table->count] = slot;
	++table->count;

	return item_dst;
}

void hashtable_remove(hashtable_t* table, const void* key)
{
	int slot = hashtable_internal_find_slot(table, key);
	CUTE_ASSERT(slot >= 0);

	uint64_t hash = table->slots[slot].key_hash;
	int base_slot = (int)(hash % (uint64_t)table->slot_capacity);
	CUTE_ASSERT(hash);
	--table->slots[base_slot].base_count;
	table->slots[slot].key_hash = 0;

	int index = table->slots[slot].item_index;
	int last_index = table->count - 1;
	if (index != last_index)
	{
		void* dst_key = s_get_key(table, index);
		void* src_key = s_get_key(table, last_index);
		CUTE_MEMCPY(dst_key, src_key, (size_t)table->key_size);
		void* dst_item = s_get_item(table, index);
		void* src_item = s_get_item(table, last_index);
		CUTE_MEMCPY(dst_item, src_item, (size_t)table->item_size);
		table->items_slot_index[index] = table->items_slot_index[last_index];
		table->slots[table->items_slot_index[last_index]].item_index = index;
	}
	--table->count;
} 

void hashtable_clear(hashtable_t* table)
{
	table->count = 0;
	CUTE_MEMSET(table->slots, 0, sizeof(*table->slots) * table->slot_capacity);
}

void* hashtable_find(const hashtable_t* table, const void* key)
{
	int slot = hashtable_internal_find_slot(table, key);
	if (slot < 0) return 0;

	int index = table->slots[slot].item_index;
	return s_get_item(table, index);
}

int hashtable_count(const hashtable_t* table)
{
	return table->count;
}

void* hashtable_items(const hashtable_t* table)
{
	return table->items_data;
}

void* hashtable_keys(const hashtable_t* table)
{
	return table->items_key;
}

void hashtable_swap(hashtable_t* table, int index_a, int index_b)
{
	if (index_a < 0 || index_a >= table->count || index_b < 0 || index_b >= table->count) return;

	int slot_a = table->items_slot_index[index_a];
	int slot_b = table->items_slot_index[index_b];

	table->items_slot_index[index_a] = slot_b;
	table->items_slot_index[index_b] = slot_a;

	void* key_a = s_get_key(table, index_a);
	void* key_b = s_get_key(table, index_b);
	CUTE_MEMCPY(table->temp_key, key_a, table->key_size);
	CUTE_MEMCPY(key_a, key_b, table->key_size);
	CUTE_MEMCPY(key_b, table->temp_key, table->key_size);

	void* item_a = s_get_item(table, index_a);
	void* item_b = s_get_item(table, index_b);
	CUTE_MEMCPY(table->temp_item, item_a, table->item_size);
	CUTE_MEMCPY(item_a, item_b, table->item_size);
	CUTE_MEMCPY(item_b, table->temp_item, table->item_size);

	table->slots[slot_a].item_index = index_b;
	table->slots[slot_b].item_index = index_a;
}

// -------------------------------------------------------------------------------------------------

void connect_token_cache_init(connect_token_cache_t* cache, int capacity, void* mem_ctx)
{
	cache->capacity = capacity;
	hashtable_init(&cache->table, CUTE_PROTOCOL_SIGNATURE_SIZE, sizeof(connect_token_cache_entry_t), capacity, mem_ctx);
	list_init(&cache->list);
	list_init(&cache->free_list);
	cache->node_memory = (connect_token_cache_node_t*)CUTE_ALLOC(sizeof(connect_token_cache_node_t) * capacity, mem_ctx);

	for (int i = 0; i < capacity; ++i)
	{
		list_node_t* node = &cache->node_memory[i].node;
		list_init_node(node);
		list_push_front(&cache->free_list, node);
	}

	cache->mem_ctx = mem_ctx;
}

void connect_token_cache_cleanup(connect_token_cache_t* cache)
{
	hashtable_cleanup(&cache->table);
	CUTE_FREE(cache->node_memory, cache->mem_ctx);
}

connect_token_cache_entry_t* connect_token_cache_find(connect_token_cache_t* cache, const uint8_t* hmac_bytes)
{
	void* entry_ptr = hashtable_find(&cache->table, hmac_bytes);
	if (entry_ptr) {
		connect_token_cache_entry_t* entry = (connect_token_cache_entry_t*)entry_ptr;
		list_node_t* node = entry->node;
		list_remove(node);
		list_push_front(&cache->list, node);
		return entry;
	} else {
		return NULL;
	}
}

void connect_token_cache_add(connect_token_cache_t* cache, const uint8_t* hmac_bytes)
{
	connect_token_cache_entry_t entry;

	int table_count = hashtable_count(&cache->table);
	CUTE_ASSERT(table_count <= cache->capacity);
	if (table_count == cache->capacity) {
		list_node_t* oldest_node = list_pop_back(&cache->list);
		connect_token_cache_node_t* oldest_entry_node = CUTE_LIST_HOST(connect_token_cache_node_t, node, oldest_node);
		hashtable_remove(&cache->table, oldest_entry_node->signature.bytes);
		CUTE_MEMCPY(oldest_entry_node->signature.bytes, hmac_bytes, CUTE_PROTOCOL_SIGNATURE_SIZE);

		connect_token_cache_entry_t* entry_ptr = (connect_token_cache_entry_t*)hashtable_insert(&cache->table, hmac_bytes, &entry);
		CUTE_ASSERT(entry_ptr);
		entry_ptr->node = oldest_node;
		list_push_front(&cache->list, entry_ptr->node);
	} else {
		connect_token_cache_entry_t* entry_ptr = (connect_token_cache_entry_t*)hashtable_insert(&cache->table, hmac_bytes, &entry);
		CUTE_ASSERT(entry_ptr);
		entry_ptr->node = list_pop_front(&cache->free_list);
		connect_token_cache_node_t* entry_node = CUTE_LIST_HOST(connect_token_cache_node_t, node, entry_ptr->node);
		CUTE_MEMCPY(entry_node->signature.bytes, hmac_bytes, CUTE_PROTOCOL_SIGNATURE_SIZE);
		list_push_front(&cache->list, entry_ptr->node);
	}
}

// -------------------------------------------------------------------------------------------------

void encryption_map_init(encryption_map_t* map, void* mem_ctx)
{
	hashtable_init(&map->table, sizeof(endpoint_t), sizeof(encryption_state_t), CUTE_ENCRYPTION_STATES_MAX, mem_ctx);
}

void encryption_map_cleanup(encryption_map_t* map)
{
	hashtable_cleanup(&map->table);
}

void encryption_map_clear(encryption_map_t* map)
{
	hashtable_clear(&map->table);
}

int encryption_map_count(encryption_map_t* map)
{
	return hashtable_count(&map->table);
}

void encryption_map_insert(encryption_map_t* map, endpoint_t endpoint, const encryption_state_t* state)
{
	hashtable_insert(&map->table, &endpoint, state);
}

encryption_state_t* encryption_map_find(encryption_map_t* map, endpoint_t endpoint)
{
	void* ptr = hashtable_find(&map->table, &endpoint);
	if (ptr) {
		encryption_state_t* state = (encryption_state_t*)ptr;
		state->last_packet_recieved_time = 0;
		return state;
	} else {
		return NULL;
	}
}

void encryption_map_remove(encryption_map_t* map, endpoint_t endpoint)
{
	hashtable_remove(&map->table, &endpoint);
}

endpoint_t* encryption_map_get_endpoints(encryption_map_t* map)
{
	return (endpoint_t*)hashtable_keys(&map->table);
}

encryption_state_t* encryption_map_get_states(encryption_map_t* map)
{
	return (encryption_state_t*)hashtable_items(&map->table);
}

void encryption_map_look_for_timeouts_or_expirations(encryption_map_t* map, float dt, uint64_t time)
{
	int index = 0;
	int count = encryption_map_count(map);
	endpoint_t* endpoints = encryption_map_get_endpoints(map);
	encryption_state_t* states = encryption_map_get_states(map);

	while (index < count)
	{
		encryption_state_t* state = states + index;
		state->last_packet_recieved_time += dt;
		int timed_out = state->last_packet_recieved_time >= state->handshake_timeout;
		int expired = state->expiration_timestamp <= time;
		if (timed_out | expired) {
			encryption_map_remove(map, endpoints[index]);
			--count;
		} else {
			++index;
		}
	}
}

// -------------------------------------------------------------------------------------------------

static CUTE_INLINE const char* s_client_state_str(client_state_t state)
{
	switch (state)
	{
	case CLIENT_STATE_CONNECT_TOKEN_EXPIRED: return "CONNECT_TOKEN_EXPIRED";
	case CLIENT_STATE_INVALID_CONNECT_TOKEN: return "INVALID_CONNECT_TOKEN";
	case CLIENT_STATE_CONNECTION_TIMED_OUT: return "CONNECTION_TIMED_OUT";
	case CLIENT_STATE_CHALLENGED_RESPONSE_TIMED_OUT: return "CHALLENGED_RESPONSE_TIMED_OUT";
	case CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT: return "CONNECTION_REQUEST_TIMED_OUT";
	case CLIENT_STATE_CONNECTION_DENIED: return "CONNECTION_DENIED";
	case CLIENT_STATE_DISCONNECTED: return "DISCONNECTED";
	case CLIENT_STATE_SENDING_CONNECTION_REQUEST: return "SENDING_CONNECTION_REQUEST";
	case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE: return "SENDING_CHALLENGE_RESPONSE";
	case CLIENT_STATE_CONNECTED: return "CONNECTED";
	}

	return NULL;
}

static void s_client_set_state(client_t* client, client_state_t state)
{
	client->state = state;
	//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Client: Switching to state %s.", s_client_state_str(state));
}

client_t* client_make(uint16_t port, const char* web_service_address, uint64_t application_id, void* user_allocator_context)
{
	client_t* client = (client_t*)CUTE_ALLOC(sizeof(client_t), app->mem_ctx);
	CUTE_MEMSET(client, 0, sizeof(client_t));
	s_client_set_state(client, CLIENT_STATE_DISCONNECTED);
	client->application_id = application_id;
	client->mem_ctx = user_allocator_context;
	client->packet_queue = circular_buffer_make(CUTE_MB, client->mem_ctx);
	return client;
}

void client_destroy(client_t* client)
{
	// TODO: Detect if disconnect was not called yet.
	circular_buffer_free(&client->packet_queue);
	CUTE_FREE(client, client->app->mem_ctx);
}

struct payload_t
{
	uint64_t sequence;
	int size;
	void* data;
};

int client_connect(client_t* client, const uint8_t* connect_token)
{
	int ret = 0;
	uint8_t* connect_token_packet = client_read_connect_token_from_web_service(
		(uint8_t*)connect_token,
		client->application_id,
		client->current_time,
		&client->connect_token
	);
	if (!connect_token_packet) {
		s_client_set_state(client, CLIENT_STATE_INVALID_CONNECT_TOKEN);
		ret = -1;
	}

	CUTE_MEMCPY(client->connect_token_packet, connect_token_packet, CUTE_CONNECT_TOKEN_PACKET_SIZE);

	// CUTE_CHECK_POINTER(client->packet_allocator); TODO
	CUTE_CHECK(socket_init(&client->socket, ADDRESS_TYPE_IPV6, 0, CUTE_PROTOCOL_CLIENT_SEND_BUFFER_SIZE, CUTE_PROTOCOL_CLIENT_RECEIVE_BUFFER_SIZE));
	replay_buffer_init(&client->replay_buffer);

	client->server_endpoint_index = 0;

	client->last_packet_sent_time = CUTE_PROTOCOL_SEND_RATE;
	s_client_set_state(client, CLIENT_STATE_SENDING_CONNECTION_REQUEST);
	client->goto_next_server_tentative_state = CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT;

	return ret;
}

static CUTE_INLINE endpoint_t s_server_endpoint(client_t* client)
{
	return client->connect_token.endpoints[client->server_endpoint_index];
}

static CUTE_INLINE const char* s_packet_str(uint8_t type)
{
	switch (type)
	{
	case PACKET_TYPE_CONNECT_TOKEN: return "CONNECT_TOKEN";
	case PACKET_TYPE_CONNECTION_ACCEPTED: return "CONNECTION_ACCEPTED";
	case PACKET_TYPE_CONNECTION_DENIED: return "CONNECTION_DENIED";
	case PACKET_TYPE_KEEPALIVE: return "KEEPALIVE";
	case PACKET_TYPE_DISCONNECT: return "DISCONNECT";
	case PACKET_TYPE_CHALLENGE_REQUEST: return "CHALLENGE_REQUEST";
	case PACKET_TYPE_CHALLENGE_RESPONSE: return "CHALLENGE_RESPONSE";
	case PACKET_TYPE_PAYLOAD: return "PAYLOAD";
	}

	return NULL;
}

static void s_send(client_t* client, void* packet)
{
	int sz = packet_write(packet, client->buffer, client->sequence++, &client->connect_token.client_to_server_key);

	if (sz >= 73) {
		socket_send(&client->socket, s_server_endpoint(client), client->buffer, sz);
		client->last_packet_sent_time = 0;
		//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Client: Sent %s packet to server.", s_packet_str(*(uint8_t*)packet));
	}
}

static void s_disconnect(client_t* client, client_state_t state, int send_packets)
{
	void* packet = NULL;
	while (!client_get_packet(client, &packet, NULL, NULL)) {
		client_free_packet(client, packet);
	}

	if (send_packets) {
		packet_disconnect_t packet;
		packet.packet_type = PACKET_TYPE_DISCONNECT;
		for (int i = 0; i < CUTE_PROTOCOL_REDUNDANT_DISCONNECT_PACKET_COUNT; ++i)
		{
			s_send(client, &packet);
		}
	}

	socket_cleanup(&client->socket);
	circular_buffer_reset(&client->packet_queue);

	s_client_set_state(client, state);
}

void client_disconnect(client_t* client)
{
	if (client->state <= 0) return;
	s_disconnect(client, CLIENT_STATE_DISCONNECTED, 1);
}

static void s_receive_packets(client_t* client)
{
	uint8_t* buffer = client->buffer;

	while (1)
	{
		// Read packet from UDP stack, and open it.
		endpoint_t from;
		int sz = socket_receive(&client->socket, &from, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);
		if (!sz) break;

		if (!endpoint_equals(s_server_endpoint(client), from)) {
			continue;
		}

		if (sz < 73) {
			continue;
		}

		uint8_t type = *buffer;
		if (type > 7) {
			continue;
		}

		switch (type)
		{
		case PACKET_TYPE_CONNECT_TOKEN: // fall-thru
		case PACKET_TYPE_CHALLENGE_RESPONSE:
			continue;
		}

		uint64_t sequence = ~0;
		void* packet_ptr = packet_open(buffer, sz, &client->connect_token.server_to_client_key, NULL, &client->replay_buffer, &sequence);
		if (!packet_ptr) continue;

		// Handle packet based on client's current state.
		int free_packet = 1;
		int should_break = 0;

		switch (client->state)
		{
		case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
			if (type == PACKET_TYPE_CHALLENGE_REQUEST) {
				packet_challenge_t* packet = (packet_challenge_t*)packet_ptr;
				client->challenge_nonce = packet->challenge_nonce;
				CUTE_MEMCPY(client->challenge_data, packet->challenge_data, CUTE_CHALLENGE_DATA_SIZE);
				s_client_set_state(client, CLIENT_STATE_SENDING_CHALLENGE_RESPONSE);
				client->goto_next_server_tentative_state = CLIENT_STATE_CHALLENGED_RESPONSE_TIMED_OUT;
				client->last_packet_sent_time = CUTE_PROTOCOL_SEND_RATE;
				client->last_packet_recieved_time = 0;
			} else if (type == PACKET_TYPE_CONNECTION_DENIED) {
				client->goto_next_server = 1;
				client->goto_next_server_tentative_state = CLIENT_STATE_CONNECTION_DENIED;
				should_break = 1;
				//log(CUTE_LOG_LEVEL_WARNING, "Protocol Client: Received CONNECTION_DENIED packet, attempting to connect to next server.");
			}
			break;

		case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
			if (type == PACKET_TYPE_CONNECTION_ACCEPTED) {
				packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
				client->client_id = packet->client_id;
				client->max_clients = packet->max_clients;
				client->connection_timeout = (float)packet->connection_timeout;
				s_client_set_state(client, CLIENT_STATE_CONNECTED);
				client->last_packet_recieved_time = 0;
			} else if (type == PACKET_TYPE_CONNECTION_DENIED) {
				client->goto_next_server = 1;
				client->goto_next_server_tentative_state = CLIENT_STATE_CONNECTION_DENIED;
				should_break = 1;
				//log(CUTE_LOG_LEVEL_WARNING, "Protocol Client: Received CONNECTION_DENIED packet, attempting to connect to next server.");
			}
			break;

		case CLIENT_STATE_CONNECTED:
			if (type == PACKET_TYPE_PAYLOAD) {
				client->last_packet_recieved_time = 0;
				packet_payload_t* packet = (packet_payload_t*)packet_ptr;
				payload_t payload;
				payload.sequence = sequence;
				payload.size = packet->payload_size;
				payload.data = packet->payload;
				if (circular_buffer_push(&client->packet_queue, &payload, sizeof(payload_t)) < 0) {
					//log(CUTE_LOG_LEVEL_WARNING, "Protocol Client: Packet queue is full; dropped payload packet.");
					free_packet = 1;
				} else {
					free_packet = 0;
				}
			} else if (type == PACKET_TYPE_KEEPALIVE) {
				client->last_packet_recieved_time = 0;
			} else if (type == PACKET_TYPE_DISCONNECT) {
				//log(CUTE_LOG_LEVEL_WARNING, "Protocol Client: Received DISCONNECT packet from server.");
				if (free_packet) {
					packet_allocator_free(NULL, (packet_type_t)type, packet_ptr);
					free_packet = 0;
					packet_ptr = NULL;
				}
				s_disconnect(client, CLIENT_STATE_DISCONNECTED, 0);
				should_break = 1;
			}
			break;

		default:
			break;
		}

		if (free_packet) {
			packet_allocator_free(NULL, (packet_type_t)type, packet_ptr);
		}

		if (should_break) {
			break;
		}
	}
}

static void s_send_packets(client_t* client)
{
	switch (client->state)
	{
	case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
		if (client->last_packet_sent_time >= CUTE_PROTOCOL_SEND_RATE) {
			s_send(client, client->connect_token_packet);
		}
		break;

	case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
		if (client->last_packet_sent_time >= CUTE_PROTOCOL_SEND_RATE) {
			packet_challenge_t packet;
			packet.packet_type = PACKET_TYPE_CHALLENGE_RESPONSE;
			packet.challenge_nonce = client->challenge_nonce;
			CUTE_MEMCPY(packet.challenge_data, client->challenge_data, CUTE_CHALLENGE_DATA_SIZE);
			s_send(client, &packet);
		}
		break;

	case CLIENT_STATE_CONNECTED:
		if (client->last_packet_sent_time >= CUTE_PROTOCOL_SEND_RATE) {
			packet_keepalive_t packet;
			packet.packet_type = PACKET_TYPE_KEEPALIVE;
			s_send(client, &packet);
		}
		break;

	default:
		break;
	}
}

static int s_goto_next_server(client_t* client)
{
	if (client->server_endpoint_index + 1 == client->connect_token.endpoint_count) {
		s_disconnect(client, client->goto_next_server_tentative_state, 0);
		//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Client: Unable to connect to any server in the server list.");
		return 0;
	}

	int index = ++client->server_endpoint_index;
	
	//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Client: Unable to connect to server index %d; now attempting index %d.", index - 1, index);

	client->last_packet_recieved_time = 0;
	client->last_packet_sent_time = CUTE_PROTOCOL_SEND_RATE;
	client->goto_next_server = 0;
	circular_buffer_reset(&client->packet_queue);

	client->server_endpoint_index = index;
	s_client_set_state(client, CLIENT_STATE_SENDING_CONNECTION_REQUEST);

	return 1;
}

void client_update(client_t* client, float dt, uint64_t current_time)
{
	if (client->state <= 0) {
		return;
	}

	client->current_time = current_time;
	client->last_packet_recieved_time += dt;
	client->last_packet_sent_time += dt;

	s_receive_packets(client);
	s_send_packets(client);

	if (client->state <= 0) {
		return;
	}

	int timeout = client->last_packet_recieved_time >= client->connect_token.handshake_timeout;
	int is_handshake = client->state >= CLIENT_STATE_SENDING_CONNECTION_REQUEST && client->state <= CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
	if (is_handshake) {
		int expired = client->connect_token.expiration_timestamp <= client->current_time;
		if (expired) {
			s_disconnect(client, CLIENT_STATE_CONNECT_TOKEN_EXPIRED, 1);
		} else if (timeout | client->goto_next_server) {
			if (s_goto_next_server(client)) {
				return;
			}
			else if (client->state == CLIENT_STATE_SENDING_CONNECTION_REQUEST) {
				s_disconnect(client, CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, 1);
			} else if (client->state == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE) {
				s_disconnect(client, CLIENT_STATE_CHALLENGED_RESPONSE_TIMED_OUT, 1);
			}
		}
	} else { // CLIENT_STATE_CONNECTED
		CUTE_ASSERT(client->state == CLIENT_STATE_CONNECTED);
		timeout = client->last_packet_recieved_time >= client->connection_timeout;
		if (timeout) {
			s_disconnect(client, CLIENT_STATE_CONNECTION_TIMED_OUT, 1);
		}
	}
}

int client_get_packet(client_t* client, void** data, int* size, uint64_t* sequence)
{
	payload_t payload;
	if (circular_buffer_pull(&client->packet_queue, &payload, sizeof(payload_t)) < 0) {
		return -1;
	}

	if (sequence) *sequence = payload.sequence;
	if (size) *size = payload.size;
	*data = payload.data;

	return 0;
}

void client_free_packet(client_t* client, void* packet)
{
	packet_payload_t* payload_packet = (packet_payload_t*)((uint8_t*)packet - CUTE_OFFSET_OF(packet_payload_t, payload));
	packet_allocator_free(NULL, (packet_type_t)payload_packet->packet_type, payload_packet);
}

int client_send_data(client_t* client, const void* data, int size)
{
	if (size < 1) return -1;
	if (size > CUTE_PROTOCOL_PACKET_PAYLOAD_MAX) return -1;
	packet_payload_t packet;
	packet.packet_type = PACKET_TYPE_PAYLOAD;
	packet.payload_size = size;
	CUTE_MEMCPY(packet.payload, data, size);
	s_send(client, &packet);
	return 0;
}

client_state_t client_get_state(client_t* client)
{
	return client->state;
}

uint64_t client_get_id(client_t* client)
{
	return client->client_id;
}

uint32_t client_get_max_clients(client_t* client)
{
	return client->max_clients;
}

endpoint_t client_get_server_address(client_t* client)
{
	return s_server_endpoint(client);
}

uint16_t client_get_port(client_t* client)
{
	return client->socket.endpoint.port;
}

// -------------------------------------------------------------------------------------------------

server_t* server_make(uint64_t application_id, const crypto_sign_public_t* public_key, const crypto_sign_secret_t* secret_key, void* mem_ctx)
{
	server_t* server = (server_t*)CUTE_ALLOC(sizeof(server_t), mem_ctx);
	CUTE_MEMSET(server, 0, sizeof(server_t));

	server->running = 0;
	server->application_id = application_id;
	server->packet_allocator = packet_allocator_create(mem_ctx);
	server->event_queue = circular_buffer_make(CUTE_MB * 10, mem_ctx);
	server->public_key = *public_key;
	server->secret_key = *secret_key;
	server->mem_ctx = mem_ctx;

	return server;
}

void server_destroy(server_t* server)
{
	packet_allocator_destroy(server->packet_allocator);
	circular_buffer_free(&server->event_queue);
	CUTE_FREE(server, server->mem_ctx);
}

error_t server_start(server_t* server, const char* address, uint32_t connection_timeout)
{
	int cleanup_map = 0;
	int cleanup_cache = 0;
	int cleanup_handles = 0;
	int cleanup_socket = 0;
	int cleanup_endpoint_table = 0;
	int cleanup_client_id_table = 0;
	int ret = 0;
	encryption_map_init(&server->encryption_map, server->mem_ctx);
	cleanup_map = 1;
	connect_token_cache_init(&server->token_cache, CUTE_PROTOCOL_CONNECT_TOKEN_ENTRIES_MAX, server->mem_ctx);
	cleanup_cache = 1;
	if (socket_init(&server->socket, address, CUTE_PROTOCOL_SERVER_SEND_BUFFER_SIZE, CUTE_PROTOCOL_SERVER_RECEIVE_BUFFER_SIZE)) ret = -1;
	cleanup_socket = 1;
	hashtable_init(&server->client_endpoint_table, sizeof(endpoint_t), sizeof(uint64_t), CUTE_PROTOCOL_SERVER_MAX_CLIENTS, server->mem_ctx);
	cleanup_endpoint_table = 1;
	hashtable_init(&server->client_id_table, sizeof(uint64_t), sizeof(int), CUTE_PROTOCOL_SERVER_MAX_CLIENTS, server->mem_ctx);
	cleanup_client_id_table = 1;

	server->running = 1;
	server->challenge_nonce = 0;
	server->client_count = 0;
	server->connection_timeout = connection_timeout;

	if (ret) {
		if (cleanup_map) encryption_map_cleanup(&server->encryption_map);
		if (cleanup_cache) connect_token_cache_cleanup(&server->token_cache);
		if (cleanup_socket) socket_cleanup(&server->socket);
		if (cleanup_endpoint_table) hashtable_cleanup(&server->client_endpoint_table);
		if (cleanup_client_id_table) hashtable_cleanup(&server->client_id_table);
		return error_failure(NULL); // -- Change this when socket_init is changed to use error_t.
	}

	return error_success();
}

static CUTE_INLINE int s_server_event_pull(server_t* server, server_event_t* event)
{
	return circular_buffer_pull(&server->event_queue, event, sizeof(server_event_t));
}

static CUTE_INLINE int s_server_event_push(server_t* server, server_event_t* event)
{
	if (circular_buffer_push(&server->event_queue, event, sizeof(server_event_t)) < 0) {
		//log(CUTE_LOG_LEVEL_WARNING, "Protocol Server: Event queue is full; growing (doubling in size) to %d bytes", server->event_queue.capacity * 2);
		if (circular_buffer_grow(&server->event_queue, server->event_queue.capacity * 2) < 0) {
			return -1;
		}
		return circular_buffer_push(&server->event_queue, event, sizeof(server_event_t));
	} else {
		return 0;
	}
}

void server_stop(server_t* server)
{
	server->running = 0;

	// Free any lingering payload packets.
	while (1)
	{
		server_event_t event;
		if (s_server_event_pull(server, &event) < 0) break;
		if (event.type == SERVER_EVENT_PAYLOAD_PACKET) {
			server_free_packet(server, event.u.payload_packet.data);
		}
	}

	encryption_map_cleanup(&server->encryption_map);
	connect_token_cache_cleanup(&server->token_cache);
	socket_cleanup(&server->socket);
	hashtable_cleanup(&server->client_endpoint_table);
	hashtable_cleanup(&server->client_id_table);
	circular_buffer_reset(&server->event_queue);
}

bool server_running(server_t* server)
{
	return server->running ? true : false;
}

static void s_server_connect_client(server_t* server, endpoint_t endpoint, encryption_state_t* state)
{
	int index = -1;
	for (int i = 0; i < CUTE_PROTOCOL_SERVER_MAX_CLIENTS; ++i) {
		if (!server->client_is_connected[i]) {
			index = i;
			break;
		}
	}
	if (index == -1) return;

	server->client_count++;
	CUTE_ASSERT(server->client_count < CUTE_PROTOCOL_SERVER_MAX_CLIENTS);

	server_event_t event;
	event.type = SERVER_EVENT_NEW_CONNECTION;
	event.u.new_connection.client_index = index;
	event.u.new_connection.endpoint = endpoint;
	if (s_server_event_push(server, &event) < 0) return;

	hashtable_insert(&server->client_id_table, &state->client_id, &index);
	hashtable_insert(&server->client_endpoint_table, &endpoint, &state->client_id);

	server->client_id[index] = state->client_id;
	server->client_is_connected[index] = true;
	server->client_is_confirmed[index] = false;
	server->client_last_packet_received_time[index] = 0;
	server->client_last_packet_sent_time[index] = 0;
	server->client_endpoint[index] = endpoint;
	server->client_sequence[index] = state->sequence;
	server->client_client_to_server_key[index] = state->client_to_server_key;
	server->client_server_to_client_key[index] = state->server_to_client_key;
	replay_buffer_init(&server->client_replay_buffer[index]);

	connect_token_cache_add(&server->token_cache, state->signature.bytes);
	encryption_map_remove(&server->encryption_map, endpoint);

	packet_connection_accepted_t packet;
	packet.packet_type = PACKET_TYPE_CONNECTION_ACCEPTED;
	packet.client_id = state->client_id;
	packet.max_clients = CUTE_PROTOCOL_SERVER_MAX_CLIENTS;
	packet.connection_timeout = server->connection_timeout;
	if (packet_write(&packet, server->buffer, server->client_sequence[index]++, server->client_server_to_client_key + index) == 16 + 73) {
		socket_send(&server->socket, server->client_endpoint[index], server->buffer, 16 + 73);
		//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(packet.packet_type), server->client_id[index]);
	}
}

static void s_server_disconnect_sequence(server_t* server, uint32_t index)
{
	for (int i = 0; i < CUTE_DISCONNECT_REDUNDANT_PACKET_COUNT; ++i)
	{
		packet_disconnect_t packet;
		packet.packet_type = PACKET_TYPE_DISCONNECT;
		if (packet_write(&packet, server->buffer, server->client_sequence[index]++, server->client_server_to_client_key + index) == 73) {
			socket_send(&server->socket, server->client_endpoint[index], server->buffer, 73);
			//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(packet.packet_type), server->client_id[index]);
		}
	}
}

static void s_server_disconnect_client(server_t* server, uint32_t index, bool send_packets)
{
	//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Disconnecting client %" PRIu64 ".", server->client_id[index]);

	server_event_t event;
	event.type = SERVER_EVENT_DISCONNECTED;
	event.u.disconnected.client_index = index;
	if (s_server_event_push(server, &event) < 0) return;

	if (send_packets) {
		s_server_disconnect_sequence(server, index);
	}

	// Free client resources.
	server->client_is_confirmed[index] = 0;
	hashtable_remove(&server->client_id_table, server->client_id + index);
	hashtable_remove(&server->client_endpoint_table, server->client_endpoint + index);

	// Move client in back to the empty slot.
	int last_index = --server->client_count;
	if (last_index != index) {
		server->client_id[index]                        = server->client_id[last_index];
		server->client_is_connected[index]              = server->client_is_connected[last_index];
		server->client_is_confirmed[index]              = server->client_is_confirmed[last_index];
		server->client_last_packet_received_time[index] = server->client_last_packet_received_time[last_index];
		server->client_last_packet_sent_time[index]     = server->client_last_packet_sent_time[last_index];
		server->client_endpoint[index]                  = server->client_endpoint[last_index];
		server->client_sequence[index]                  = server->client_sequence[last_index];
		server->client_client_to_server_key[index]      = server->client_client_to_server_key[last_index];
		server->client_server_to_client_key[index]      = server->client_server_to_client_key[last_index];
		server->client_replay_buffer[index]             = server->client_replay_buffer[last_index];
	}
}

static void s_server_receive_packets(server_t* server)
{
	uint8_t* buffer = server->buffer;

	while (1)
	{
		endpoint_t from;
		int sz = socket_receive(&server->socket, &from, buffer, CUTE_PROTOCOL_PACKET_SIZE_MAX);
		if (!sz) break;

		if (sz < 73) {
			continue;
		}

		uint8_t type = *buffer;
		if (type > 7) {
			continue;
		}

		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED: // fall-thru
		case PACKET_TYPE_CONNECTION_DENIED: // fall-thru
		case PACKET_TYPE_CHALLENGE_REQUEST:
			continue;
		}

		if (type == PACKET_TYPE_CONNECT_TOKEN) {
			if (sz != 1024) {
				continue;
			}

			connect_token_decrypted_t token;
			if (server_decrypt_connect_token_packet(buffer, &server->public_key, &server->secret_key, server->application_id, server->current_time, &token).is_error()) {
				continue;
			}

			endpoint_t server_endpoint = server->socket.endpoint;
			int found = 0;
			for (int i = 0; i < token.endpoint_count; ++i)
			{
				if (endpoint_equals(server_endpoint, token.endpoints[i])) {
					found = 1;
					break;
				}
			}
			if (!found) continue;

			int endpoint_already_connected = !!hashtable_find(&server->client_endpoint_table, &from);
			if (endpoint_already_connected) continue;

			int client_id_already_connected = !!hashtable_find(&server->client_id_table, &token.client_id);
			if (client_id_already_connected) continue;

			int token_already_in_use = !!connect_token_cache_find(&server->token_cache, token.signature.bytes);
			if (token_already_in_use) continue;

			encryption_state_t* state = encryption_map_find(&server->encryption_map, from);
			if (!state) {
				encryption_state_t encryption_state;
				encryption_state.sequence = 0;
				encryption_state.expiration_timestamp = token.expiration_timestamp;
				encryption_state.handshake_timeout = token.handshake_timeout;
				encryption_state.last_packet_recieved_time = 0;
				encryption_state.last_packet_sent_time = CUTE_PROTOCOL_SEND_RATE;
				encryption_state.client_to_server_key = token.client_to_server_key;
				encryption_state.server_to_client_key = token.server_to_client_key;
				encryption_state.client_id = token.client_id;
				CUTE_MEMCPY(encryption_state.signature.bytes, token.signature.bytes, CUTE_PROTOCOL_SIGNATURE_SIZE);
				encryption_map_insert(&server->encryption_map, from, &encryption_state);
				state = encryption_map_find(&server->encryption_map, from);
				CUTE_ASSERT(state);
			}

			if (server->client_count == CUTE_PROTOCOL_SERVER_MAX_CLIENTS) {
				packet_connection_denied_t packet;
				packet.packet_type = PACKET_TYPE_CONNECTION_DENIED;
				if (packet_write(&packet, server->buffer, state->sequence++, &token.server_to_client_key) == 73) {
					socket_send(&server->socket, from, server->buffer, 73);
					//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to potential client (server is full).", s_packet_str(packet.packet_type));
				}
			}
		} else {
			uint64_t* client_id_ptr = (uint64_t*)hashtable_find(&server->client_endpoint_table, &from);
			replay_buffer_t* replay_buffer = NULL;
			const crypto_key_t* client_to_server_key;
			encryption_state_t* state = NULL;
			uint32_t index = ~0;

			int endpoint_already_connected = !!client_id_ptr;
			if (endpoint_already_connected) {
				if (type == PACKET_TYPE_CHALLENGE_RESPONSE) {
					// Someone already connected with this address.
					continue;
				}

				index = (uint32_t)*(int*)hashtable_find(&server->client_id_table, client_id_ptr);
				replay_buffer = server->client_replay_buffer + index;
				client_to_server_key = server->client_client_to_server_key + index;
			} else {
				state = encryption_map_find(&server->encryption_map, from);
				if (!state) continue;
				int connect_token_expired = state->expiration_timestamp <= server->current_time;
				if (connect_token_expired) {
					encryption_map_remove(&server->encryption_map, from);
					continue;
				}
				client_to_server_key = &state->client_to_server_key;
			}

			void* packet_ptr = packet_open(buffer, sz, client_to_server_key, server->packet_allocator, replay_buffer);
			if (!packet_ptr) continue;

			int free_packet = 1;

			switch (type)
			{
			case PACKET_TYPE_KEEPALIVE:
				CUTE_ASSERT(index != ~0);
				server->client_last_packet_received_time[index] = 0;
				if (!server->client_is_confirmed[index]) //log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Client %" PRIu64 " is now *confirmed*.", server->client_id[index]);
				server->client_is_confirmed[index] = 1;
				break;

			case PACKET_TYPE_DISCONNECT:
				CUTE_ASSERT(index != ~0);
				//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Client %" PRIu64 " has sent the server a DISCONNECT packet.", server->client_id[index]);
				s_server_disconnect_client(server, index, 0);
				break;

			case PACKET_TYPE_CHALLENGE_RESPONSE:
			{
				CUTE_ASSERT(!endpoint_already_connected);
				int client_id_already_connected = !!hashtable_find(&server->client_id_table, &state->client_id);
				if (client_id_already_connected) break;
				if (server->client_count == CUTE_PROTOCOL_SERVER_MAX_CLIENTS) {
					packet_connection_denied_t packet;
					packet.packet_type = PACKET_TYPE_CONNECTION_DENIED;
					if (packet_write(&packet, server->buffer, state->sequence++, &state->server_to_client_key) == 73) {
						socket_send(&server->socket, from, server->buffer, 73);
						//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to potential client (server is full).", s_packet_str(packet.packet_type));
					}
				} else {
					s_server_connect_client(server, from, state);
				}
			}	break;

			case PACKET_TYPE_PAYLOAD:
				CUTE_ASSERT(index != ~0);
				server->client_last_packet_received_time[index] = 0;
				if (!server->client_is_confirmed[index]) //log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Client %" PRIu64 " is now *confirmed*.", server->client_id[index]);
				server->client_is_confirmed[index] = 1;
				free_packet = 0;
				packet_payload_t* packet = (packet_payload_t*)packet_ptr;
				server_event_t event;
				event.type = SERVER_EVENT_PAYLOAD_PACKET;
				event.u.payload_packet.client_index = index;
				event.u.payload_packet.size = packet->payload_size;
				event.u.payload_packet.data = packet->payload;
				if (s_server_event_push(server, &event) < 0) {
					//log(CUTE_LOG_LEVEL_WARNING, "Protocol Server: Event queue is full; dropping payload packet for client %" PRIu64 ".", server->client_id[index]);
					free_packet = 1;
				}
				break;
			}

			if (free_packet) {
				packet_allocator_free(server->packet_allocator, (packet_type_t)type, packet_ptr);
			}
		}
	}
}

static void s_server_send_packets(server_t* server, float dt)
{
	CUTE_ASSERT(server->running);

	// Send challenge request packets.
	int state_count = encryption_map_count(&server->encryption_map);
	encryption_state_t* states = encryption_map_get_states(&server->encryption_map);
	endpoint_t* endpoints = encryption_map_get_endpoints(&server->encryption_map);
	uint8_t* buffer = server->buffer;
	socket_t* socket = &server->socket;
	for (int i = 0; i < state_count; ++i)
	{
		encryption_state_t* state = states + i;
		state->last_packet_sent_time += dt;

		if (state->last_packet_sent_time >= CUTE_PROTOCOL_SEND_RATE) {
			state->last_packet_sent_time = 0;

			packet_challenge_t packet;
			packet.packet_type = PACKET_TYPE_CHALLENGE_REQUEST;
			packet.challenge_nonce = server->challenge_nonce++;
			crypto_random_bytes(packet.challenge_data, sizeof(packet.challenge_data));

			if (packet_write(&packet, buffer, state->sequence++, &state->server_to_client_key) == 264 + 73) {
				socket_send(socket, endpoints[i], buffer, 264 + 73);
				//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to potential client %" PRIu64 ".", s_packet_str(packet.packet_type), state->client_id);
			}
		}
	}

	// Update client timers.
	int client_count = server->client_count;
	float* last_received_times = server->client_last_packet_received_time;
	float* last_sent_times = server->client_last_packet_sent_time;
	for (int i = 0; i < client_count; ++i)
	{
		last_received_times[i] += dt;
		last_sent_times[i] += dt;
	}

	// Send keepalive packets.
	//int* is_loopback = server->client_is_loopback;
	bool* confirmed = server->client_is_confirmed;
	uint64_t* sequences = server->client_sequence;
	crypto_key_t* server_to_client_keys = server->client_server_to_client_key;
	handle_t* ids = server->client_id;
	uint32_t connection_timeout = server->connection_timeout;
	for (int i = 0; i < client_count; ++i)
	{
		//if (!is_loopback[i]) {
			if (last_sent_times[i] >= CUTE_PROTOCOL_SEND_RATE) {
				last_sent_times[i] = 0;

				if (!confirmed[i]) {
					packet_connection_accepted_t packet;
					packet.packet_type = PACKET_TYPE_CONNECTION_ACCEPTED;
					packet.client_id = ids[i];
					packet.max_clients = CUTE_PROTOCOL_SERVER_MAX_CLIENTS;
					packet.connection_timeout = connection_timeout;
					if (packet_write(&packet, buffer, sequences[i]++, server_to_client_keys + i) == 16 + 73) {
						socket_send(socket, endpoints[i], buffer, 16 + 73);
						//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(packet.packet_type), server->client_id[i]);
					}
				}

				packet_keepalive_t packet;
				packet.packet_type = PACKET_TYPE_KEEPALIVE;
				if (packet_write(&packet, buffer, sequences[i]++, server_to_client_keys + i) == 73) {
					socket_send(socket, endpoints[i], buffer, 73);
					//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(packet.packet_type), server->client_id[i]);
				}
			}
		//}
	}
}

bool server_pop_event(server_t* server, server_event_t* event)
{
	return s_server_event_pull(server, event) ? true : false;
}

void server_free_packet(server_t* server, void* packet)
{
	packet_payload_t* payload_packet = (packet_payload_t*)((uint8_t*)packet - CUTE_OFFSET_OF(packet_payload_t, payload));
	packet_allocator_free(server->packet_allocator, (packet_type_t)payload_packet->packet_type, payload_packet);
}

void server_disconnect_client(server_t* server, int client_index)
{
	CUTE_ASSERT(server->client_count >= 1);
	s_server_disconnect_client(server, client_index, true);
}

error_t server_send_to_client(server_t* server, const void* packet, int size, int client_index)
{
	if (size < 1) return error_failure("`size` is negative.");
	if (size > CUTE_PROTOCOL_PACKET_PAYLOAD_MAX) return error_failure("`size` exceeds `CUTE_PROTOCOL_PACKET_PAYLOAD_MAX`.");
	CUTE_ASSERT(server->client_count >= 1 && client_index >= 0 && client_index < CUTE_PROTOCOL_SERVER_MAX_CLIENTS);

	int index = client_index;
	if (!server->client_is_confirmed[index]) {
		packet_connection_accepted_t packet;
		packet.packet_type = PACKET_TYPE_CONNECTION_ACCEPTED;
		packet.client_id = server->client_id[index];
		packet.max_clients = CUTE_PROTOCOL_SERVER_MAX_CLIENTS;
		packet.connection_timeout = server->connection_timeout;
		if (packet_write(&packet, server->buffer, server->client_sequence[index]++, server->client_server_to_client_key + index) == 16 + 73) {
			socket_send(&server->socket, server->client_endpoint[index], server->buffer, 16 + 73);
			//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(packet.packet_type), server->client_id[index]);
			server->client_last_packet_sent_time[index] = 0;
		} else {
			return error_failure("Failed to write packet.");
		}
	}

	packet_payload_t payload;
	payload.packet_type = PACKET_TYPE_PAYLOAD;
	payload.payload_size = size;
	CUTE_MEMCPY(payload.payload, packet, size);
	int sz = packet_write(&payload, server->buffer, server->client_sequence[index]++, server->client_server_to_client_key + index);
	if (sz > 73) {
		socket_send(&server->socket, server->client_endpoint[index], server->buffer, sz);
		//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Sent %s to client %" PRIu64 ".", s_packet_str(payload.packet_type), server->client_id[index]);
		server->client_last_packet_sent_time[index] = 0;
	} else {
		return error_failure("Failed to write packet.");
	}

	return error_success();
}

static void s_server_look_for_timeouts(server_t* server)
{
	int client_count = server->client_count;
	float* last_received_times = server->client_last_packet_received_time;
	for (int i = 0; i < client_count;)
	{
		if (last_received_times[i] >= (float)server->connection_timeout) {
			--client_count;
			//log(CUTE_LOG_LEVEL_INFORMATIONAL, "Protocol Server: Client %" PRIu64 " has timed out.", server->client_id[i]);
			server_disconnect_client(server, i);
		} else {
			 ++i;
		}
	}
}

void server_update(server_t* server, float dt, uint64_t current_time)
{
	server->current_time = current_time;

	s_server_receive_packets(server);
	s_server_send_packets(server, dt);
	s_server_look_for_timeouts(server);
}

int server_client_count(server_t* server)
{
	return server->client_count;
}

uint64_t server_get_client_id(server_t* server, int client_index)
{
	CUTE_ASSERT(server->client_count >= 1 && client_index >= 0 && client_index < CUTE_PROTOCOL_SERVER_MAX_CLIENTS);
	return server->client_id[client_index];
}

}
}
