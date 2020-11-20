/*
	Cute Framework
	Copyright (C) 2021 Randy Gaul https://randygaul.net

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

#include <cute_defines.h>
#include <cute_error.h>
#include <cute_c_runtime.h>
#include <cute_array.h>

namespace cute
{

/**
 * Represents a single HTTPS request for clients to talk with web servers. POST and GET requests are
 * supported for when you just need a basic way to communicate over HTTPS. Insecure HTTP is not supported,
 * but cert verification can be skipped (not recommended).
 * 
 * Supports chunked encoding. Does not support trailing headers, 100-continue, keep-alive, or other
 * "advanced" HTTP features.
 * 
 * The design of this API comes mainly from Mattias Gustavsson's http.h single-file C header.
 * https://github.com/mattiasgustavsson/libs/blob/main/http.h
 * 
 * Here is a full working example.
 * 
 *    error_t err;
 *    https_t* https = https_get("raw.githubusercontent.com", "443", "/RandyGaul/cute_framework/main/src/cute_https.h", &err);
 *    if (https) {
 *        while (https_state(https) == HTTPS_STATE_PENDING) {
 *            size_t bytes_read = https_process(https);
 *            printf("Received %zu bytes...\n", bytes_read);
 *        }
 *        if (https_state(https) == HTTPS_STATE_COMPLETED) {
 *            const https_response_t* response = https_response(https);
 *            printf("%s", response->content);
 *        }
 *        https_destroy(https);
 *    } else {
 *        printf("HTTPS request failed: %s\n", err.details);
 *    }
 */
struct https_t;

/**
 * Initiates a GET request for the specified host (website address) and a given uri. For example if we wanted
 * to retrieve this header from github, we could use this `host` "raw.githubusercontent.com" with this `uri`
 * "/RandyGaul/cute_framework/main/src/cute_https.h" at port 443 (standard HTTPS port).
 * 
 * Any errors are optionally reported through the `err` parameter.
 * 
 * `verify_cert` will verify the server's x509 certificate, but can be disabled (dangerous).
 * 
 * Returns an `https_t` pointer which needs to be processed with `https_process` and cleaned up by `https_destroy`.
 * 
 * `host` and `port` are unused when building with emscripten -- this is since an XMLHttpRequest is used
 * underneath, meaning only files from the server this code came from can be loaded, and as such the `uri`
 * should only be a relative path on the server.
 */
CUTE_API https_t* CUTE_CALL https_get(const char* host, const char* port, const char* uri, error_t* err = NULL, bool verify_cert = true);

/**
 * Initiates a POST request for the specified host (website address) and a given uri. The content of the post
 * is in `data`, which can be `NULL` if `size` is 0.
 * 
 * Any errors are optionally reported through the `err` parameter.
 * 
 * `verify_cert` will verify the server's x509 certificate, but can be disabled (dangerous).
 * 
 * Returns an `https_t` pointer which needs to be processed with `https_process` and cleaned up by `https_destroy`.
 * 
 * `host` and `port` are unused when building with emscripten -- this is since an XMLHttpRequest is used
 * underneath, meaning only files from the server this code came from can be loaded, and as such the `uri`
 * should only be a relative path on the server.
 */
CUTE_API https_t* CUTE_CALL https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, error_t* err = NULL, bool verify_cert = true);

/**
 * Frees up all memory and closes the underlying HTTPS connection if still open.
 */
CUTE_API void CUTE_CALL https_destroy(https_t* https);

enum https_state_t
{
	HTTPS_STATE_PENDING,   // Keep calling `https_process`.
	HTTPS_STATE_COMPLETED, // The response has been acquired, retrieve it with `https_response`.
	HTTPS_STATE_FAILED,    // The request has failed, the only valid operation left is `https_destroy`.
};

/**
 * Returns the current state of the `https` object. This is used mainly for calling `https_process`.
 */
CUTE_API https_state_t CUTE_CALL https_state(https_t* https);

/**
 * Since this API uses non-blocking sockets `https_process` needs to be call periodically after `https_get`
 * or `https_post` is called for as long as `https_state` returns `HTTPS_STATE_PENDING`. You can call
 * this function from within its own loop, put it on another thread within a loop, or call it once per
 * game tick -- whichever you prefer.
 * 
 * Returns the bytes recieved so far.
 */
CUTE_API size_t CUTE_CALL https_process(https_t* https);

struct https_string_t
{
	const char* ptr;
	size_t len;
};

struct https_header_t
{
	https_string_t name;
	https_string_t content;
};

enum transfer_encoding_t
{
	TRANSFER_ENCODING_NONE                = 0x00,
	TRANSFER_ENCODING_CHUNKED             = 0x01,
	TRANSFER_ENCODING_GZIP                = 0x02,
	TRANSFER_ENCODING_DEFLATE             = 0x04,
	TRANSFER_ENCODING_DEPRECATED_COMPRESS = 0x08,
};

/**
 * Represents the response from a server after a successful process loop via `https_process`, where the
 * status returned from `https_state` is `HTTPS_STATE_COMPLETED`.
 */
struct https_response_t
{
	int code;
	size_t content_len;
	const char* content;
	array<https_header_t> headers;

	/**
	 * Flags from `transfer_encoding_t`. For example, if content is gzip'd, you can tell by using
	 * something like so: `bool is_gzip = !!(response->transfer_encoding & TRANSFER_ENCODING_GZIP);`
	 * 
	 * Please note that if the encoding is `TRANSFER_ENCODING_CHUNKED` the `content` buffer will not
	 * contain any chunked encoding -- all chunked data has been decoded already. For gzip/deflate
	 * the `content` buffer will need to be decompressed by you.
	 */
	int transfer_encoding_flags;

	/**
	 * Convenience function to find a specific header. Returns true if the header was found, and false
	 * otherwise.
	 */
	CUTE_INLINE bool find_header(const char* header_name, https_header_t* header_out = NULL) const;
};

/**
 * A response can be retrieved from the `https` object after `https_state` returns `HTTPS_STATE_COMPLETED`.
 * Calling this function otherwise will get you a NULL pointer returned. This will get cleaned up automatically
 * when `https_destroy` is called.
 */
CUTE_API const https_response_t* CUTE_CALL https_response(https_t* https);

// -------------------------------------------------------------------------------------------------
// Inline functions.

CUTE_INLINE bool https_strcmp(const char* lit, https_string_t string)
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

CUTE_INLINE bool https_response_t::find_header(const char* header_name, https_header_t* header_out) const
{
	*header_out = { 0 };
	for (int i = 0; i < headers.count(); ++i) {
		https_header_t header = headers[i];
		if (!https_strcmp(header_name, header.name)) {
			if (header_out) *header_out = header;
			return true;
		}
	}
	return false;
}

}
