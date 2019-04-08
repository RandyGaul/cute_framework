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

#include <libsodium/sodium.h>

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

CUTE_STATIC_ASSERT(sizeof(crypto_key_t) == 32, "Cute Protocol standard calls for encryption keys to be 32 bytes.");
CUTE_STATIC_ASSERT(CUTE_CRYPTO_HMAC_BYTES == 16, "Cute Protocol standard calls for `HMAC bytes` to be 16 bytes.");
CUTE_STATIC_ASSERT(CUTE_PROTOCOL_VERSION_STRING_LEN == 10, "Cute Protocol standard calls for the version string to be 10 bytes.");
CUTE_STATIC_ASSERT(CUTE_CONNECT_TOKEN_PACKET_SIZE == 1024, "Cute Protocol standard calls for connect token packet to be exactly 1024 bytes.");

int generate_connect_token(
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
	const crypto_key_t* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	CUTE_ASSERT(address_count >= 1 && address_count <= 32);

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
		CUTE_CHECK(endpoint_init(&endpoint, address_list[i]));
		write_endpoint(p, endpoint);
	}

	int bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written <= 656);
	int zeroes = 656 - bytes_written;
	for (int i = 0; i < zeroes; ++i)
		write_uint8(p, 0);

	bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written == 656);

	// Write the connect token nonce.
	uint8_t* big_nonce = *p;
	crypto_random_bytes(big_nonce, CUTE_CONNECT_TOKEN_NONCE_SIZE);
	*p += CUTE_CONNECT_TOKEN_NONCE_SIZE;

	// Write the SECRET SECTION.
	uint8_t* secret_section = *p;
	write_uint64(p, client_id);
	write_key(p, client_to_server_key);
	write_key(p, server_to_client_key);
	if (user_data) {
		CUTE_MEMCPY(*p, user_data, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);
	} else {
		CUTE_MEMSET(*p, 0, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);
	}
	*p += CUTE_CONNECT_TOKEN_USER_DATA_SIZE;
	bytes_written = (int)(*p - public_section);
	CUTE_ASSERT(bytes_written == CUTE_CONNECT_TOKEN_PACKET_SIZE - CUTE_CRYPTO_HMAC_BYTES);

	CUTE_CHECK(crypto_encrypt_bignonce(shared_secret_key, secret_section, CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE, public_section, 656, big_nonce));
	CUTE_ASSERT(bytes_written + CUTE_CRYPTO_HMAC_BYTES == CUTE_CONNECT_TOKEN_PACKET_SIZE);

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

int read_connect_token_packet_public_section(uint8_t* buffer, uint64_t application_id, uint64_t current_time, packet_connect_token_t* packet)
{
	uint8_t* buffer_start = buffer;

	// Read public section.
	packet->packet_type = (packet_type_t)read_uint8(&buffer);
	CUTE_CHECK(packet->packet_type != PACKET_TYPE_CONNECT_TOKEN);
	CUTE_CHECK(CUTE_STRNCMP((const char*)buffer, (const char*)CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN));
	buffer += CUTE_PROTOCOL_VERSION_STRING_LEN;
	CUTE_CHECK(read_uint64(&buffer) != application_id);
	packet->expiration_timestamp = read_uint64(&buffer);
	CUTE_CHECK(packet->expiration_timestamp < current_time);
	packet->handshake_timeout = read_uint32(&buffer);
	packet->endpoint_count = read_uint32(&buffer);
	int count = (int)packet->endpoint_count;
	CUTE_CHECK(count <= 0 || count > 32);
	for (int i = 0; i < count; ++i)
		packet->endpoints[i] = read_endpoint(&buffer);
	int bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read <= 656);
	buffer += 656 - bytes_read;
	bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read == 656);

	return 0;

cute_error:
	return -1;
}

static CUTE_INLINE uint8_t* s_header(uint8_t** p, uint8_t type, uint64_t sequence)
{
	write_uint8(p, type);
	write_uint64(p, sequence);
	return *p;
}

static CUTE_INLINE void s_associated_data(uint8_t** p, uint8_t type, uint64_t application_id)
{
	write_uint8(p, type);
	write_bytes(p, CUTE_PROTOCOL_VERSION_STRING, CUTE_PROTOCOL_VERSION_STRING_LEN);
	write_uint64(p, application_id);
}

int packet_write(void* packet_ptr, uint8_t* buffer, uint64_t application_id, uint64_t sequence, const crypto_key_t* key)
{
	uint8_t type = *(uint8_t*)packet_ptr;
	uint8_t* buffer_start = buffer;
	uint8_t* payload = s_header(&buffer, type, sequence);
	int payload_size = 0;

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED:
	{
		packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_ptr;
		write_uint64(&buffer, packet->client_handle);
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
		write_uint64(&buffer, packet->nonce);
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

	uint8_t associated_data[1 + CUTE_PROTOCOL_VERSION_STRING_LEN + 8];
	uint8_t* ad_ptr = associated_data;
	s_associated_data(&ad_ptr, type, application_id);

	CUTE_CHECK(crypto_encrypt(key, payload, payload_size, associated_data, sizeof(associated_data), sequence));

	return (int)(buffer - buffer_start) + CUTE_CRYPTO_HMAC_BYTES;

cute_error:
	return -1;
}

void* packet_open(uint8_t* buffer, int size, const crypto_key_t* key, uint64_t application_id, packet_allocator_t* pa, replay_buffer_t* replay_buffer)
{
	uint8_t* buffer_start = buffer;
	uint8_t type = read_uint8(&buffer);

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED: CUTE_CHECK(size != 16 + 25); break;
	case PACKET_TYPE_CONNECTION_DENIED: CUTE_CHECK(size != 25); break;
	case PACKET_TYPE_KEEPALIVE: CUTE_CHECK(size != 25); break;
	case PACKET_TYPE_DISCONNECT: CUTE_CHECK(size != 25); break;
	case PACKET_TYPE_CHALLENGE_REQUEST: CUTE_CHECK(size != 264 + 25); break;
	case PACKET_TYPE_CHALLENGE_RESPONSE: CUTE_CHECK(size != 264 + 25); break;
	case PACKET_TYPE_PAYLOAD: CUTE_CHECK(size - 25 < 1 || size - 25 > 1255); break;
	}

	uint64_t sequence = read_uint64(&buffer);
	int bytes_read = (int)(buffer - buffer_start);
	CUTE_ASSERT(bytes_read == 1 + 8);

	if (replay_buffer) {
		CUTE_CHECK(replay_buffer_cull_duplicate(replay_buffer, sequence));
	}

	uint8_t associated_data[1 + CUTE_PROTOCOL_VERSION_STRING_LEN + 8];
	uint8_t* ad_ptr = associated_data;
	s_associated_data(&ad_ptr, type, application_id);

	CUTE_CHECK(crypto_decrypt(key, buffer, size - bytes_read, associated_data, sizeof(associated_data), sequence));

	if (replay_buffer) {
		replay_buffer_update(replay_buffer, sequence);
	}

	switch (type)
	{
	case PACKET_TYPE_CONNECTION_ACCEPTED:
	{
		packet_connection_accepted_t* packet = (packet_connection_accepted_t*)packet_allocator_alloc(pa, (packet_type_t)type);
		packet->packet_type = type;
		packet->client_handle = read_uint64(&buffer);
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
		packet->nonce = read_uint64(&buffer);
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

cute_error:
	return NULL;
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

	return CUTE_ALLOC(size, NULL);
}

void packet_allocator_free(packet_allocator_t* packet_allocator, packet_type_t type, void* packet)
{
	CUTE_FREE(packet, NULL);
}

// -------------------------------------------------------------------------------------------------

uint8_t* client_read_connect_token_from_web_service(uint8_t* buffer, uint64_t application_id, uint64_t current_time, connect_token_t* token)
{
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
	CUTE_CHECK(read_connect_token_packet_public_section(buffer, application_id, current_time, &packet));
	token->expiration_timestamp = packet.expiration_timestamp;
	token->handshake_timeout = packet.handshake_timeout;
	token->endpoint_count = packet.endpoint_count;
	CUTE_MEMCPY(token->endpoints, packet.endpoints, sizeof(endpoint_t) * token->endpoint_count);

	return connect_token_packet;

cute_error:
	return NULL;
}

int CUTE_CALL server_decrypt_connect_token_packet(uint8_t* packet_buffer, const crypto_key_t* secret_key, uint64_t application_id, uint64_t current_time, connect_token_decrypted_t* token)
{
	// Read public section.
	packet_connect_token_t packet;
	CUTE_CHECK(read_connect_token_packet_public_section(packet_buffer, application_id, current_time, &packet));
	CUTE_CHECK(packet.expiration_timestamp < current_time);
	token->expiration_timestamp = packet.expiration_timestamp;
	token->handshake_timeout = packet.handshake_timeout;
	token->endpoint_count = packet.endpoint_count;
	CUTE_MEMCPY(token->endpoints, packet.endpoints, sizeof(endpoint_t) * token->endpoint_count);

	// Decrypt the secret section.
	uint8_t* big_nonce = packet_buffer + 656;
	uint8_t* secret_section = big_nonce + CUTE_CONNECT_TOKEN_NONCE_SIZE;
	uint8_t* hmac_bytes = secret_section + CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE;
	uint8_t* additional_data = packet_buffer;

	CUTE_MEMCPY(token->hmac_bytes, hmac_bytes, CUTE_CRYPTO_HMAC_BYTES);

	CUTE_CHECK(crypto_decrypt_bignonce(secret_key, secret_section, CUTE_CONNECT_TOKEN_SECRET_SECTION_SIZE + CUTE_CRYPTO_HMAC_BYTES, additional_data, 656, big_nonce));

	// Read secret section.
	token->client_id = read_uint64(&secret_section);
	token->client_to_server_key = read_key(&secret_section);
	token->server_to_client_key = read_key(&secret_section);
	CUTE_MEMCPY(token->user_data, secret_section, CUTE_CONNECT_TOKEN_USER_DATA_SIZE);

	return 0;

cute_error:
	return -1;
}

// -------------------------------------------------------------------------------------------------

// All primes up to just above 10,000.
static const uint16_t s_primes[] = {
	2, 3, 5, 7, 11, 13, 17, 19, 23, 29,	31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
	73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
	179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
	283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
	419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
	547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
	661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
	811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
	947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
	1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
	1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
	1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
	1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657,
	1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
	1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987,
	1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129,
	2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287,
	2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
	2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617,
	2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741,
	2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903,
	2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
	3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257,
	3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413,
	3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571,
	3581, 3583, 3593, 3607, 3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727,
	3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821, 3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 3907,
	3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003, 4007, 4013, 4019, 4021, 4027, 4049, 4051, 4057,
	4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129, 4133, 4139, 4153, 4157, 4159, 4177, 4201, 4211, 4217, 4219, 4229, 4231,
	4241, 4243, 4253, 4259, 4261, 4271, 4273, 4283, 4289, 4297, 4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 4409,
	4421, 4423, 4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507, 4513, 4517, 4519, 4523, 4547, 4549, 4561, 4567, 4583,
	4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651, 4657, 4663, 4673, 4679, 4691, 4703, 4721, 4723, 4729, 4733, 4751,
	4759, 4783, 4787, 4789, 4793, 4799, 4801, 4813, 4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 4933, 4937,
	4943, 4951, 4957, 4967, 4969, 4973, 4987, 4993, 4999, 5003, 5009, 5011, 5021, 5023, 5039, 5051, 5059, 5077, 5081, 5087,
	5099, 5101, 5107, 5113, 5119, 5147, 5153, 5167, 5171, 5179, 5189, 5197, 5209, 5227, 5231, 5233, 5237, 5261, 5273, 5279,
	5281, 5297, 5303, 5309, 5323, 5333, 5347, 5351, 5381, 5387, 5393, 5399, 5407, 5413, 5417, 5419, 5431, 5437, 5441, 5443,
	5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507, 5519, 5521, 5527, 5531, 5557, 5563, 5569, 5573, 5581, 5591, 5623, 5639,
	5641, 5647, 5651, 5653, 5657, 5659, 5669, 5683, 5689, 5693, 5701, 5711, 5717, 5737, 5741, 5743, 5749, 5779, 5783, 5791,
	5801, 5807, 5813, 5821, 5827, 5839, 5843, 5849, 5851, 5857, 5861, 5867, 5869, 5879, 5881, 5897, 5903, 5923, 5927, 5939,
	5953, 5981, 5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053, 6067, 6073, 6079, 6089, 6091, 6101, 6113, 6121, 6131, 6133,
	6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211, 6217, 6221, 6229, 6247, 6257, 6263, 6269, 6271, 6277, 6287, 6299, 6301,
	6311, 6317, 6323, 6329, 6337, 6343, 6353, 6359, 6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 6473,
	6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571, 6577, 6581, 6599, 6607, 6619, 6637, 6653, 6659, 6661, 6673,
	6679, 6689, 6691, 6701, 6703, 6709, 6719, 6733, 6737, 6761, 6763, 6779, 6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833,
	6841, 6857, 6863, 6869, 6871, 6883, 6899, 6907, 6911, 6917, 6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991, 6997,
	7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069, 7079, 7103, 7109, 7121, 7127, 7129, 7151, 7159, 7177, 7187, 7193, 7207,
	7211, 7213, 7219, 7229, 7237, 7243, 7247, 7253, 7283, 7297, 7307, 7309, 7321, 7331, 7333, 7349, 7351, 7369, 7393, 7411,
	7417, 7433, 7451, 7457, 7459, 7477, 7481, 7487, 7489, 7499, 7507, 7517, 7523, 7529, 7537, 7541, 7547, 7549, 7559, 7561,
	7573, 7577, 7583, 7589, 7591, 7603, 7607, 7621, 7639, 7643, 7649, 7669, 7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723,
	7727, 7741, 7753, 7757, 7759, 7789, 7793, 7817, 7823, 7829, 7841, 7853, 7867, 7873, 7877, 7879, 7883, 7901, 7907, 7919,
	7927, 7933, 7937, 7949, 7951, 7963, 7993, 8009, 8011, 8017, 8039, 8053, 8059, 8069, 8081, 8087, 8089, 8093, 8101, 8111,
	8117, 8123, 8147, 8161, 8167, 8171, 8179, 8191, 8209, 8219, 8221, 8231, 8233, 8237, 8243, 8263, 8269, 8273, 8287, 8291,
	8293, 8297, 8311, 8317, 8329, 8353, 8363, 8369, 8377, 8387, 8389, 8419, 8423, 8429, 8431, 8443, 8447, 8461, 8467, 8501,
	8513, 8521, 8527, 8537, 8539, 8543, 8563, 8573, 8581, 8597, 8599, 8609, 8623, 8627, 8629, 8641, 8647, 8663, 8669, 8677,
	8681, 8689, 8693, 8699, 8707, 8713, 8719, 8731, 8737, 8741, 8747, 8753, 8761, 8779, 8783, 8803, 8807, 8819, 8821, 8831,
	8837, 8839, 8849, 8861, 8863, 8867, 8887, 8893, 8923, 8929, 8933, 8941, 8951, 8963, 8969, 8971, 8999, 9001, 9007, 9011,
	9013, 9029, 9041, 9043, 9049, 9059, 9067, 9091, 9103, 9109, 9127, 9133, 9137, 9151, 9157, 9161, 9173, 9181, 9187, 9199,
	9203, 9209, 9221, 9227, 9239, 9241, 9257, 9277, 9281, 9283, 9293, 9311, 9319, 9323, 9337, 9341, 9343, 9349, 9371, 9377,
	9391, 9397, 9403, 9413, 9419, 9421, 9431, 9433, 9437, 9439, 9461, 9463, 9467, 9473, 9479, 9491, 9497, 9511, 9521, 9533,
	9539, 9547, 9551, 9587, 9601, 9613, 9619, 9623, 9629, 9631, 9643, 9649, 9661, 9677, 9679, 9689, 9697, 9719, 9721, 9733,
	9739, 9743, 9749, 9767, 9769, 9781, 9787, 9791, 9803, 9811, 9817, 9829, 9833, 9839, 9851, 9857, 9859, 9871, 9883, 9887,
	9901, 9907, 9923, 9929, 9931, 9941, 9949, 9967, 9973, 10007,
};

static CUTE_INLINE uint32_t s_get_next_prime(uint32_t val)
{
	CUTE_ASSERT(val <= 10007);
	for (int i = 0; i < sizeof(s_primes) / sizeof(*s_primes); ++i)
		if (s_primes[i] >= val) return s_primes[i];
	CUTE_ASSERT(0);
	return ~0;
}


int hashtable_init(hashtable_t* table, int key_size, int item_size, int capacity, void* mem_ctx)
{
	CUTE_ASSERT(capacity);
	CUTE_MEMSET(table, 0, sizeof(hashtable_t));

	table->count = 0;
	table->slot_capacity = s_get_next_prime(capacity);
	table->key_size = key_size;
	table->item_size = item_size;
	int slots_size = (int)(table->slot_capacity * sizeof(*table->slots));
	table->slots = (hashtable_slot_t*)CUTE_ALLOC((size_t)slots_size, mem_ctx);
	CUTE_CHECK_POINTER(table->slots);
	CUTE_MEMSET(table->slots, 0, (size_t) slots_size);

	crypto_shorthash_keygen(table->secret_key);

	table->item_capacity = s_get_next_prime(capacity + capacity / 2);
	table->items_key = CUTE_ALLOC(table->item_capacity * (table->key_size + sizeof(*table->items_slot_index) + table->item_size) + table->item_size + table->key_size, mem_ctx);
	CUTE_CHECK_POINTER(table->items_key);
	table->items_slot_index = (int*)((uint8_t*)table->items_key + table->item_capacity * table->key_size);
	table->items_data = (void*)(table->items_slot_index + table->item_capacity);
	table->temp_key = (void*)(((uintptr_t)table->items_data) + table->item_size * table->item_capacity);
	table->temp_item = (void*)(((uintptr_t)table->temp_key) + table->key_size);
	table->mem_ctx = mem_ctx;

	return 0;

cute_error:
	CUTE_FREE(table->slots, mem_ctx);
	CUTE_FREE(table->items_key, mem_ctx);
	return -1;
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
	if (crypto_shorthash(hash_bytes, (const uint8_t*)key, table->key_size, table->secret_key) < 0) {
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

void hashtable_insert(hashtable_t* table, const void* key, const void* item)
{
	CUTE_ASSERT(hashtable_internal_find_slot(table, key) < 0);
	uint64_t hash = s_calc_hash(table, key);

	CUTE_ASSERT(table->count < table->slot_capacity);

	int base_slot = (int)(hash % (uint64_t)table->slot_capacity);
	int base_count = table->slots[ base_slot ].base_count;
	int slot = base_slot;
	int first_free = slot;
	while (base_count)
	{
		uint64_t slot_hash = table->slots[slot].key_hash;
		if (slot_hash == 0 && table->slots[ first_free ].key_hash != 0) first_free = slot;
		int slot_base = (int)(slot_hash % (uint64_t)table->slot_capacity);
		if (slot_base == base_slot) 
			--base_count;
		slot = (slot + 1) % table->slot_capacity;
	}

	slot = first_free;
	while (table->slots[ slot ].key_hash)
		slot = (slot + 1) % table->slot_capacity;

	CUTE_ASSERT(table->count < table->item_capacity);

	CUTE_ASSERT(!table->slots[slot].key_hash && (hash % (uint64_t)table->slot_capacity) == (uint64_t)base_slot);
	CUTE_ASSERT(hash);
	table->slots[slot].key_hash = hash;
	table->slots[slot].item_index = table->count;
	++table->slots[ base_slot ].base_count;

	void* item_dst = s_get_item(table, table->count);
	void* key_dst = s_get_key(table, table->count);
	CUTE_MEMCPY(item_dst, item, table->item_size);
	CUTE_MEMCPY(key_dst, key, table->key_size);
    table->items_slot_index[table->count] = slot;
	++table->count;
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
		CUTE_MEMCPY(dst_key, src_key, (size_t)table->item_size);
		void* dst_item = s_get_item(table, index);
		void* src_item = s_get_item(table, last_index);
		CUTE_MEMCPY(dst_item, src_item, (size_t)table->item_size);
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

}
}
