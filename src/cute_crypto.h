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

#include <libsodium/sodium.h>

#define CUTE_CRYPTO_MAC_BYTES ((int)crypto_aead_xchacha20poly1305_ietf_ABYTES)
#define CUTE_CRYPTO_NONCE_BYTES ((int)crypto_aead_xchacha20poly1305_ietf_NPUBBYTES)

namespace cute
{

struct crypto_key_t;

extern CUTE_API int CUTE_CALL crypto_encrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* additional_data, int additional_data_size, uint64_t nonce);
extern CUTE_API int CUTE_CALL crypto_decrypt(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* additional_data, int additional_data_size, uint64_t nonce);

extern CUTE_API int CUTE_CALL crypto_encrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* additional_data, int additional_data_size, const uint8_t* nonce);
extern CUTE_API int CUTE_CALL crypto_decrypt_bignonce(const crypto_key_t* key, uint8_t* data, int data_size, const uint8_t* additional_data, int additional_data_size, const uint8_t* nonce);

}

#include <cute_crypto_utils.h>

#endif // CUTE_CRYPTO_H
