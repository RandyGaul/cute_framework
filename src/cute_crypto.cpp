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
#include <cute_error.h>
#include <cute_c_runtime.h>

#include <internal/cute_crypto_internal.h>

#include <hydrogen.h>

#define CUTE_CRYPTO_CONTEXT "CUTE_CTX"

namespace cute
{

CUTE_STATIC_ASSERT(CUTE_CRYPTO_HEADER_BYTES == (int)hydro_secretbox_HEADERBYTES, "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(crypto_signature_t) == hydro_sign_BYTES, "Must be equal.");
CUTE_STATIC_ASSERT(sizeof(uint64_t) == sizeof(long long unsigned int), "Must be equal.");

void crypto_encrypt(const crypto_key_t* key, uint8_t* data, int data_size, uint64_t msg_id)
{
	hydro_secretbox_encrypt(data, data, (uint64_t)data_size, msg_id, CUTE_CRYPTO_CONTEXT, key->key);
}

error_t crypto_decrypt(const crypto_key_t* key, uint8_t* data, int data_size, uint64_t msg_id)
{
	if (hydro_secretbox_decrypt(data, data, (size_t)data_size, msg_id, CUTE_CRYPTO_CONTEXT, key->key) != 0) {
		return error_failure("Message forged.");
	} else {
		return error_success();
	}
}

crypto_key_t crypto_generate_key()
{
	crypto_key_t key;
	hydro_secretbox_keygen(key.key);
	return key;
}

void crypto_random_bytes(void* data, int byte_count)
{
	hydro_random_buf(data, byte_count);
}

void crypto_sign_keygen(crypto_sign_public_t* public_key, crypto_sign_secret_t* secret_key)
{
	hydro_sign_keypair key_pair;
	hydro_sign_keygen(&key_pair);
	CUTE_MEMCPY(public_key->key, key_pair.pk, 32);
	CUTE_MEMCPY(secret_key->key, key_pair.sk, 64);
}

void crypto_sign_create(const crypto_sign_secret_t* secret_key, crypto_signature_t* signature, const uint8_t* data, int data_size)
{
	hydro_sign_create(signature->bytes, data, (size_t)data_size, CUTE_CRYPTO_CONTEXT, secret_key->key);
}

error_t crypto_sign_verify(const crypto_sign_public_t* public_key, const crypto_signature_t* signature, const uint8_t* data, int data_size)
{
	if (hydro_sign_verify(signature->bytes, data, (size_t)data_size, CUTE_CRYPTO_CONTEXT, public_key->key) != 0) {
		return error_failure("Message forged.");
	} else {
		return error_success();
	}
}

error_t crypto_init()
{
	if (hydro_init() != 0) {
		return error_failure("Unable to initialize crypto library. It is *not safe* to connect to the net.");
	}
	return error_success();
}

}
