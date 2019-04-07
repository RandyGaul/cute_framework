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

#include <internal/cute_defines_internal.h>
#include <internal/cute_serialize_internal.h>
#include <internal/cute_protocol_internal.h>

namespace cute
{
namespace protocol
{

int generate_connect_token(
	uint32_t application_id,
	uint64_t creation_timestamp,
	const crypto_key_t* client_to_server_key,
	const crypto_key_t* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const crypto_key_t* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	CUTE_ASSERT(address_count >= 1 && address_count <= 32);
	CUTE_STATIC_ASSERT(sizeof(crypto_key_t) == 32, "Cute Protocol standard calls for encryption keys to be 32 bytes.");
	CUTE_STATIC_ASSERT(CUTE_CRYPTO_HMAC_BYTES == 16, "Cute Protocol standard calls for `HMAC bytes` to be 16 bytes.");

	uint8_t* token_start = token_ptr_out;
	uint8_t** p = &token_ptr_out;

	// Write the REST SECTION.
	write_bytes(p, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(p, application_id);
	write_uint64(p, creation_timestamp);
	write_bytes(p, (const uint8_t*)client_to_server_key, 32);
	write_bytes(p, (const uint8_t*)server_to_client_key, 32);

	// Write the PUBLIC SECTION.
	write_uint8(p, 0);
	write_bytes(p, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(p, application_id);
	write_uint64(p, expiration_timestamp);
	write_uint32(p, handshake_timeout);
	write_uint32(p, (uint32_t)address_count);
	for (int i = 0; i < address_count; ++i)
	{
		endpoint_t endpoint;
		CUTE_CHECK(endpoint_init(&endpoint, address_list[i]));
		write_endpoint(p, endpoint);
	}

	int bytes_written = (int)(*p - token_start);
	CUTE_ASSERT(bytes_written <= 656);
	int zeroes = 656 - bytes_written;
	for (int i = 0; i < zeroes; ++i)
		write_uint8(p, 0);

	bytes_written = (int)(*p - token_start);
	CUTE_ASSERT(bytes_written == 656);

	// Write the connect token nonce.
	uint8_t* big_nonce = *p;
	crypto_random_bytes(big_nonce, 24);
	*p += 24;

	// Write the SECRET SECTION.
	write_uint64(p, client_id);
	write_bytes(p, (const uint8_t*)client_to_server_key, 32);
	write_bytes(p, (const uint8_t*)server_to_client_key, 32);
	if (user_data) {
		CUTE_MEMCPY(*p, user_data, 256);
	} else {
		CUTE_MEMSET(*p, 0, 256);
	}
	*p += 256;
	bytes_written = (int)(*p - token_start);
	CUTE_ASSERT(bytes_written == 1024 - 16);

	CUTE_CHECK(crypto_encrypt_bignonce(shared_secret_key, token_start + 656 + 24, CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE, token_start, 656, big_nonce));
	CUTE_ASSERT(bytes_written + 16 == 1024);

	return 0;

cute_error:
	return -1;
}

// -------------------------------------------------------------------------------------------------

void packet_queue_init(packet_queue_t* q)
{
	CUTE_PLACEMENT_NEW(q) packet_queue_t;
}

int packet_queue_push(packet_queue_t* q, void* packet, packet_type_t type)
{
	if (q->count >= CUTE_PACKET_QUEUE_MAX_ENTRIES) {
		return -1;
	} else {
		q->count++;
		q->types[q->index1] = type;
		q->packets[q->index1] = packet;
		q->index1 = (q->index1 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

int packet_queue_pop(packet_queue_t* q, void** packet, packet_type_t* type)
{
	if (q->count <= 0) {
		return -1;
	} else {
		q->count--;
		*type = q->types[q->index0];
		*packet = q->packets[q->index0];
		q->index0 = (q->index0 + 1) % CUTE_PACKET_QUEUE_MAX_ENTRIES;
		return 0;
	}
}

// -------------------------------------------------------------------------------------------------

void replay_buffer_init(replay_buffer_t* buffer)
{
    buffer->max = 0;
    CUTE_MEMSET(buffer->entries, ~0, sizeof(uint64_t) * CUTE_REPLAY_BUFFER_SIZE);
}

int replay_buffer_cull_duplicate(replay_buffer_t* buffer, uint64_t sequence)
{
    if (sequence + CUTE_REPLAY_BUFFER_SIZE < buffer->max) {
        // This is UDP - just drop old packets.
        return -1;
    }

    int index = (int)(sequence % CUTE_REPLAY_BUFFER_SIZE);
    uint64_t val = buffer->entries[index];
    int empty_slot = val == ~0ULL;
    int outdated = val >= sequence;
    if (empty_slot | !outdated) {
        return 0;
    } else {
        // Duplicate or replayed packet detected.
        return -1;
    }
}

void replay_buffer_update(replay_buffer_t* buffer, uint64_t sequence)
{
    if (buffer->max < sequence) {
        buffer->max = sequence;
    }

    int index = (int)(sequence % CUTE_REPLAY_BUFFER_SIZE);
    uint64_t val = buffer->entries[index];
    int empty_slot = val == ~0ULL;
    int outdated = val >= sequence;
    if (empty_slot | !outdated) {
        buffer->entries[index] = sequence;
    }
}

// -------------------------------------------------------------------------------------------------

#define CUTE_ADDITIONAL_DATA_SIZE (CUTE_PROTOCOL_VERSION_STRING_LEN + sizeof(uint64_t) + 1)
static CUTE_INLINE void s_write_header(uint8_t* header, packet_type_t packet_type, uint64_t game_id)
{
	write_uint8(&header, (uint8_t)packet_type);
	write_bytes(&header, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(&header, game_id);
}

static uint64_t s_read_uint64(uint8_t** buffer)
{
	uint64_t val = *(uint64_t*)*buffer;
	*buffer += sizeof(uint64_t);
	return val;
}

void* packet_open(packet_allocator_t* pa, replay_buffer_t* nonce_buffer, uint64_t game_id, uint64_t timestamp, uint8_t* buffer, int size, uint64_t sequence_offset, const crypto_key_t* key, int is_server, packet_type_t* packet_type)
{
	uint8_t* buffer_start = buffer;
	packet_type_t type = (packet_type_t)read_uint8(&buffer);
	*packet_type = type;

	// Filter out invalid packet types.
	if (is_server) {
		switch (type)
		{
		default: return NULL;
		case PACKET_TYPE_CONNECTION_REQUEST: break;
		case PACKET_TYPE_KEEPALIVE: break;
		case PACKET_TYPE_DISCONNECT: break;
		case PACKET_TYPE_CHALLENGE_RESPONSE: break;
		case PACKET_TYPE_USERDATA: break;
		}
	} else {
		switch (type)
		{
		default: return NULL;
		case PACKET_TYPE_CONNECTION_ACCEPTED: break;
		case PACKET_TYPE_CONNECTION_DENIED: break;
		case PACKET_TYPE_KEEPALIVE: break;
		case PACKET_TYPE_DISCONNECT: break;
		case PACKET_TYPE_CHALLENGE_REQUEST: break;
		case PACKET_TYPE_USERDATA: break;
		}
	}

	if (type == PACKET_TYPE_CONNECTION_REQUEST) {
		CUTE_CHECK(size != CUTE_PACKET_SIZE_MAX);

		// Read in and verify the header.
		CUTE_CHECK(read_uint8(&buffer) != 'C');
		CUTE_CHECK(read_uint8(&buffer) != 'U');
		CUTE_CHECK(read_uint8(&buffer) != 'T');
		CUTE_CHECK(read_uint8(&buffer) != 'E');
		CUTE_CHECK(read_uint8(&buffer) != ' ');
		CUTE_CHECK(read_uint8(&buffer) != '1');
		CUTE_CHECK(read_uint8(&buffer) != '.');
		CUTE_CHECK(read_uint8(&buffer) != '0');
		CUTE_CHECK(read_uint8(&buffer) != '\0');
		uint64_t game_id_from_packet = read_uint64(&buffer);
		CUTE_CHECK(game_id_from_packet != game_id);
		uint8_t nonce[CUTE_CRYPTO_NONCE_BYTES];
		read_bytes(&buffer, nonce, CUTE_CRYPTO_NONCE_BYTES);
		uint64_t expire_timestamp = read_uint64(&buffer);
		CUTE_CHECK(expire_timestamp > timestamp);

		// Decrypt secret section of the connect token.
		uint8_t* additional_data = buffer_start;
		int additional_data_size = ~0; // FIXME (was CUTE_CONNECT_TOKEN_HEADER_SIZE or something dumb).

		int secret_size = CUTE_PACKET_SIZE_MAX - additional_data_size;
		uint8_t* secret_data = buffer_start + additional_data_size;

		CUTE_CHECK(crypto_decrypt_bignonce(key, secret_data, secret_size, additional_data, additional_data_size, nonce));
		secret_size -= CUTE_CRYPTO_HMAC_BYTES;

		// Read the token secret data.
		packet_decrypted_connect_token_t* token = (packet_decrypted_connect_token_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_REQUEST);
		token->client_id = read_uint64(&secret_data);
		token->expire_timestamp = expire_timestamp;
		token->sequence_offset = read_uint64(&secret_data);
		read_bytes(&secret_data, token->key.key, sizeof(crypto_key_t));
		read_bytes(&secret_data, token->user_data, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);

		// Read the token public data.
		uint16_t endpoint_count = read_uint16(&additional_data);
		token->endpoint_count = endpoint_count;
		for (int i = 0; i < endpoint_count; ++i)
		{
			address_type_t type = (address_type_t)read_uint8(&additional_data);
			if(type == ADDRESS_TYPE_NONE) {
				packet_allocator_free(pa, PACKET_TYPE_CHALLENGE_REQUEST, token);
				return NULL;
			}
			token->endpoints[i].type = type;
			read_bytes(&additional_data, (uint8_t*)(&token->endpoints[i].u), type == ADDRESS_TYPE_IPV4 ? sizeof(endpoint_t::u.ipv4) : sizeof(endpoint_t::u.ipv6));
			token->endpoints[i].port = read_uint16(&additional_data);
		}

		return token;
	} else {
		uint8_t additional_data[CUTE_ADDITIONAL_DATA_SIZE];
		s_write_header(additional_data, type, game_id);
		uint64_t sequence = read_uint64(&buffer);
		if (crypto_decrypt(key, buffer, size - sizeof(uint64_t) - 1, additional_data, CUTE_ADDITIONAL_DATA_SIZE, sequence + sequence_offset) < 0) {
			// Forged packet!
			return NULL;
		}

		if (replay_buffer_cull_duplicate(nonce_buffer, sequence) < 0) {
			// Duplicate, or very old, packet detected.
			return NULL;
		}

		switch (type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
		{
			packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_ACCEPTED);
			packet->client_number = read_uint32(&buffer);
			packet->max_clients = read_uint32(&buffer);
			return packet;
		}	break;

		case PACKET_TYPE_CONNECTION_DENIED:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_CONNECTION_DENIED);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_KEEPALIVE:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_KEEPALIVE);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_DISCONNECT:
		{
			packet_connection_denied_t* packet = (packet_connection_denied_t*)packet_allocator_alloc(pa, PACKET_TYPE_DISCONNECT);
			packet->packet_type = (uint8_t)type;
			return packet;
		}	break;

		case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
		case PACKET_TYPE_CHALLENGE_RESPONSE:
		{
			packet_challenge_t* packet = (packet_challenge_t*)packet_allocator_alloc(pa, PACKET_TYPE_CHALLENGE_REQUEST);
			packet->nonce = read_uint64(&buffer);
			CUTE_MEMCPY(packet->challenge_data, buffer, sizeof(packet->challenge_data));
			return packet;
		}	break;

		case PACKET_TYPE_USERDATA:
		{
			packet_userdata_t* packet = (packet_userdata_t*)packet_allocator_alloc(pa, PACKET_TYPE_USERDATA);
			packet->size = read_uint32(&buffer);
			CUTE_MEMSET(packet->data, 0, sizeof(packet->data));
			CUTE_MEMCPY(packet->data, buffer, packet->size);
			return packet;
		}	break;

		default: return NULL;
		}
	}

cute_error:
	return NULL;
}

int packet_write(void* packet_ptr, packet_type_t packet_type, uint8_t* buffer, uint64_t game_id, uint64_t sequence, const crypto_key_t* key)
{
	uint8_t* buffer_start = buffer;

	if (packet_type == PACKET_TYPE_CONNECTION_REQUEST) {
		packet_encrypted_connect_token_t* packet = (packet_encrypted_connect_token_t*)packet_ptr;
		//write_uint8(&buffer, (uint8_t)PACKET_TYPE_CONNECTION_REQUEST);
		//write_bytes(&buffer, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
		//write_uint64(&buffer, game_id);
		//write_bytes(&buffer, packet->nonce, sizeof(packet->nonce));
		//write_uint64(&buffer, packet->expire_timestamp);
		//write_bytes(&buffer, packet->secret_data, sizeof(packet->secret_data));
		int packet_size = (int)(buffer - buffer_start);
		//CUTE_ASSERT(packet_size == CUTE_PACKET_SIZE_MAX);
		return packet_size;
	} else {
		write_uint8(&buffer, (uint8_t)packet_type);
		write_uint64(&buffer, sequence);

		uint8_t* payload_start = buffer;
		int payload_size = 0;
		switch (packet_type)
		{
		case PACKET_TYPE_CONNECTION_ACCEPTED:
		{
			packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
			write_uint32(&buffer, packet->client_number);
			write_uint32(&buffer, packet->max_clients);
		}	break;

		case PACKET_TYPE_CONNECTION_DENIED: // fall-thru
		case PACKET_TYPE_KEEPALIVE: // fall-thru
		case PACKET_TYPE_DISCONNECT:
		{
			// No payload data.
		}	break;

		case PACKET_TYPE_CHALLENGE_REQUEST: // fall-thru
		case PACKET_TYPE_CHALLENGE_RESPONSE:
		{
			packet_challenge_t* packet = (packet_challenge_t*)packet_ptr;
			write_uint64(&buffer, packet->nonce);
			write_bytes(&buffer, packet->challenge_data, sizeof(packet->challenge_data));
		}	break;

		case PACKET_TYPE_USERDATA:
		{
			packet_userdata_t* packet = (packet_userdata_t*)packet_ptr;
			CUTE_ASSERT(packet->size <= CUTE_PACKET_PAYLOAD_MAX);
			CUTE_MEMCPY(buffer, packet->data, packet->size);
			buffer += packet->size;
		}	break;

		default:
			error_set("Attempted to write invalid packet type.");
			CUTE_ASSERT(0);
			return -1;
		}

		uint8_t additional_data[CUTE_ADDITIONAL_DATA_SIZE];
		s_write_header(additional_data, packet_type, game_id);

		if (crypto_encrypt(key, payload_start, payload_size, additional_data, CUTE_ADDITIONAL_DATA_SIZE, sequence) < 0) {
			return -1;
		}

		buffer += CUTE_CRYPTO_HMAC_BYTES;

		int packet_size = (int)(buffer - buffer_start);
		CUTE_ASSERT(packet_size <= CUTE_PACKET_SIZE_MAX);
		return packet_size;
	}
}

// -------------------------------------------------------------------------------------------------

int connect_token_open(connect_token_t* token, uint8_t* buffer)
{
	return -1;
}

// -------------------------------------------------------------------------------------------------


packet_allocator_t* packet_allocator_make(void* user_allocator_context)
{
	return NULL;
}

void packet_allocator_destroy(packet_allocator_t* packet_allocator)
{
}

void* packet_allocator_alloc(packet_allocator_t* packet_allocator, packet_type_t type)
{
	return NULL;
}

void packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet)
{
}

}
}
