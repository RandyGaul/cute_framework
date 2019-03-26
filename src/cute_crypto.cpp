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

namespace cute
{

int crypto_encrypt_asymmetric(const crypto_key_t* endpoint_public_key, uint8_t* buffer, int size_to_encrypt, int buffer_size)
{
	if (size_to_encrypt + CUTE_CRYPTO_ASYMMETRIC_BYTES > buffer_size) {
		error_set("Can not encrypt data: `buffer_size` must be at least `CUTE_CRYPTO_ASYMMETRIC_BYTES` bytes larger than `size_to_encrypt`.");
		return -1;
	}
	CUTE_MEMMOVE(buffer + CUTE_CRYPTO_ASYMMETRIC_BYTES, buffer, size_to_encrypt);
	return crypto_box_seal(buffer, buffer + CUTE_CRYPTO_ASYMMETRIC_BYTES, size_to_encrypt, endpoint_public_key->key);
}

int crypto_decrypt_asymmetric(const crypto_key_t* your_public_key, const crypto_key_t* your_secret_key, uint8_t* buffer, int buffer_size)
{
	return crypto_box_seal_open(buffer, buffer, buffer_size, your_public_key->key, your_secret_key->key);
}

int crypto_encrypt(const crypto_key_t* symmetric_key, uint8_t* buffer, int size_to_encrypt, int buffer_size, const crypto_nonce_t* nonce)
{
	if (size_to_encrypt + CUTE_CRYPTO_SYMMETRIC_BYTES > buffer_size) {
		error_set("Can not encrypt data: `buffer_size` must be at least `CUTE_CRYPTO_SYMMETRIC_BYTES` bytes larger than `size_to_encrypt`.");
		return -1;
	}
	return crypto_secretbox_easy(buffer, buffer, size_to_encrypt, nonce->nonce, symmetric_key->key);
}

int crypto_decrypt(const crypto_key_t* symmetric_key, uint8_t* data, int byte_count, const crypto_nonce_t* nonce)
{
	return crypto_secretbox_open_easy(data, data, byte_count, nonce->nonce, symmetric_key->key);
}

int crypto_generate_keypair(crypto_key_t* public_key, crypto_key_t* private_key)
{
	return crypto_box_keypair(public_key->key, private_key->key);
}

crypto_key_t crypto_generate_symmetric_key()
{
	crypto_key_t key;
	crypto_secretbox_keygen(key.key);
	return key;
}

crypto_nonce_t crypto_random_nonce()
{
	crypto_nonce_t nonce;
	crypto_random_bytes(&nonce, sizeof(nonce));
	return nonce;
}

void crypto_random_bytes(void* data, int byte_count)
{
	randombytes_buf(data, byte_count);
}

const char* crypto_sodium_version_linked()
{
	return sodium_version_string();
}

namespace internal
{
	int crypto_init()
	{
		if (sodium_init() < 0) {
			error_set( "Unable to initialize crypto library. It is *not safe* to connect to the net.");
			return -1;
		}
		if (crypto_box_publickeybytes() != crypto_box_secretkeybytes()) {
			// The version of libsodium Cute was originally written with held this invariant.
			error_set( "Unable to initialize crypto library. It is *not safe* to connect to the net.");
			return -1;
		}
		return 0;
	}
}

}
