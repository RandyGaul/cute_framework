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

#include <cute_defines.h>

#define CUTE_CRYPTO_HMAC_BYTES ((int)16U)
#define CUTE_CRYPTO_NONCE_BYTES ((int)24U)
#define CUTE_CRYPTO_KEY_BYTES ((int)32U)

namespace cute
{

struct crypto_key_t;

CUTE_API int CUTE_CALL crypto_encrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, uint64_t sequence_nonce);
CUTE_API int CUTE_CALL crypto_decrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, uint64_t sequence_nonce);

CUTE_API int CUTE_CALL crypto_encrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, const uint8_t* sequence_nonce);
CUTE_API int CUTE_CALL crypto_decrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* associated_data, int associated_data_size, const uint8_t* sequence_nonce);

struct crypto_key_t
{
	uint8_t key[CUTE_CRYPTO_KEY_BYTES];
};

CUTE_API void CUTE_CALL crypto_random_bytes(void* data, int byte_count);
CUTE_API crypto_key_t CUTE_CALL crypto_generate_key();
CUTE_API const char* CUTE_CALL crypto_sodium_version_linked();

}

#endif // CUTE_CRYPTO_H
