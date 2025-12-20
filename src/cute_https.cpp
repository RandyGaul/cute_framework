/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_https.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_array.h>
#include <cute_string.h>
#include <cute_coroutine.h>

#ifndef CF_EMSCRIPTEN

#include <internal/cute_alloc_internal.h>

#include <SDL3/SDL.h>

#ifndef CF_APPLE
#	define CUTE_TLS_IMPLEMENTATION
	// .m file is used to compile this on apple.
#	include <cute/cute_tls.h>
#else
extern "C" {
#	include <cute/cute_tls.h>
}
#endif

// Credit to Mattias Gustavsson for the original API design of cute_https.h

using namespace Cute;

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

static void s_get_line(CF_Coroutine co, CF_Response* response)
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

static bool s_status(CF_Coroutine co, CF_Response* response)
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

static bool s_header(CF_Coroutine co, CF_Response* response)
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

static void s_headers(CF_Coroutine co, CF_Response* response)
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

static void s_decode(CF_Coroutine co)
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
	}
}

static void s_https_process(CF_Coroutine co)
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
	s = String::fmt(
		"%s %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"TE: trailers, deflate, gzip\r\n",
		request->content ? "POST" : "GET", request->uri, request->host
	);
	for (int i = 0; i < request->headers.count(); ++i) {
		s.fmt_append("%s: %s\r\n", request->headers[i].name, request->headers[i].value);
	}

	if (request->content) {
		s.fmt_append("Content-Length: %d\r\n", request->content_length);
	}

	s.append("\r\n");

	if (request->content) {
		const char* content = (const char*)request->content;
		s.append(content, content + request->content_length);
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
			} else if (coroutine_state(decoder) == CF_COROUTINE_STATE_DEAD) {
				break;
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

#endif // CF_EMSCRIPTEN

namespace Cute
{

}
