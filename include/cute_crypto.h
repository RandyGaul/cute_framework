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

#ifndef CUTE_CRYPTO_H
#define CUTE_CRYPTO_H

#include "cute_defines.h"
#include "cute_error.h"


namespace cute
{

struct crypto_key_t
{
	uint8_t key[32];
};

#define CUTE_CRYPTO_HEADER_BYTES ((int)(20 + 16))

CUTE_API crypto_key_t CUTE_CALL crypto_generate_key();
CUTE_API void CUTE_CALL crypto_encrypt(const crypto_key_t* key, uint8_t* data, int data_size, uint64_t msg_id = 0);
CUTE_API error_t CUTE_CALL crypto_decrypt(const crypto_key_t* key, uint8_t* data, int data_size, uint64_t msg_id = 0);
CUTE_API void CUTE_CALL crypto_random_bytes(void* data, int byte_count);

struct crypto_sign_public_t
{
	uint8_t key[32];
};

struct crypto_sign_secret_t
{
	uint8_t key[64];
};

struct crypto_signature_t
{
	uint8_t bytes[64];
};

CUTE_API void CUTE_CALL crypto_sign_keygen(crypto_sign_public_t* public_key, crypto_sign_secret_t* secret_key);
CUTE_API void CUTE_CALL crypto_sign_create(const crypto_sign_secret_t* secret_key, crypto_signature_t* signature, const uint8_t* data, int data_size);
CUTE_API error_t CUTE_CALL crypto_sign_verify(const crypto_sign_public_t* public_key, const crypto_signature_t* signature, const uint8_t* data, int data_size);

}

#endif // CUTE_CRYPTO_H
