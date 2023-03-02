/*
	Cute Framework
	Copyright (C) 2023 Randy Gaul https://randygaul.github.io/

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

#ifndef CUTE_BASE64_H
#define CUTE_BASE64_H

#include "cute_defines.h"
#include "cute_result.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Info about base 64 encoding: https://tools.ietf.org/html/rfc4648

/**
 * @function CF_BASE64_ENCODED_SIZE
 * @category base64
 * @brief    Calculates the size of data after base64 encoding it.
 * @param    size         The size of un-encoded raw data.
 * @return   Returns the number of bytes the base64 encoded data will take up. This will inflate the size ~33%.
 * @remarks  Use this for the `dst_size` in `cf_base64_encode`.
 *           Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
 *           base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).
 * @related  CF_BASE64_ENCODED_SIZE CF_BASE64_DECODED_SIZE cf_base64_encode cf_base64_decode
 */
#define CF_BASE64_ENCODED_SIZE(size) ((((size) + 2) / 3) * 4)

/**
 * @function CF_BASE64_DECODED_SIZE
 * @category base64
 * @brief    Calculates the size of data after base64 decoding it.
 * @param    size         The size of base64 encoded data.
 * @return   Returns the number of bytes the raw dencoded data will take up. This will deflate the size ~33%.
 * @remarks  Use this for the `dst_size` in `cf_base64_decode`.
 *           Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
 *           base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).
 * @related  CF_BASE64_ENCODED_SIZE CF_BASE64_DECODED_SIZE cf_base64_encode cf_base64_decode
 */
#define CF_BASE64_DECODED_SIZE(size) ((((size) + 3) / 4) * 3)

/**
 * @function cf_base64_encode
 * @category base64
 * @brief    Encodes raw binary data in base64 format.
 * @param    dst         The destination buffer, where base64 encoded data is written to.
 * @param    dst_size    The size of `dst` in bytes. You can use `CF_BASE64_ENCODED_SIZE` on `src_size` to calculate this value.
 * @param    src         Raw unencoded bytes.
 * @param    src_size    The size of `src` in bytes.
 * @return   Returns a `CF_Result` containing information about any errors.
 * @remarks  Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
 *           base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).
 * @related  CF_BASE64_ENCODED_SIZE CF_BASE64_DECODED_SIZE cf_base64_encode cf_base64_decode
 */
CUTE_API CF_Result CUTE_CALL cf_base64_encode(void* dst, size_t dst_size, const void* src, size_t src_size);

/**
 * @function cf_base64_decode
 * @category base64
 * @brief    Decodes base64 encoded data into raw bytes.
 * @param    dst         The destination buffer, where raw decoded data is written to.
 * @param    dst_size    The size of `dst` in bytes. You can use `CF_BASE64_DECODED_SIZE` on `src_size` to calculate this value.
 * @param    src         Base64 encoded bytes.
 * @param    src_size    The size of `src` in bytes.
 * @return   Returns a `CF_Result` containing information about any errors.
 * @remarks  Base64 encoding is useful for storing data as text in a copy-paste-safe manner. For more information about
 *           base64 encoding see this link: [RFC-4648](https://tools.ietf.org/html/rfc4648) or [Wikipedia Base64](https://en.wikipedia.org/wiki/Base64).
 * @related  CF_BASE64_ENCODED_SIZE CF_BASE64_DECODED_SIZE cf_base64_encode cf_base64_decode
 */
CUTE_API CF_Result CUTE_CALL cf_base64_decode(void* dst, size_t dst_size, const void* src, size_t src_size);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

CUTE_INLINE Result base64_encode(void* dst, size_t dst_size, const void* src, size_t src_size) { return cf_base64_encode(dst, dst_size, src, src_size); }
CUTE_INLINE Result base64_decode(void* dst, size_t dst_size, const void* src, size_t src_size) { return cf_base64_decode(dst, dst_size, src, src_size); }

}

#endif // CUTE_CPP

#endif // !CUTE_BASE64_H
