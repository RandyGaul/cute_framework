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

#include <cute_crypto.h>
#include <cute_c_runtime.h>
using namespace cute;

CUTE_TEST_CASE(test_crypto_encrypt_decrypt, "Generate key, encrypt a message, decrypt the message.");
int test_crypto_encrypt_decrypt()
{
	crypto_key_t k = crypto_generate_key();

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * (message_length + CUTE_CRYPTO_HMAC_BYTES));
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	uint64_t sequence;
	crypto_random_bytes(&sequence, sizeof(sequence));

	CUTE_TEST_CHECK(crypto_encrypt(&k, message_buffer, message_length, NULL, 0, sequence));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt(&k, message_buffer, message_length + CUTE_CRYPTO_HMAC_BYTES, NULL, 0, sequence));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}

CUTE_TEST_CASE(test_crypto_encrypt_decrypt_associated_data, "Generate key and associated data, encrypt message, decrypt message.");
int test_crypto_encrypt_decrypt_associated_data()
{
	crypto_key_t k = crypto_generate_key();

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * (message_length + CUTE_CRYPTO_HMAC_BYTES));
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	uint64_t sequence;
	crypto_random_bytes(&sequence, sizeof(sequence));

	uint8_t associated_data[256];
	crypto_random_bytes(associated_data, sizeof(associated_data));

	CUTE_TEST_CHECK(crypto_encrypt(&k, message_buffer, message_length, associated_data, sizeof(associated_data), sequence));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt(&k, message_buffer, message_length + CUTE_CRYPTO_HMAC_BYTES, associated_data, sizeof(associated_data), sequence));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}

CUTE_TEST_CASE(test_crypto_encrypt_decrypt_big_nonce, "Generate key with big nonce, encrypt message, decrypt message.");
int test_crypto_encrypt_decrypt_big_nonce()
{
	crypto_key_t k = crypto_generate_key();

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * (message_length + CUTE_CRYPTO_HMAC_BYTES));
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	uint8_t big_nonce[CUTE_CRYPTO_NONCE_BYTES];
	crypto_random_bytes(big_nonce, sizeof(big_nonce));

	CUTE_TEST_CHECK(crypto_encrypt_bignonce(&k, message_buffer, message_length, NULL, 0, big_nonce));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt_bignonce(&k, message_buffer, message_length + CUTE_CRYPTO_HMAC_BYTES, NULL, 0, big_nonce));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}

CUTE_TEST_CASE(test_crypto_encrypt_decrypt_associated_data_big_nonce, "Generate key and associated data and big nonce, encrypt message, decrypt message.");
int test_crypto_encrypt_decrypt_associated_data_big_nonce()
{
	crypto_key_t k = crypto_generate_key();

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * (message_length + CUTE_CRYPTO_HMAC_BYTES));
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	uint8_t big_nonce[CUTE_CRYPTO_NONCE_BYTES];
	crypto_random_bytes(big_nonce, sizeof(big_nonce));

	uint8_t associated_data[256];
	crypto_random_bytes(associated_data, sizeof(associated_data));

	CUTE_TEST_CHECK(crypto_encrypt_bignonce(&k, message_buffer, message_length, associated_data, sizeof(associated_data), big_nonce));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt_bignonce(&k, message_buffer, message_length + CUTE_CRYPTO_HMAC_BYTES, associated_data, sizeof(associated_data), big_nonce));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}
