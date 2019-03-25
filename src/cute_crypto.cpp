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

namespace cute
{

void crypto_generate_keypair(crypto_key_t* public_key, crypto_key_t* private_key)
{
	crypto_box_keypair(public_key->key, private_key->key);
}

int crypto_encrypt_asymmetric(const crypto_key_t* endpoint_public_key, const crypto_key_t* your_private_key, uint8_t* data, int byte_count, const crypto_nonce_t* nonce)
{
	return crypto_box_easy(data, data, byte_count, nonce->nonce, endpoint_public_key->key, your_private_key->key);
}

int crypto_decrypt_asymmetric(const crypto_key_t* endpoint_public_key, const crypto_key_t* your_private_key, uint8_t* data, int byte_count, const crypto_nonce_t* nonce)
{
	return crypto_box_open_easy(data, data, byte_count, nonce->nonce, endpoint_public_key->key, your_private_key->key);
}

crypto_key_t crypto_generate_symmetric_key()
{
	crypto_key_t key;
	crypto_secretbox_keygen(key.key);
	return key;
}

int crypto_encrypt(const crypto_key_t* symmetric_key, uint8_t* data, int byte_count, const crypto_nonce_t* nonce)
{
	return crypto_secretbox_easy(data, data, byte_count, nonce->nonce, symmetric_key->key);
}

int crypto_decrypt(const crypto_key_t* symmetric_key, uint8_t* data, int byte_count, const crypto_nonce_t* nonce)
{
	return crypto_secretbox_open_easy(data, data, byte_count, nonce->nonce, symmetric_key->key);
}

void crypto_random_bytes(void* data, int byte_count)
{
	randombytes_buf(data, byte_count);
}

namespace internal
{
	int crypto_init()
	{
		if (sodium_init() < 0) {
			error_set( "Unable to initialize crypto library. It is *not safe* to connect to the net.");
			return -1;
		}
		return 0;
	}
}

}
