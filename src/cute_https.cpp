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

#include <cute_https.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_array.h>
#include <cute_string.h>
#include <cute_coroutine.h>

#include <internal/cute_alloc_internal.h>

#include <SDL.h>

#define CUTE_TLS_IMPLEMENTATION
#include <cute/cute_tls.h>

using namespace Cute;

// TODO - Rewrite with string view wrapper class, instead of dynamic strings, as an optimization.

#ifndef CF_EMSCRIPTEN

#define CF_RESPONSE_CHUNKED             1
#define CF_RESPONSE_GZIP                2
#define CF_RESPONSE_DEFLATE             4
#define CF_RESPONSE_DEPRECATED_COMPRESS 8

typedef struct CF_Response
{
	const char* in = NULL;
	const char* end = NULL;
	int code = 0;
	bool ok = true;
	int flags = 0;
	bool gzip = false;
	bool deflate = false;
	int content_length = 0;
	bool trailers = false;
	Map<const char*, CF_HttpsHeader> headers;
	String parse;
	String content;
} CF_Response;

typedef struct CF_Request
{
	const char* host = NULL;
	int port = 0;
	const char* uri = NULL;
	int content_length = 0;
	const void* content = NULL;
	bool verify_cert = true;
	CF_HttpsResult result = CF_HTTPS_RESULT_PENDING;
	CF_Response response = { };
	CF_Coroutine co = { };
	TLS_Connection connection = { };
	Array<CF_HttpsHeader> headers;
} CF_Request;

static CF_INLINE CF_HttpsResult s_tls_state_to_https_result(TLS_State state)
{
	switch (state) {
	case TLS_STATE_BAD_CERTIFICATE                       : return CF_HTTPS_RESULT_BAD_CERTIFICATE;
	case TLS_STATE_SERVER_ASKED_FOR_CLIENT_CERTS         : return CF_HTTPS_RESULT_FAILED;
	case TLS_STATE_CERTIFICATE_EXPIRED                   : return CF_HTTPS_RESULT_CERTIFICATE_EXPIRED;
	case TLS_STATE_BAD_HOSTNAME                          : return CF_HTTPS_RESULT_BAD_HOSTNAME;
	case TLS_STATE_CANNOT_VERIFY_CA_CHAIN                : return CF_HTTPS_RESULT_CANNOT_VERIFY_CA_CHAIN;
	case TLS_STATE_NO_MATCHING_ENCRYPTION_ALGORITHMS     : return CF_HTTPS_RESULT_NO_MATCHING_ENCRYPTION_ALGORITHMS;
	case TLS_STATE_INVALID_SOCKET                        : return CF_HTTPS_RESULT_SOCKET_ERROR;
	case TLS_STATE_UNKNOWN_ERROR                         : return CF_HTTPS_RESULT_FAILED;
	case TLS_STATE_DISCONNECTED                          : return CF_HTTPS_RESULT_OK;
	case TLS_STATE_DISCONNECTED_BUT_PACKETS_STILL_REMAIN : return CF_HTTPS_RESULT_PENDING;
	case TLS_STATE_PENDING                               : return CF_HTTPS_RESULT_PENDING;
	case TLS_STATE_CONNECTED                             : return CF_HTTPS_RESULT_PENDING;
	case TLS_STATE_PACKET_QUEUE_FILLED                   : return CF_HTTPS_RESULT_PENDING;
	}
	return CF_HTTPS_RESULT_FAILED;
}

static void s_get_line(Coroutine co, CF_Response* response)
{
	response->parse.clear();
	while (1) {
		while (response->in != response->end) {
			const char* found_cr = (const char*)CF_MEMCHR(response->in, '\r', response->end - response->in);
			if (found_cr) {
				response->parse.append(response->in, found_cr);
				response->in = found_cr + 1;
				if (*response->in == '\n') {
					response->in++;
					return;
				}
			} else {
				response->parse.append(response->in, response->end);
				response->in = response->end;
			}
		}

		if (response->in == response->end) {
			coroutine_yield(co);
		}
	}
}

static bool s_status(Coroutine co, CF_Response* response)
{
	s_get_line(co, response);
	const char* in = response->parse.c_str();

	// Only accept HTTP version 1.1.
	if (CF_MEMCMP(in, "HTTP/1.1 ", 9)) {
		return false;
	}

	// Parse status code.
	in += 9;
	const char* next = CF_STRCHR(in, ' ');
	int num_digits = (int)(next - in);
	if (num_digits != 3) return false;
	int code = (int)CF_STRTOLL(in, NULL, 10);
	if (!code || code > 999) return false;
	response->code = code;

	// Just skip parsing the status phrase.
	response->parse.clear();

	return true;
}

static bool s_header(Coroutine co, CF_Response* response)
{
	const char* in = response->parse.c_str();

	// Name of the header.
	const char* next = CF_STRCHR(in, ':');
	if (!next) return false;
	String name = String(in, next);
	name.trim();
	in = next + 1;

	// Content of the header.
	String content = next + 1;
	content.trim();

	// Handle certain resposne headers.
	if (name == "Content-Length") {
		if (response->flags) {
			// Content-Length and transfer encoding flags are not compatible.
			return false;
		}
		response->content_length = content.to_int();
	} else if (name == "Transfer-Encoding") {
		Array<String> encodings = content.split(',');
		for (int i = 0; i < encodings.size(); ++i) {
			int prev_flags = response->flags;
			if (content == "deflate") {
				if (response->flags & CF_RESPONSE_GZIP) return false;
				response->flags |= CF_RESPONSE_DEFLATE;
			} else if (content == "gzip") {
				if (response->flags & CF_RESPONSE_DEFLATE) return false;
				response->flags |= CF_RESPONSE_GZIP;
			} else if (content == "chunked") {
				if (response->content_length > 0) {
					// Content-Length and transfer encoding flags are not compatible.
					return false;
				}
				response->flags |= CF_RESPONSE_CHUNKED;
			} else if (content.len() > 0) {
				// Invalid encoding found.
				return false;
			}
			if ((prev_flags & CF_RESPONSE_CHUNKED) && (response->flags != prev_flags)) {
				// Chunked encoding must be specified last.
				return false;
			}
		}
	} else if (name == "Trailer") {
		response->trailers = true;
		// Don't bother parsing the trailer list, just forward them all along to the user when get them later.
		// This isn't technically compliant, but it's simple and good enough.
	}

	// Add header to response collection.
	CF_HttpsHeader header;
	header.name = sintern(name.c_str()); // Potentially malicious memory bloat here.
	header.value = content.steal();
	if (!response->headers.has(header.name)) {
		response->headers.insert(header.name, header);
	} else {
		// BUG - Duplicate headers are ignored!
	}
	return true;
}

static void s_headers(Coroutine co, CF_Response* response)
{
	while (1) {
		s_get_line(co, response);
		if (response->parse.len() == 0) {
			break;
		}
		if (!s_header(co, response)) {
			response->ok = false;
			return;
		}
	}
}

static void s_decode(Coroutine co)
{
	CF_Response* response = (CF_Response*)coroutine_get_udata(co);
	// Read status line.
	if (!s_status(co, response)) {
		response->ok = false;
		return;
	}

	// Read headers.
	s_headers(co, response);
	if (!response->ok) return;

	// Read in response body.
	if (response->flags & CF_RESPONSE_CHUNKED) {
		// Chunked decoding.
		while (1) {
			// Parse chunk size (hex).
			s_get_line(co, response);
			char* ptr;
			uint64_t chunk_size = CF_STRTOLL(response->parse.c_str(), &ptr, 16);
			if (ptr == response->parse.c_str()) {
				response->ok = false;
				return;
			}

			// Skip any chunk extensions (these are optional by definition).

			if (chunk_size == 0) {
				// Final chunk reached.
				break;
			}

			// Read in chunk data.
			while (1) {
				if (response->in == response->end) {
					coroutine_yield(co);
				}

				uint64_t bytes = min(chunk_size, response->end - response->in);
				response->content.append(response->in, response->in + bytes);
				response->in += bytes;

				int len = response->content.len();
				if (len == chunk_size) {
					break;
				}
			}

			s_get_line(co, response);
			if (response->parse.len() != 0) {
				// Chunks are supposed to end with CRLF.
				response->ok = false;
				return;
			}
		}
	} else {
		// Read in content bytes (non-chunked).
		uint64_t bytes_read = 0;
		while (1) {
			uint64_t bytes = response->end - response->in;
			response->content.append(response->in, response->in + bytes);
			bytes_read += bytes;

			if (bytes_read == response->content_length) {
				break;
			} else if (bytes_read > response->content_length) {
				// Content length did not match expectation.
				response->ok = false;
				return;
			} else {
				coroutine_yield(co);
			}
		}
	}

	// Note what compression was used.
	// TODO - Just decompress things ourselves. Code for DEFLATE already exists in cute_aseprite.h, but,
	//        gzip header parsing + CRC would need to be written (super annoying). So, this will probably
	//        never get implemented.
	if (response->flags & CF_RESPONSE_DEFLATE) {
		response->deflate = true;
	} else if (response->flags & CF_RESPONSE_GZIP) {
		response->gzip = true;
	}

	// Parsing code implemented incorrectly.
	CF_ASSERT(!(response->deflate && response->gzip));

	if (response->trailers) {
		// Read in any trailing headers.
		s_headers(co, response);
	} else {
		s_get_line(co, response);
		if (response->parse.len() != 0) {
			// End of response doesn't have expected final empty-line CRLF.
			response->ok = false;
			return;
		}
	}
}

static void s_https_process(Coroutine co)
{
	CF_Request* request = (CF_Request*)coroutine_get_udata(co);

	// Start up the TLS connection.
	request->connection = tls_connect(request->host, request->port);
	while (1) {
		TLS_State state = tls_process(request->connection);
		request->result = s_tls_state_to_https_result(state);
		if (state == TLS_STATE_CONNECTED) {
			// Connected!
			break;
		} else if (state < 0) {
			return;
		}
		coroutine_yield(co);
	}

	// Send of the HTTP request.
	String s;
	if (request->content) {
		s = String::fmt(
			"POST %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Connection: close\r\n"
			"TE: trailers, deflate, gzip\r\n"
			"Content-Length: %d\r\n"
			"\r\n",
			request->uri, request->host, request->content_length
		);
		const char* content = (const char*)request->content;
		s.append(content, content + request->content_length); 
	} else {
		s = String::fmt(
			"GET %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Connection: close\r\n"
			"TE: trailers, deflate, gzip\r\n"
			"\r\n",
			request->uri, request->host
		);
	}

	if (tls_send(request->connection, s.c_str(), s.len()) < 0) {
		request->result = CF_HTTPS_RESULT_SOCKET_ERROR;
		return;
	}

	// Receive all response bytes.
	CF_Response* response = &request->response;
	CF_Coroutine decoder = make_coroutine(s_decode, 0, response);
	char buf[TLS_MAX_PACKET_SIZE];
	while (1) {
		TLS_State state = tls_process(request->connection);
		if (state == TLS_STATE_DISCONNECTED) {
			tls_disconnect(request->connection);
			request->connection.id = 0;
			break;
		}

		int bytes = tls_read(request->connection, buf, sizeof(buf));
		if (bytes < 0) {
			request->result = CF_HTTPS_RESULT_SOCKET_ERROR;
			destroy_coroutine(decoder);
			return;
		}
		if (bytes) {
			// Parse the response as it arrives, packet-by-packet.
			response->in = buf;
			response->end = buf + bytes;
			response->parse.clear();
			coroutine_resume(decoder); // s_decode
			if (!response->ok) {
				request->result = CF_HTTPS_RESULT_FAILED;
				destroy_coroutine(decoder);
				return;
			}
		}
		coroutine_yield(co);
	}

	request->result = CF_HTTPS_RESULT_OK;
	destroy_coroutine(decoder);
}

static CF_Request* s_request(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert)
{
	CF_Request* request = CF_NEW(CF_Request);
	request->host = host;
	request->port = port;
	request->uri = uri;
	request->content = content;
	request->content_length = content_length;
	request->verify_cert = verify_cert;
	request->co = make_coroutine(s_https_process, CF_MB, request);
	return request;
}

CF_HttpsRequest cf_https_get(const char* host, int port, const char* uri, bool verify_cert)
{
	CF_Request* request = s_request(host, port, uri, NULL, 0, verify_cert);
	CF_HttpsRequest result;
	result.id = (uint64_t)request;
	return result;
}

CF_HttpsRequest cf_https_post(const char* host, int port, const char* uri, const void* content, int content_length, bool verify_cert)
{
	CF_Request* request = s_request(host, port, uri, content, content_length, verify_cert);
	CF_HttpsRequest result;
	result.id = (uint64_t)request;
	return result;
}

void cf_https_destroy(CF_HttpsRequest request_handle)
{
	CF_Request* request = (CF_Request*)request_handle.id;
	if (request->connection.id) tls_disconnect(request->connection);
	destroy_coroutine(request->co);
	for (int i = 0; i < request->response.headers.count(); ++i) {
		CF_HttpsHeader header = request->response.headers.items()[i];
		char* val = (char*)header.value;
		sfree(val); // This was stolen earlier from a String, so we manually cleanup here.
	}
	request->~CF_Request();
	cf_free(request);
}

void cf_https_add_header(CF_HttpsRequest request_handle, const char* name, const char* value)
{
	CF_Request* request = (CF_Request*)request_handle.id;
	CF_HttpsHeader& header = request->headers.add();
	header.name = name;
	header.value = value;
}

CF_HttpsResult cf_https_process(CF_HttpsRequest request_handle)
{
	CF_Request* request = (CF_Request*)request_handle.id;
	coroutine_resume(request->co); // s_https_process
	return request->result;
}

CF_HttpsResponse cf_https_response(CF_HttpsRequest request_handle)
{
	CF_Request* request = (CF_Request*)request_handle.id;
	CF_HttpsResponse result;
	result.id = (uint64_t)&request->response;
	return result;
}

int cf_https_response_code(CF_HttpsResponse response_handle)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->code;
}

int cf_https_response_content_length(CF_HttpsResponse response_handle)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->content.len();
}

char* cf_https_response_content(CF_HttpsResponse response_handle)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->content.c_str();
}

CF_HttpsHeader cf_https_response_find_header(CF_HttpsResponse response_handle, const char* header_name)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->headers.get(sintern(header_name));
}

int cf_https_response_headers_count(CF_HttpsResponse response_handle)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->headers.count();
}

htbl const CF_HttpsHeader* cf_https_response_headers(CF_HttpsResponse response_handle)
{
	CF_Response* response = (CF_Response*)response_handle.id;
	return response->headers.items();
}

#else // CF_EMSCRIPTEN

struct CF_Https
{
	const char* host = NULL;
	const char* uri = NULL;
	CF_HttpsState state = CF_HTTPS_STATE_PENDING; // TODO - Atomic this.
	const void* data = NULL;
	size_t size = 0;
	const char** unpacked_headers = NULL;
	int response_code = 0;
	cf_internal_https_response_t response;
	bool request_sent = false;
	int bytes_read = 0; // TODO - Atomic this.
};

extern "C" {

EMSCRIPTEN_KEEPALIVE void s_bytes_downloaded(void* https_ptr, size_t size)
{
	CF_Https* https = (CF_Https*)https_ptr;
	https->bytes_read = size;
}

EMSCRIPTEN_KEEPALIVE void s_response_code(void* https_ptr, int code)
{
	CF_Https* https = (CF_Https*)https_ptr;
	https->response.code = code;
}

static char* s_trim(char* s)
{
	size_t i = CF_STRLEN(s);
	while (i && *s == ' ') {
		++s;
		--i;
	}
	while (i && s[i - 1] == ' ') {
		s[i - 1] = 0;
		--i;
	}
	return s;
}

static const char** s_unpack_headers(const char* headers)
{
	size_t count = 0;
	for (const char* pos = CF_STRCHR(headers, '\n'); pos; pos = CF_STRCHR(pos + 1, '\n')) {
		count++;
	}

	char **unpacked_headers = (char**)CF_ALLOC(sizeof(char*) * ((count * 2) + 1), NULL);
	unpacked_headers[count * 2] = NULL;

	const char* row_start = headers;
	const char* row_end = CF_STRCHR(row_start, '\n');
	for (size_t i = 0; row_end; i += 2) {
		const char* split = CF_STRCHR(row_start, ':');
		size_t key_size = (size_t)split - (size_t)row_start;
		char* key = (char*)CF_ALLOC(key_size + 1, NULL);
		CF_STRNCPY(key, row_start, key_size);
		key[key_size] = '\0';

		size_t content_size = (size_t)row_end - (size_t)split - 1;
		char* value = (char*)CF_ALLOC(content_size + 1, NULL);
		CF_STRNCPY(value, split + 1, content_size);
		value[content_size] = '\0';

		unpacked_headers[i] = CF_STRDUP(s_trim(key));
		unpacked_headers[i + 1] = CF_STRDUP(s_trim(value));
		CF_FREE(key, NULL);
		CF_FREE(value, NULL);

		row_start = row_end + 1;
		row_end = CF_STRCHR(row_start, '\n');
	}

	return (const char**)unpacked_headers;
}

static void s_free_unpacked_headers(const char** unpacked_headers)
{
	for (int i = 0; unpacked_headers[i]; ++i)
		CF_FREE((void*)unpacked_headers[i], NULL);
	CF_FREE((void*)unpacked_headers, NULL);
}

EMSCRIPTEN_KEEPALIVE void s_response_headers(void* https_ptr, const char* headers)
{
	CF_Https* https = (CF_Https*)https_ptr;

	const char** unpacked_headers = s_unpack_headers(headers);
	https->unpacked_headers = unpacked_headers;

	for (int i = 0; unpacked_headers[i]; i += 2) {
		const char* header_name = unpacked_headers[i];
		const char* header_content = unpacked_headers[i + 1];
		CF_HttpsHeader header;
		header.name.ptr = header_name;
		header.name.len = CF_STRLEN(header_name);
		header.content.ptr = header_content;
		header.content.len = CF_STRLEN(header_content);
		https->response.headers.add(header);
	}
}

EMSCRIPTEN_KEEPALIVE void s_content(void* https_ptr, void* data, size_t size)
{
	CF_Https* https = (CF_Https*)https_ptr;
	((char*)data)[size - 1] = 0;
	https->response.content = (const char*)data;
	https->response.content_len = size - 1;
}

EMSCRIPTEN_KEEPALIVE void s_loaded(void* https_ptr)
{
	CF_Https* https = (CF_Https*)https_ptr;
	https->state = CF_HTTPS_STATE_COMPLETED;
}

EMSCRIPTEN_KEEPALIVE void s_error(void* https_ptr)
{
	CF_Https* https = (CF_Https*)https_ptr;
	https->state = CF_HTTPS_STATE_FAILED;
}

}

EM_JS(void, s_js_get, (void* https_ptr, const char* c_host, const char* c_uri),
{
	var host = UTF8ToString(c_host);
	var uri = UTF8ToString(c_uri);
	var url = host + uri;
	var request = new XMLHttpRequest();

	function progress_fn(e) {
		_s_loaded(https_ptr, e.loaded);
	}

	function load_fn(e) {
		_s_response_code(https_ptr, request.status);
		var headers = request.getAllResponseHeaders();
		var c_headers = allocate(intArrayFromString(headers), ALLOC_NORMAL);
		_s_response_headers(https_ptr, c_headers);
		var u8_array = new Uint8Array(request.response);
		var c_buffer = Module._malloc(u8_array.length);
		Module.HEAPU8.set(u8_array, c_buffer);
		_s_content(https_ptr, c_buffer, u8_array.length);
		_s_loaded(https_ptr);
	}

	function error_fn(e) {
		_s_error(https_ptr);
	}

	request.addEventListener("progress", progress_fn);
	request.addEventListener("load", load_fn);
	request.addEventListener("error", error_fn);
	request.open("GET", url);
	request.responseType = "arraybuffer";
	request.send();
});

EM_JS(void, s_js_post, (void* https_ptr, const char* c_host, const char* c_uri, const void* data, size_t size),
{
	var host = UTF8ToString(c_host);
	var uri = UTF8ToString(c_uri);
	var url = host + uri;
	var request = new XMLHttpRequest();
	request.setRequestHeader("Host", host);

	function progress_fn(e) {
		_s_loaded(https_ptr, e.loaded);
	}

	function load_fn(e) {
		_s_response_code(https_ptr, request.status);
		var headers = request.getAllResponseHeaders();
		var c_headers = allocate(intArrayFromString(headers), ALLOC_NORMAL);
		_s_response_headers(https_ptr, c_headers);
		var u8_array = new Uint8Array(request.response);
		var c_buffer = Module._malloc(u8_array.length);
		Module.HEAPU8.set(u8_array, c_buffer);
		_s_content(https_ptr, c_buffer, u8_array.length);
		_s_loaded(https_ptr);
	}

	function error_fn(e) {
		_s_error(https_ptr);
	}

	var data_array = (data && size) ? HEAPU8.slice(data, data + size) : null;

	request.addEventListener("progress", progress_fn);
	request.addEventListener("load", load_fn);
	request.addEventListener("error", error_fn);
	request.open("POST", url);
	request.responseType = "arraybuffer";
	request.send(data_array);
});

CF_Https* cf_https_get(const char* host, const char* port, const char* uri, CF_Result* err, bool verify_cert)
{
	CF_Https* https = CF_NEW(CF_Https, NULL);
	https->host = host;
	https->uri = uri;
	// `port` and `verify_cert` are not used with emscripten.
	if (err) *err = cf_result_success();
	return https;
}

CF_Https* cf_https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, CF_Result* err, bool verify_cert)
{
	CF_Https* https = CF_NEW(CF_Https, NULL);
	https->host = host;
	https->uri = uri;
	https->data = data;
	https->size = size;
	// `port` and `verify_cert` are not used with emscripten.
	if (err) *err = cf_result_success();
	return https;
}

void cf_https_destroy(CF_Https* https)
{
	s_free_unpacked_headers(https->unpacked_headers);
	free((void*)https->response.content);
	https->~CF_Https();
	CF_FREE(https, NULL);
}

CF_HttpsState cf_https_state(CF_Https* https)
{
	return https->state;
}

size_t cf_https_process(CF_Https* https)
{
	if (https->state != CF_HTTPS_STATE_PENDING) return https->bytes_read;

	if (!https->request_sent) {
		if (https->data) {
			s_js_post(https, https->host, https->uri, https->data, https->size);
		} else {
			s_js_get(https, https->host, https->uri);
		}
		https->request_sent = true;
	}

	return https->bytes_read;
}

bool cf_internal_https_response(CF_Https* https, cf_internal_https_response_t** response_out)
{
	if (https->state != CF_HTTPS_STATE_COMPLETED) return false;

	*response_out = &https->response;

	return true;
}

CF_HttpsResponse cf_https_response(CF_Https* https)
{
	cf_https_response response_out = { 0 };

	cf_internal_https_response_t* response_internal;
	if (!cf_internal_https_response(https, &response_internal)) return response_out;

	response_out.code = response_internal->code;
	response_out.content_len = response_internal->content_len;
	response_out.content = response_internal->content;
	response_out.headers = response_internal->headers.data();
	response_out.headers_count = response_internal->headers.count();
	response_out.transfer_encoding_flags = response_internal->transfer_encoding_flags;

	return true;
}

#endif // CF_EMSCRIPTEN

namespace Cute
{

}
