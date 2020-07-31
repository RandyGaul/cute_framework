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

int crypto_encrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, uint64_t sequence_nonce)
{
	uint8_t nonce_bytes[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	CUTE_MEMSET(nonce_bytes, 0, sizeof(nonce_bytes));
	*((uint64_t*)(nonce_bytes + sizeof(nonce_bytes) - sizeof(uint64_t))) = sequence_nonce;

	uint64_t encrypted_sz;
	int ret = crypto_aead_chacha20poly1305_ietf_encrypt(data, &encrypted_sz, data, (uint64_t)data_size, associated_data, associated_data_size, NULL, nonce_bytes, key->key);
	if (ret < 0) return -1;
	CUTE_ASSERT(encrypted_sz == data_size + CUTE_CRYPTO_HMAC_BYTES);
	return ret;
}

int crypto_decrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, uint64_t sequence_nonce)
{
	uint8_t nonce_bytes[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	CUTE_MEMSET(nonce_bytes, 0, sizeof(nonce_bytes));
	*((uint64_t*)(nonce_bytes + sizeof(nonce_bytes) - sizeof(uint64_t))) = sequence_nonce;

	uint64_t encrypted_sz;
	int ret = crypto_aead_chacha20poly1305_ietf_decrypt(data, &encrypted_sz, NULL, data, (uint64_t)data_size, associated_data, associated_data_size, nonce_bytes, key->key);
	if (ret < 0) return -1;
	CUTE_ASSERT(encrypted_sz == data_size - CUTE_CRYPTO_HMAC_BYTES);
	return ret;
}

int crypto_encrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, const uint8_t* sequence_nonce)
{
	uint64_t encrypted_sz;
	int ret = crypto_aead_xchacha20poly1305_ietf_encrypt(data, &encrypted_sz, data, (uint64_t)data_size, associated_data, associated_data_size, NULL, sequence_nonce, key->key);
	if (ret < 0) return -1;
	CUTE_ASSERT(encrypted_sz == data_size + CUTE_CRYPTO_HMAC_BYTES);
	return ret;
}

int crypto_decrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, const uint8_t* sequence_nonce)
{
	uint64_t decrypted_sz;
	int ret = crypto_aead_xchacha20poly1305_ietf_decrypt(data, &decrypted_sz, NULL, data, (uint64_t)data_size, associated_data, (uint64_t)associated_data_size, sequence_nonce, key->key);
	if (ret < 0) return -1;
	CUTE_ASSERT(decrypted_sz == data_size - CUTE_CRYPTO_HMAC_BYTES);
	return ret;
}

int crypto_generate_keypair(crypto_key_t* public_key, crypto_key_t* private_key)
{
	return crypto_box_keypair(public_key->key, private_key->key);
}

crypto_key_t crypto_generate_key()
{
	crypto_key_t key;
	crypto_secretbox_keygen(key.key);
	return key;
}

void crypto_random_bytes(void* data, int byte_count)
{
	randombytes_buf(data, byte_count);
}

const char* crypto_sodium_version_linked()
{
	return sodium_version_string();
}

error_t crypto_init()
{
	if (sodium_init() < 0) {
		return error_failure("Unable to initialize crypto library. It is *not safe* to connect to the net.");
	}
	if (crypto_box_publickeybytes() != crypto_box_secretkeybytes()) {
		// The version of libsodium Cute was originally written with held this invariant.
		return error_failure( "Unable to initialize crypto library. It is *not safe* to connect to the net.");
	}
	return error_success();
}

}
