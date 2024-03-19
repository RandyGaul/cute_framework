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
 * @remarks  You may create a request by calling either `cf_https_get` or `cf_https_post`.
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
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_HttpsRequest CF_CALL cf_https_get(const char* host, int port, const char* uri, bool verify_cert);

/**
 * @function cf_https_post
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_HttpsRequest CF_CALL cf_https_post(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert);

/**
 * @function cf_https_add_header
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_https_add_header(CF_HttpsRequest request, const char* name, const char* value);

/**
 * @function cf_https_destroy
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API void CF_CALL cf_https_destroy(CF_HttpsRequest request);

/**
 * @enum     CF_HttpsResult
 * @category web
 * @brief    The states of power for the application.
 * @related  TODO
 */
#define CF_HTTPS_RESULT_DEFS \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_BAD_CERTIFICATE,                    -7) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_CERTIFICATE_EXPIRED,                -6) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_BAD_HOSTNAME,                       -5) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_CANNOT_VERIFY_CA_CHAIN,             -4) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_NO_MATCHING_ENCRYPTION_ALGORITHMS,  -3) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_SOCKET_ERROR,                       -2) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_FAILED,                             -1) \
	/* @entry ... */                                             \
	CF_ENUM(HTTPS_RESULT_PENDING,                             0) \
	/* @entry ... */                                             \
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
 * @related  TODO
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
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
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
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API int CF_CALL cf_https_response_code(CF_HttpsResponse response);

/**
 * @function cf_https_response_content_length
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API int CF_CALL cf_https_response_content_length(CF_HttpsResponse response);

/**
 * @function cf_https_response_content
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API char* CF_CALL cf_https_response_content(CF_HttpsResponse response);

/**
 * @function cf_https_response_find_header
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API CF_HttpsHeader CF_CALL cf_https_response_find_header(CF_HttpsResponse response, const char* header_name);

/**
 * @function cf_https_response_headers_count
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
 */
CF_API int CF_CALL cf_https_response_headers_count(CF_HttpsResponse response);

/**
 * @function cf_https_response_headers
 * @category web
 * @brief    TODO
 * @return   TODO
 * @remarks  TODO
 * @related  TODO
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

}

#endif // CF_CPP

#endif // CF_EMSCRIPTEN

#endif // CF_HTTPS_H
