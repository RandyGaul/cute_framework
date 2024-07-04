/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_HTTPS_H
#define CF_HTTPS_H

#include "cute_defines.h"
#include "cute_result.h"
#include "cute_c_runtime.h"
#include "cute_array.h"
#include "cute_hashtable.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef CF_EMSCRIPTEN

/**
 * @struct   CF_HttpsRequest
 * @category web
 * @brief    Represents an [HTTPS request](https://www.ibm.com/docs/en/cics-ts/5.3?topic=protocol-http-requests).
 * @example  > Creating a request and waiting for a response.
 *     int main(int argc, char* argv[])
 *     {
 *         const char* hostname = "www.google.com";
 *         //const char* hostname = "badssl.com";
 *         //const char* hostname = "expired.badssl.com";
 *         //const char* hostname = "wrong.host.badssl.com";
 *         //const char* hostname = "self-signed.badssl.com";
 *         //const char* hostname = "untrusted-root.badssl.com";
 *         CF_HttpsRequest request = cf_https_get(hostname, 443, "/", true);
 *     
 *         while (1) {
 *             CF_HttpsResult state = cf_https_process(request);
 *             if (state < 0) {
 *                 printf("%s\n", cf_https_result_to_string(state));
 *                 cf_https_destroy(request);
 *                 return -1;
 *             }
 *             if (state == CF_HTTPS_RESULT_OK) {
 *                 break;
 *             }
 *         }
 *     
 *         CF_HttpsResponse response = cf_https_response(request);
 *         const char* content = cf_https_response_content(response);
 *         int length = cf_https_response_content_length(response);
 *         printf("%.*s", length, content);
 *         cf_https_destroy(request);
 *     
 *         return 0;
 *     }
 * @remarks  You may create a request by calling either `cf_https_get` or `cf_https_post`. It is intended to continually
 *           call `cf_https_process` in a loop until the request generates a response, or fails.
 * @related  CF_HttpsRequest CF_HttpsResponse cf_https_get cf_https_post
 */
typedef struct CF_HttpsRequest { uint64_t id; } CF_HttpsRequest;
// @end

/**
 * @struct   CF_HttpsResponse
 * @category web
 * @brief    Represents an [HTTPS response](https://www.ibm.com/docs/en/cics-ts/5.2?topic=protocol-http-responses).
 * @remarks  The response may be fetched from a `CF_HttpsRequest` by calling `cf_https_response`.
 * @related  CF_HttpsRequest CF_HttpsResponse cf_https_response cf_https_response_find_header cf_https_response_content
 */
typedef struct CF_HttpsResponse { uint64_t id; } CF_HttpsResponse;
// @end

/**
 * @struct   CF_HttpsHeader
 * @category web
 * @brief    Represents an [HTTPS header](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers).
 * @remarks  Headers may be fetched from a `CF_HttpsResponse` by calling `cf_https_response_find_header`, or `cf_https_response_headers`.
 * @related  CF_HttpsRequest CF_HttpsResponse CF_HttpsHeader cf_https_response_find_header cf_https_response_headers
 */
typedef struct CF_HttpsHeader
{
	/* @member The name of the header, also known as the key of the header. */
	const char* name;

	/* @member The value of the header. */
	const char* value;
} CF_HttpsHeader;
// @end

/**
 * @function cf_https_get
 * @category web
 * @brief    Creates an HTTPS GET request.
 * @param    host         The address of the host, e.g. "www.google.com".
 * @param    port         The port number to connect over, typically 443 for common web traffic.
 * @param    uri          The address of the resource the request references, e.g. "/index.html".
 * @param    verify_cert  Recommended as true. Set to true to verify the server certificate (you want this on).
 * @return   Returns a `CF_HttpsRequest` for processing the get request and receiving a response.
 * @remarks  You should continually call `cf_https_process` on the `CF_HttpsRequest`. See `CF_HttpsRequest` for details.
 * @related  CF_HttpsRequest cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response
 */
CF_API CF_HttpsRequest CF_CALL cf_https_get(const char* host, int port, const char* uri, bool verify_cert);


/**
 * @function cf_https_post
 * @category web
 * @brief    Creates an HTTPS POST request.
 * @param    host            The address of the host, e.g. "www.google.com".
 * @param    port            The port number to connect over, typically 443 for common web traffic.
 * @param    uri             The address of the resource the request references, e.g. "/index.html".
 * @param    content         The content to send along with the POST request.
 * @param    content_length  Length in bytes of the `content` string.
 * @param    verify_cert  Recommended as true. Set to true to verify the server certificate (you want this on).
 * @return   Returns a `CF_HttpsRequest` for processing the get request and receiving a response.
 * @remarks  You should continually call `cf_https_process` on the `CF_HttpsRequest`. See `CF_HttpsRequest` for details.
 * @related  CF_HttpsRequest cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response
 */
CF_API CF_HttpsRequest CF_CALL cf_https_post(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert);

/**
 * @function cf_https_add_header
 * @category web
 * @brief    Adds a header to the request.
 * @param    request    The request.
 * @param    name       The key value of the header.
 * @param    value      String representation of the header's value.
 * @remarks  You should call this before calling `cf_https_process`. Calling this after `cf_https_process` will break things.
 * @related  CF_HttpsRequest cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response
 */
CF_API void CF_CALL cf_https_add_header(CF_HttpsRequest request, const char* name, const char* value);

/**
 * @function cf_https_destroy
 * @category web
 * @brief    Destroys a `CF_HttpsRequest`.
 * @related  CF_HttpsRequest cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response
 */
CF_API void CF_CALL cf_https_destroy(CF_HttpsRequest request);

/**
 * @enum     CF_HttpsResult
 * @category web
 * @brief    Status of a `CF_HttpsRequest`.
 * @remarks  Intended to be used in a loop, along with `cf_https_process`. See `CF_HttpsRequest`.
 * @related  CF_HttpsRequest cf_https_process cf_https_result_to_string
 */
#define CF_HTTPS_RESULT_DEFS \
	/* @entry The server has an invalid certificate. */          \
	CF_ENUM(HTTPS_RESULT_BAD_CERTIFICATE,                    -7) \
	/* @entry The server's certificate has expired. */           \
	CF_ENUM(HTTPS_RESULT_CERTIFICATE_EXPIRED,                -6) \
	/* @entry The name of the host is invalid. */                \
	CF_ENUM(HTTPS_RESULT_BAD_HOSTNAME,                       -5) \
	/* @entry Unable to verify the host's cert. */               \
	CF_ENUM(HTTPS_RESULT_CANNOT_VERIFY_CA_CHAIN,             -4) \
	/* @entry Unable to form a secure connection. */             \
	CF_ENUM(HTTPS_RESULT_NO_MATCHING_ENCRYPTION_ALGORITHMS,  -3) \
	/* @entry Socket on the local machine failed. */             \
	CF_ENUM(HTTPS_RESULT_SOCKET_ERROR,                       -2) \
	/* @entry Unknown error. */                                  \
	CF_ENUM(HTTPS_RESULT_FAILED,                             -1) \
	/* @entry Continue calling `cf_https_process`. */            \
	CF_ENUM(HTTPS_RESULT_PENDING,                             0) \
	/* @entry The result has finished, you may stop calling `cf_https_process`, and fetch the response via `cf_https_response`. */ \
	CF_ENUM(HTTPS_RESULT_OK,                                  1) \
	/* @end */

typedef enum CF_HttpsResult
{
	#define CF_ENUM(K, V) CF_##K = V,
	CF_HTTPS_RESULT_DEFS
	#undef CF_ENUM
} CF_HttpsResult;

/**
 * @function cf_https_result_to_string
 * @category web
 * @brief    Convert an enum `CF_HttpsState` to a c-style string.
 * @param    state        The state to convert to a string.
 * @related  CF_HttpsRequest cf_https_process cf_https_result_to_string
 */
CF_INLINE const char* cf_https_result_to_string(CF_HttpsResult state)
{
	switch (state) {
	#define CF_ENUM(K, V) case CF_##K: return CF_STRINGIZE(CF_##K);
	CF_HTTPS_RESULT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

/**
 * @function cf_https_process
 * @category web
 * @brief    Processes a request and generates a response.
 * @return   Returns the status of the request `CF_HttpsResult`.
 * @remarks  You should call this function in a loop. See `CF_HttpsResult`.
 * @related  CF_HttpsResult cf_https_process cf_https_get cf_https_post cf_https_response
 */
CF_API CF_HttpsResult CF_CALL cf_https_process(CF_HttpsRequest request);

/**
 * @function cf_https_response
 * @category web
 * @brief    Returns a `CF_HttpsResponse` from a request.
 * @remarks  A response can be retrieved from the `https` object after `cf_https_state` returns `CF_HTTPS_STATE_COMPLETED`.
 *           Calling this function otherwise will get you a NULL pointer returned. This will get cleaned up automatically
 *           when `cf_https_destroy` is called.
 * @related  CF_Https cf_https_get cf_https_post cf_https_destroy cf_https_process cf_https_response cf_https_state
 */
CF_API CF_HttpsResponse CF_CALL cf_https_response(CF_HttpsRequest request);

/**
 * @function cf_https_response_code
 * @category web
 * @brief    Returns the HTTP code for the response.
 * @related  CF_HttpsResponse cf_https_response cf_https_response_code
 */
CF_API int CF_CALL cf_https_response_code(CF_HttpsResponse response);

/**
 * @function cf_https_response_content_length
 * @category web
 * @brief    Returns the length of the response content.
 * @related  CF_HttpsResponse cf_https_response cf_https_response_content
 */
CF_API int CF_CALL cf_https_response_content_length(CF_HttpsResponse response);

/**
 * @function cf_https_response_content
 * @category web
 * @brief    Returns the content of the response as a string.
 * @related  CF_HttpsResponse cf_https_response_content cf_https_response_content_length
 */
CF_API char* CF_CALL cf_https_response_content(CF_HttpsResponse response);

/**
 * @function cf_https_response_find_header
 * @category web
 * @brief    Searches for and returns a header by name.
 * @param    response     The HTTP response.
 * @param    header_name  The name of the header to search for.
 * @related  CF_HttpsHeader CF_HttpsResponse cf_https_response_find_header cf_https_response_headers
 */
CF_API CF_HttpsHeader CF_CALL cf_https_response_find_header(CF_HttpsResponse response, const char* header_name);

/**
 * @function cf_https_response_headers_count
 * @category web
 * @brief    Returns the number of headers in the response.
 * @remarks  Intended to be used with `cf_https_response_headers`.
 * @related  CF_HttpsHeader CF_HttpsResponse cf_https_response_find_header cf_https_response_headers
 */
CF_API int CF_CALL cf_https_response_headers_count(CF_HttpsResponse response);

/**
 * @function cf_https_response_headers
 * @category web
 * @brief    Returns an array of response headers.
 * @remarks  Intended to be used with `cf_https_response_headers_count`. Do not free this array, it will
 *           get cleaned up when the originating `CF_HttpsRequest` is destroyed via `cf_https_destroy`.
 *           If you're familiar with `htbl` you may treat this pointer as a hashtable key'd by strings.
 * @related  CF_HttpsHeader CF_HttpsResponse cf_https_response_find_header cf_https_response_headers
 */
CF_API htbl const CF_HttpsHeader* CF_CALL cf_https_response_headers(CF_HttpsResponse response);

#ifdef __cplusplus
}
#endif // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

#ifdef CF_CPP

namespace Cute
{

using HttpsRequest = CF_HttpsRequest;
using HttpsResponse = CF_HttpsResponse;
using HttpsHeader = CF_HttpsHeader;

CF_INLINE HttpsRequest https_get(const char* host, int port, const char* uri, bool verify_cert = true) { return cf_https_get(host, port, uri, verify_cert); }
CF_INLINE HttpsRequest https_post(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert = true) { return cf_https_post(host, port, uri, content, content_length, verify_cert); }
CF_INLINE void https_add_header(HttpsRequest request, const char* name, const char* value) { cf_https_add_header(request, name, value); }
CF_INLINE void https_destroy(HttpsRequest request) { cf_https_destroy(request); }

using HttpsResult = CF_HttpsResult;
#define CF_ENUM(K, V) CF_INLINE constexpr HttpsResult K = CF_##K;
CF_HTTPS_RESULT_DEFS
#undef CF_ENUM

CF_INLINE const char* to_string(HttpsResult result)
{
	switch (result) {
	#define CF_ENUM(K, V) case CF_##K: return #K;
	CF_HTTPS_RESULT_DEFS
	#undef CF_ENUM
	default: return NULL;
	}
}

CF_INLINE HttpsResult https_process(HttpsRequest request) { return cf_https_process(request); }
CF_INLINE HttpsResponse https_response(HttpsRequest request) { return cf_https_response(request); }
CF_INLINE int https_response_code(HttpsResponse response) { return cf_https_response_code(response); }
CF_INLINE int https_response_content_length(HttpsResponse response) { return cf_https_response_content_length(response); }
CF_INLINE char* https_response_content(HttpsResponse response) { return cf_https_response_content(response); }
CF_INLINE HttpsHeader https_response_find_header(HttpsResponse response, const char* header_name) { return cf_https_response_find_header(response, header_name); }
CF_INLINE int https_response_headers_count(HttpsResponse response) { return cf_https_response_headers_count(response); }
CF_INLINE htbl const HttpsHeader* https_response_headers(HttpsResponse response) { return cf_https_response_headers(response); }

}

#endif // CF_CPP

#endif // CF_EMSCRIPTEN

#endif // CF_HTTPS_H
