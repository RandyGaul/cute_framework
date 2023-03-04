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

#ifndef CUTE_HTTPS_H
#define CUTE_HTTPS_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_c_runtime.h"
#include "cute_array.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @struct   CF_Https
 * @category web
 * @brief    Represents a single HTTPS request for clients to talk with web servers.
 * @remarks  POST and GET requests are supported for when you just need a basic way to communicate over HTTPS. Insecure HTTP is not supported,
 *           but cert verification can be skipped (not recommended).
 *           
 *           Supports chunked encoding. Does not support trailing headers, 100-continue, keep-alive, or other
 *           "advanced" HTTP features.
 *           
 *           The design of this API comes mainly from Mattias Gustavsson's http.h single-file C header.
 *           https://github.com/mattiasgustavsson/libs/blob/main/http.h
 *           
 *           Here is a full working example.
 *           
 *           ```cpp
 *           CF_Result err;
 *           CF_Https* https = cf_https_get("raw.githubusercontent.com", "443", "/RandyGaul/cute_framework/main/src/cute_https.h", &err);
 *           if (https) {
 *               while (cf_https_state(https) == CF_HTTPS_STATE_PENDING) {
 *                   size_t bytes_read = cf_https_process(https);
 *                   printf("Received %zu bytes...\n", bytes_read);
 *               }
 *               if (cf_https_state(https) == CF_HTTPS_STATE_COMPLETED) {
 *                   const CF_HttpsResponse response;
 *                   cf_https_response(https, &response);
 *                   printf("%s", response.content);
 *               }
 *               cf_https_destroy(https);
 *           } else {
 *               printf("HTTPS request failed: %s\n", err.details);
 *           }
 *           ```
 * @related  CF_Https cf_https_get cf_https_post
 */
typedef struct CF_Https CF_Https;
// @end

/**
 * @function cf_https_get
 * @category web
 * @brief    Initiates a GET request for the specified host (website address) and a given uri.
 * @param    host        The web address where we send the HTTPS GET request.
 * @param    port        The port to use.
 * @param    uri         The URI on the host.
 * @param    err         Can be `NULL`. Reports any errors.
 * @param    bool        You should set this to `true`. `false` will disable the secure part of HTTPS. Please only do this for testing.
 * @return   Returns a `CF_Https` pointer.
 * @remarks  If we wanted to retrieve this header from github, we could use this `host` "raw.githubusercontent.com" with this `uri`
 *           "/RandyGaul/cute_framework/main/src/cute_https.h" at port 443 (standard HTTPS port).
 *           
 *           Any errors are optionally reported through the `err` parameter.
 *           
 *           `verify_cert` will verify the server's x509 certificate, but can be disabled (dangerous).
 *           
 *           Returns a `CF_Https` pointer which needs to be processed with `cf_https_process` and cleaned up by `cf_https_destroy`. See
 *           `CF_Https` for a full example.
 *           
 *           `host` and `port` are unused when building with emscripten -- this is since an XMLHttpRequest is used
 *           underneath, meaning only files from the server this code came from can be loaded, and as such the `uri`
 *           should only be a relative path on the server.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
CUTE_API CF_Https* CUTE_CALL cf_https_get(const char* host, const char* port, const char* uri, CF_Result* err, bool verify_cert);

/**
 * @function cf_https_post
 * @category web
 * @brief    Initiates a GET request for the specified host (website address) and a given uri.
 * @param    host        The web address where we send the HTTPS GET request.
 * @param    port        The port to use.
 * @param    data        Pointer to data to send to the host.
 * @param    size        Size of `data` in bytes.
 * @param    uri         The URI on the host.
 * @param    err         Can be `NULL`. Reports any errors.
 * @param    bool        You should set this to `true`. `false` will disable the secure part of HTTPS. Please only do this for testing.
 * @return   Returns a `CF_Https` pointer.
 * @remarks  Initiates a POST request for the specified host (website address) and a given uri. The content of the post
 *           is in `data`, which can be `NULL` if `size` is 0.
 *           
 *           Any errors are optionally reported through the `err` parameter.
 *           
 *           `verify_cert` will verify the server's x509 certificate, but can be disabled (dangerous).
 *           
 *           Returns an `CF_Https` pointer which needs to be processed with `cf_https_process` and cleaned up by `cf_https_destroy`.
 *           
 *           `host` and `port` are unused when building with emscripten -- this is since an XMLHttpRequest is used
 *           underneath, meaning only files from the server this code came from can be loaded, and as such the `uri`
 *           should only be a relative path on the server.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
CUTE_API CF_Https* CUTE_CALL cf_https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, CF_Result* err, bool verify_cert);

/**
 * @function cf_https_destroy
 * @category web
 * @brief    Frees up all memory and closes the underlying HTTPS connection if still open.
 * @param    https       A `CF_Https` to destroy.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
CUTE_API void CUTE_CALL cf_https_destroy(CF_Https* https);

/**
 * @enum     CF_HttpsState
 * @category web
 * @brief    The states of power for the application.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
#define CF_HTTPS_STATE_DEFS \
	/* @entry Keep calling `cf_https_process`. See `CF_Https` for a full example. */          \
	CF_ENUM(HTTPS_STATE_PENDING,   0)                                                         \
	/* @entry The response has been acquired, retrieve it with `cf_https_response`. */        \
	CF_ENUM(HTTPS_STATE_COMPLETED, 1)                                                         \
	/* @entry The request has failed, the only valid operation left is `cf_https_destroy`. */ \
	CF_ENUM(HTTPS_STATE_FAILED,    2)                                                         \
	/* @end */

typedef enum CF_HttpsState
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HTTPS_STATE_DEFS
	#undef CF_ENUM
} CF_HttpsState;

/**
 * @function cf_https_state_type_to_string
 * @category web
 * @brief    Convert an enum `CF_HttpsState` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
CUTE_INLINE const char* cf_https_state_type_to_string(CF_HttpsState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return CUTE_STRINGIZE(CF_##K);
	CF_HTTPS_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_https_state
 * @category web
 * @brief    Returns the current state of the `https` object.
 * @remarks  This is used mainly for calling `https_process`.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response
 */
CUTE_API CF_HttpsState CUTE_CALL cf_https_state(CF_Https* https);

/**
 * @function cf_https_process
 * @category web
 * @brief    Processes an HTTPS request.
 * @return   Returns the bytes recieved so far.
 * @remarks  Since this API uses non-blocking sockets `cf_https_process` needs to be call periodically after `cf_https_get`
 *           or `cf_https_post` is called for as long as `cf_https_state` returns `CF_HTTPS_STATE_PENDING`. You can call
 *           this function from within its own loop, put it on another thread within a loop, or call it once per
 *           game tick -- whichever you prefer.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response cf_https_state
 */
CUTE_API size_t CUTE_CALL cf_https_process(CF_Https* https);

/**
 * @struct   CF_HttpsString
 * @category web
 * @brief    A string within an `CF_HttpsResponse`.
 * @related  CF_Https CF_HttpsString CF_HttpsHeader CF_HttpsHeader CF_HttpsResponse cf_https_response
 */
typedef struct CF_HttpsString
{
	/* @member The string contents, not nul-terminated. */
	const char* ptr;

	/* The number of characters in the string. */
	size_t len;
} CF_HttpsString;
// @end

/**
 * @struct   CF_HttpsHeader
 * @category web
 * @brief    An HTTP header from a `CF_HttpsResponse`.
 * @related  CF_Https CF_HttpsString CF_HttpsHeader CF_HttpsHeader CF_HttpsResponse cf_https_response cf_https_strcmp cf_https_response_find_header
 */
typedef struct CF_HttpsHeader
{
	/* @member Name of the header. */
	CF_HttpsString name;

	/* @member String contents of the header, not nul-terminated. */
	CF_HttpsString content;
} CF_HttpsHeader;
// @end

/**
 * @enum     Transfer Encoding
 * @category web
 * @brief    Flags for `CF_HttpsResponse` transfer encoding settings.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process
 */
#define CF_TRANSFER_ENCODING_FLAG_DEFS \
	/* @entry No transfer encoding settings -- just raw bytes. */ \
	CF_ENUM(TRANSFER_ENCODING_FLAG_NONE,                0x00)     \
	/* @entry The content of the response is in chunked form. */  \
	CF_ENUM(TRANSFER_ENCODING_FLAG_CHUNKED,             0x01)     \
	/* @entry GZIP compression is used. */                        \
	CF_ENUM(TRANSFER_ENCODING_FLAG_GZIP,                0x02)     \
	/* @entry DEFLATE compression is used. */                     \
	CF_ENUM(TRANSFER_ENCODING_FLAG_DEFLATE,             0x04)     \
	/* @entry A deprecated compression method was used. */        \
	CF_ENUM(TRANSFER_ENCODING_FLAG_DEPRECATED_COMPRESS, 0x08)     \
	/* @end */

enum
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_TRANSFER_ENCODING_FLAG_DEFS
	#undef CF_ENUM
};

/**
 * @struct   CF_HttpsResponse
 * @category web
 * @brief    Represents the response from a server,
 * @remarks  After a successful loop via `cf_https_process (see `CF_Https`) a response can be fetched by calling `cf_https_response`.
 * @related  CF_Https CF_HttpsString CF_HttpsHeader CF_HttpsHeader CF_HttpsResponse cf_https_response
 */
typedef struct CF_HttpsResponse
{
	/* @member The HTTP response code. */
	int code;

	/* @member Length of the HTTP response content. */
	size_t content_len;

	/* @member The HTTP response content. */
	const char* content;

	/* @member Array of headers from the response. See `CF_HttpsHeader`. */
	const CF_HttpsHeader* headers;

	/* @member Number of headers in `headers`. */
	int headers_count;

	/* @member Flags from `TransferEncoding`. For example, if content is gzip'd, you can tell by using
	 *         something like so: `bool is_gzip = !!(response->transfer_encoding & CF_TRANSFER_ENCODING_GZIP);`.
	 *         
	 *         Please note that if the encoding is `CF_TRANSFER_ENCODING_CHUNKED` the `content` buffer will not
	 *         contain any chunked encoding -- all chunked data has been decoded already. For gzip/deflate
	 *         the `content` buffer will need to be decompressed by you.
	 */
	int transfer_encoding_flags;
} CF_HttpsResponse;
// @end

/**
 * @function cf_https_response
 * @category web
 * @brief    Returns a `CF_HttpsResponse` from a request.
 * @remarks  A response can be retrieved from the `https` object after `cf_https_state` returns `CF_HTTPS_STATE_COMPLETED`.
 *           Calling this function otherwise will get you a NULL pointer returned. This will get cleaned up automatically
 *           when `cf_https_destroy` is called.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response cf_https_state
 */
CUTE_API CF_HttpsResponse CUTE_CALL cf_https_response(CF_Https* https);

/**
 * @function cf_https_strcmp
 * @category web
 * @brief    Helper function to test a string for equality with a `CF_HttpsString`.
 * @return   Returns true the string is equal. Case is ignored.
 * @related  CF_Https CF_HttpsResponse cf_https_response CF_HttpsString CF_HttpsHeader cf_https_response_find_header
 */
CUTE_INLINE bool cf_https_strcmp(const char* lit, CF_HttpsString string)
{
	size_t len = CUTE_STRLEN(lit);
	if (len != string.len) return true;
	for (size_t i = 0; i < len; ++i) {
		if (CUTE_TOLOWER(lit[i]) != CUTE_TOLOWER(string.ptr[i])) {
			return true;
		}
	}
	return false;
}

/**
 * @function cf_https_response_find_header
 * @category web
 * @brief    Helper function find a specific header in a `CF_HttpsResponse`.
 * @return   Returns true the header was found. The header will be written to `header_out`. Case is ignored.
 * @related  CF_Https CF_HttpsResponse cf_https_response CF_HttpsString CF_HttpsHeader cf_https_response_find_header
 */
CUTE_INLINE bool cf_https_response_find_header(const CF_HttpsResponse* response, const char* header_name, CF_HttpsHeader* header_out)
{
	CUTE_MEMSET(header_out, 0, sizeof(CF_HttpsHeader));

	for (int i = 0; i < response->headers_count; ++i) {
		CF_HttpsHeader header = response->headers[i];
		if (!cf_https_strcmp(header_name, header.name)) {
			if (header_out) *header_out = header;
			return true;
		}
	}
	return false;
}

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CUTE_CPP

namespace Cute
{

using Https = CF_Https;
using HttpsString = CF_HttpsString;
using HttpsHeader = CF_HttpsHeader;

using HttpsState = CF_HttpsState;
#define CF_ENUM(K, V) CUTE_INLINE constexpr HttpsState K = CF_##K;
CF_HTTPS_STATE_DEFS
#undef CF_ENUM

CUTE_INLINE const char* to_string(HttpsState state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HTTPS_STATE_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

enum : int
{
	#define CF_ENUM(K, V) K = V,
	CF_TRANSFER_ENCODING_FLAG_DEFS
	#undef CF_ENUM
};

struct https_response_t
{
	int code;
	size_t content_len;
	const char* content;
	Array<HttpsHeader> headers;

	int transfer_encoding_flags;

	/**
	* Convenience function to find a specific header. Returns true if the header was found, and false
	* otherwise.
	*/
	CUTE_INLINE bool find_header(const char* header_name, HttpsHeader* header_out = NULL) const;
};


CUTE_INLINE Https* https_get(const char* host, const char* port, const char* uri, CF_Result* err = NULL, bool verify_cert = true) { return cf_https_get(host, port, uri, err, verify_cert); }
CUTE_INLINE Https* https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, CF_Result* err = NULL, bool verify_cert = true) { return cf_https_post(host, port, uri, data, size, err, verify_cert); }
CUTE_INLINE void https_destroy(Https* https) { cf_https_destroy(https); }
CUTE_INLINE HttpsState https_state(Https* https) { return cf_https_state(https); }
CUTE_INLINE size_t https_process(Https* https) { return cf_https_process(https); }

CUTE_API const https_response_t* CUTE_CALL https_response(Https* https);

CUTE_INLINE bool https_strcmp(const char* lit, HttpsString string) { return cf_https_strcmp(lit, string); }

CUTE_INLINE bool https_response_find_header(const https_response_t* response, const char* header_name, HttpsHeader* header_out)
{
	*header_out = { 0 };

	for (int i = 0; i < response->headers.count(); ++i) {
		CF_HttpsHeader header = response->headers[i];
		if (!cf_https_strcmp(header_name, header.name)) {
			if (header_out) *header_out = header;
			return true;
		}
	}
	return false;
}

CUTE_INLINE bool https_response_t::find_header(const char* header_name, HttpsHeader* header_out) const
{
	return https_response_find_header(this, header_name, header_out);
}

}

#endif // CUTE_CPP

#endif // CUTE_HTTPS_H
