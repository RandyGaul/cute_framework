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

CUTE_TEST_CASE(test_crypto_symmetric_key_encrypt_decrypt, "Generate symmetric key, encrypt a message, decrypt the message.");
int test_crypto_symmetric_key_encrypt_decrypt()
{
	crypto_key_t k = crypto_generate_symmetric_key();

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * message_length + CUTE_CRYPTO_SYMMETRIC_BYTES);
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	crypto_nonce_t nonce;
	crypto_random_bytes(&nonce, sizeof(nonce));

	CUTE_TEST_CHECK(crypto_encrypt(&k, message_buffer, message_length, &nonce));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt(&k, message_buffer, message_length + CUTE_CRYPTO_SYMMETRIC_BYTES, &nonce));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}

CUTE_TEST_CASE(test_crypto_assymetric_key_encrypt_decrypt, "Simulate sending a secure anonymous message to server with public and secret keypair.");
int test_crypto_assymetric_key_encrypt_decrypt()
{
	crypto_key_t pk, sk;
	crypto_generate_keypair(&pk, &sk);

	const char* message_string = "The message.";
	int message_length = (int)CUTE_STRLEN(message_string) + 1;
	uint8_t* message_buffer = (uint8_t*)malloc(sizeof(uint8_t) * message_length + CUTE_CRYPTO_ASYMMETRIC_BYTES);
	CUTE_MEMCPY(message_buffer, message_string, message_length);

	CUTE_TEST_CHECK(crypto_encrypt_asymmetric(&pk, message_buffer, message_length));
	CUTE_TEST_ASSERT(CUTE_MEMCMP(message_buffer, message_string, message_length));
	CUTE_TEST_CHECK(crypto_decrypt_asymmetric(&pk, &sk, message_buffer, message_length + CUTE_CRYPTO_ASYMMETRIC_BYTES));
	CUTE_TEST_ASSERT(!CUTE_MEMCMP(message_buffer, message_string, message_length));

	free(message_buffer);

	return 0;
}
