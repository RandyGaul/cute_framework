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

#include <cute_https.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_array.h>

#include <mbedtls/build_info.h>
#include <mbedtls/platform.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>

#include <SDL.h>

#if defined(CUTE_WINDOWS)
#	include <windows.h>
#	include <wincrypt.h>
#	pragma comment(lib, "crypt32.lib")
#elif defined(CUTE_MACOSX)
#	include <Security/Security.h>
#endif

namespace cute
{
#ifndef CUTE_EMSCRIPTEN

struct https_decoder_t;

typedef bool (https_decode_fn)(https_decoder_t* h, const char* data, size_t size, size_t* bytes_read);
typedef bool (https_process_line_fn)(https_decoder_t* h);

struct https_decoder_t
{
	https_decode_fn* decode = NULL;
	https_process_line_fn* process_line = NULL;
	int transfer_encoding = 0;
	size_t content_processed = 0;
	size_t content_length = 0;
	size_t chunk_processed = 0;
	size_t chunk_size = 0;
	bool found_last_chunk = false;
	size_t buffer_offset = 0;
	int response_code = 0;
	array<char> buffer;
	array<https_header_t> headers;
	error_t err = error_success();

	void next(https_process_line_fn* process_line)
	{
		this->process_line = process_line;
		buffer.clear();
		buffer_offset = 0;
	}

	char* data()
	{
		return buffer.data() + buffer_offset;
	}

	size_t data_left()
	{
		return buffer.count() - buffer_offset;
	}

	bool advance(size_t size)
	{
		CUTE_ASSERT(buffer_offset + size <= buffer.count());
		buffer_offset += size;
		return buffer_offset == buffer.count();
	}
};

struct https_t
{
	mbedtls_net_context server_fd;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;

	const char* host = NULL;
	const char* port = NULL;
	https_state_t state = HTTPS_STATE_PENDING; // TODO - Atomic this.
	array<char> request;
	array<char> response_buffer;
	https_response_t response;

	bool request_sent = false;

	int bytes_read = 0; // TODO - Atomic this.
	https_decoder_t h;
};

https_t* https_make()
{
	https_t* https = CUTE_NEW(https_t, NULL);

	mbedtls_net_init(&https->server_fd);
	mbedtls_ssl_init(&https->ssl);
	mbedtls_ssl_config_init(&https->conf);
	mbedtls_x509_crt_init(&https->cacert);
	mbedtls_ctr_drbg_init(&https->ctr_drbg);
	mbedtls_entropy_init(&https->entropy);

	const char* seed = "Cute Framework";

	if (mbedtls_ctr_drbg_seed(&https->ctr_drbg, mbedtls_entropy_func, &https->entropy, (const unsigned char*)seed, CUTE_STRLEN(seed))) {
		https->~https_t();
		CUTE_FREE(https, NULL);
		return NULL;
	}

	return https;
}

void https_destroy(https_t* https)
{
	mbedtls_net_free(&https->server_fd);
	mbedtls_x509_crt_free(&https->cacert);
	mbedtls_ssl_free(&https->ssl);
	mbedtls_ssl_config_free(&https->conf);
	mbedtls_ctr_drbg_free(&https->ctr_drbg);
	mbedtls_entropy_free(&https->entropy);
	https->~https_t();
	CUTE_FREE(https, NULL);
}

static void s_tls_log(void* param, int debug_level, const char* file_name, int line_number, const char*  message)
{
	printf("%s\n", message);
}

static error_t s_load_platform_certs(https_t* https)
{
#if defined(CUTE_WINDOWS)

	HCERTSTORE hCertStore;
	PCCERT_CONTEXT pCertContext = NULL;

	if (!(hCertStore = CertOpenSystemStoreA((HCRYPTPROV)NULL, "ROOT"))) {
		return error_failure("CertOpenSystemStoreA failed.");
	}

	while (pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext)) {
		mbedtls_x509_crt_parse_der(&https->cacert, (unsigned char*)pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
	}

	CertFreeCertificateContext(pCertContext);
	CertCloseStore(hCertStore, 0);

#elif defined(CUTE_MACOSX)

	SecKeychainRef keychain_ref;
	CFMutableDictionaryRef search_settings_ref;
	CFArrayRef result_ref;

	if (SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain", &keychain_ref) != errSecSuccess) {
		return error_failure("SecKeychainOpen failed.");
	}

	search_settings_ref = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
	CFDictionarySetValue(search_settings_ref, kSecClass, kSecClassCertificate);
	CFDictionarySetValue(search_settings_ref, kSecMatchLimit, kSecMatchLimitAll);
	CFDictionarySetValue(search_settings_ref, kSecReturnRef, kCFBooleanTrue);
	CFDictionarySetValue(search_settings_ref, kSecMatchSearchList, CFArrayCreate(NULL, (const void **)&keychain_ref, 1, NULL));

	if (SecItemCopyMatching(search_settings_ref, (CFTypeRef*)&result_ref) != errSecSuccess) {
		return error_failure("SecItemCopyMatching failed.");
	}

	for (CFIndex i = 0; i < CFArrayGetCount(result_ref); i++) {
		SecCertificateRef item_ref = (SecCertificateRef) CFArrayGetValueAtIndex(result_ref, i);
		CFDataRef data_ref;

		if ((data_ref = SecCertificateCopyData(item_ref))) {
			mbedtls_x509_crt_parse_der(&https->cacert, (unsigned char*)CFDataGetBytePtr(data_ref), CFDataGetLength(data_ref));
			CFRelease(data_ref);
		}
	}

	CFRelease(keychain_ref);

#elif defined(CUTE_LINUX)

	if (mbedtls_x509_crt_parse_path(&https->cacert, "/etc/ssl/certs/") < 0) {
		return error_failure("mbedtls_x509_crt_parse_path failed.");
	}

#else

	// TODO - Android, iOS (probably the same as CUTE_MACOSX section).

#	error Platform not yet supported for https.

#endif

	return error_success();
}

error_t https_connect(https_t* https, const char* host, const char* port, bool verify_cert)
{
	int result;

	result = MBEDTLS_ERR_NET_UNKNOWN_HOST;
	if ((result = mbedtls_net_connect(&https->server_fd, host, port, MBEDTLS_NET_PROTO_TCP))) {
		return error_failure("Failed to connect TCP socket to host with mbedtls_net_connect.");
	}

	if ((result = mbedtls_net_set_nonblock(&https->server_fd))) {
		return error_failure("Failed to set socket to non-blocking.");
	}

	if ((result = mbedtls_ssl_config_defaults(&https->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT))) {
		return error_failure("Failed to set ssl defaults with mbedtls_ssl_config_defaults.");
	}

	mbedtls_ssl_conf_authmode(&https->conf, verify_cert ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_OPTIONAL);
	s_load_platform_certs(https);
	mbedtls_ssl_conf_ca_chain(&https->conf, &https->cacert, NULL);
	mbedtls_ssl_conf_rng(&https->conf, mbedtls_ctr_drbg_random, &https->ctr_drbg);

	if ((result = mbedtls_ssl_setup(&https->ssl, &https->conf))) {
		return error_failure("Failed to setup ssl context with mbedtls_ssl_setup.");
	}

	if ((result = mbedtls_ssl_set_hostname(&https->ssl, host))) {
		return error_failure("mbedtls_ssl_set_hostname failed.");
	}

	mbedtls_ssl_set_bio(&https->ssl, &https->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	while ((result = mbedtls_ssl_handshake(&https->ssl))) {
		if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE) {
			char buf[1024];
			mbedtls_strerror(result, buf, 1024);
			//const char* high = mbedtls_high_level_strerr(result);
			//const char* low = mbedtls_low_level_strerr(result);
			return error_failure("TLS handshake failed with mbedtls_ssl_handshake.");
		}
	}

	if (verify_cert) {
		uint32_t flags;
		if ((flags = mbedtls_ssl_get_verify_result(&https->ssl))) {
			return error_failure("Failed to verify certs via mbedtls_ssl_get_verify_result -- unsafe to connect.");
		}
	}

	https->host = host;
	https->port = port;
	https->state = HTTPS_STATE_PENDING;
	return error_success();
}

void https_disconnect(https_t* https)
{
	mbedtls_ssl_close_notify(&https->ssl);
}

https_t* https_get(const char* host, const char* port, const char* uri, error_t* err_out, bool verify_cert)
{
	https_t* https = https_make();
	error_t err = https_connect(https, host, port, verify_cert);
	if (err.is_error()) {
		https_destroy(https);
		if (err_out) *err_out = err;
		return NULL;
	}

	const char* fmt =
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"\r\n";
	size_t len = 64 + CUTE_STRLEN(uri) + CUTE_STRLEN(host);
	https->request.ensure_count((int)len);
	sprintf(https->request.data(), fmt, uri, https->host);
	https->request_sent = false;
	if (err_out) *err_out = error_success();

	return https;
}

https_t* https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, error_t* err_out, bool verify_cert)
{
	https_t* https = https_make();
	error_t err = https_connect(https, host, port, verify_cert);
	if (err.is_error()) {
		https_destroy(https);
		if (err_out) *err_out = err;
		return NULL;
	}

	const char* fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Length: %zu\r\n"
		"\r\n";
	size_t len = 64 + CUTE_STRLEN(uri) + CUTE_STRLEN(host);
	https->request.ensure_count((int)len);
	sprintf(https->request.data(), fmt, uri, https->host, size);
	https->request_sent = false;
	len = CUTE_STRLEN(https->request.data());
	https->request.ensure_count((int)(len + size));
	CUTE_MEMCPY(https->request.data() + len, data, size);
	if (err_out) *err_out = error_success();

	return https;
}

const https_response_t* https_response(https_t* https)
{
	if (https->state != HTTPS_STATE_COMPLETED) return NULL;
	return &https->response;
}

static bool s_crlf(https_decoder_t* h, const char* data, size_t size, size_t* bytes_read)
{
	const char* in = data;
	const char* end = data + size;
	while (in != end) {
		const char* found_lf = (const char*)CUTE_MEMCHR(in, '\n', end - in);
		if (!found_lf) {
			break;
		}

		char prev_char = 0;
		bool was_first_character_in_line = found_lf == data;
		if (was_first_character_in_line) {
			if (h->buffer.count()) {
				prev_char = h->buffer.last();
			}
		} else {
			prev_char = *(found_lf - 1);
		}

		if (prev_char == '\r') {
			*bytes_read = 1 + (found_lf - data);
			return true;
		}

		in = found_lf + 1;
	}

	*bytes_read = size;
	return false;
}

static bool s_chunk_size(https_decoder_t* h);
static bool s_header(https_decoder_t* h);
static bool s_no_chunks(https_decoder_t* h);

static bool s_get_line(https_decoder_t* h, const char* data, size_t size, size_t* bytes_read)
{
	bool found_crlf = s_crlf(h, data, size, bytes_read);

	int old_count = h->buffer.count();
	h->buffer.ensure_count((int)(h->buffer.count() + *bytes_read));
	void* buffer_data = h->buffer.data() + old_count;
	CUTE_MEMCPY(buffer_data, data, *bytes_read);

	if (found_crlf || h->process_line == s_no_chunks) {
		return h->process_line(h);
	}

	return false;
}

static bool s_no_chunks(https_decoder_t* h)
{
	size_t bytes_read = 0;
	CUTE_ASSERT(h->content_processed < h->content_length);
	h->content_processed = h->buffer.count();
	bool finished = h->content_processed == h->content_length;
	return finished;
}

static bool s_state_chunk(https_decoder_t* h) {
	// RFC-7230 section 4.1 Chunked Transfer Encoding
	size_t bytes_read = 0;
	CUTE_ASSERT(h->chunk_processed < h->chunk_size);
	h->chunk_processed = h->buffer.count() - 2;
	bool finished = h->chunk_processed == h->chunk_size;
	if (finished) {
		h->chunk_processed = 0;
		h->next(s_chunk_size);
	}
	return false;
}

static bool s_int(https_string_t str, int radix, uint64_t* out)
{
	char* ptr;
	uint64_t number = CUTE_STRTOLL(str.ptr, &ptr, radix);
	*out = number;
	return str.ptr == ptr;
}

static bool s_read_int(https_decoder_t* h, int radix, uint64_t* out)
{
	char* ptr;
	uint64_t number = CUTE_STRTOLL(h->data(), &ptr, radix);
	*out = number;
	if (ptr == h->data()) {
		h->err = error_failure("Failed to parse number.");
		return true;
	} else {
		h->advance(ptr - h->data());
	}
	return false;
}

static bool s_chunk_size(https_decoder_t* h) {
	if (s_read_int(h, 16, (uint64_t*)&h->chunk_size)) {
		return true;
	}

	bool was_last_chunk = h->chunk_size == 0;
	if (was_last_chunk) {
		h->found_last_chunk = true;
		h->next(s_header);
		return false;
	}

	// RFC-7230 section 4.1.1 Chunk Extensions -- Skip (optional).

	h->next(s_state_chunk);
	return false;
}

static https_string_t s_scan(https_decoder_t* h, char delimiter, bool must_find = true)
{
	const char* data = h->data();
	size_t len = h->data_left();
	https_string_t string;
	string.ptr = data;
	string.len = 0;
	while (len && *data != delimiter) {
		++data;
		--len;
		string.len++;
	}

	if (len && *data == delimiter) {
		h->advance(string.len + 1);
	} else if (len) {
		h->advance(string.len);
	} else if (must_find) {
		h->err = error_failure("Malformed header found while searching for colon separator ':'");
		string.ptr = NULL;
		string.len = 0;
	}

	// Trim whitespace.
	while (string.len && *string.ptr == ' ') {
		string.ptr++;
		string.len--;
	}
	while (string.len && string.ptr[string.len - 1] == ' ') {
		string.len--;
	}

	return string;
}

static bool s_header(https_decoder_t* h)
{
	// RFC-7230 section 3 Message Format
	if (h->data_left() == 2) {
		if (h->transfer_encoding & TRANSFER_ENCODING_CHUNKED) {
			if (h->found_last_chunk) {
				// TODO - Possible to start logic for trailers here.
				return true;
			}
			h->next(s_chunk_size);
		} else if (h->content_length > 0) {
			h->next(s_no_chunks);
		} else {
			return true;
		}

		return false;
	}

	// RFC-7230 3.2
	https_string_t name = s_scan(h, ':');
	if (!name.ptr) return true;
	https_string_t content = s_scan(h, '\r');
	if (!content.ptr) return true;

	if (!https_strcmp("Content-Length", name)) {
		if (h->transfer_encoding) {
			h->err = error_failure("Found illegal combo of headers for both content-length and transfer-encoding.");
			return true;
		}

		if (s_int(content, 10, (uint64_t*)&h->content_length)) {
			h->err = error_failure("Failed to read content length.");
			return true;
		}
	} else if (!https_strcmp("Transfer-Encoding", name)) {
		if (h->content_length) {
			h->err = error_failure("Found illegal combo of headers for both content-length and transfer-encoding.");
			return true;
		}

		// RFC-7230 section 3.3.1 Transfer-Encoding
		// RFC-7230 section 4.2 Compression Codings
		https_string_t string = content;
		while (string.ptr) {
			int prev_flags = h->transfer_encoding;

			if (!https_strcmp("chunked", string)) {
				h->transfer_encoding |= TRANSFER_ENCODING_CHUNKED;
			} else if (!https_strcmp("compress", string) || !https_strcmp("x_compress", string)) {
				h->transfer_encoding |= TRANSFER_ENCODING_DEPRECATED_COMPRESS;
			} else if (!https_strcmp("deflate", string)) {
				h->transfer_encoding |= TRANSFER_ENCODING_DEFLATE;
			} else if (!https_strcmp("gzip", string) || !https_strcmp("x_gzip", string)) {
				h->transfer_encoding |= TRANSFER_ENCODING_GZIP;
			} else if (string.len > 0) {
				h->err = error_failure("Unrecognized transfer encoding encountered.");
				return true;
			}

			// RFC-7230 3.3.1
			if ((prev_flags & TRANSFER_ENCODING_CHUNKED) && (h->transfer_encoding != prev_flags)) {
				h->err = error_failure("Invalid transfer encoding order found (chunked must be last).");
				return true;
			}

			string = s_scan(h, ',');
			if (!string.ptr) break;
		}
	}

	https_header_t header;
	header.name = name;
	header.content = content;
	h->headers.add(header);

	h->next(s_header);
	return false;
}

static bool s_request(https_decoder_t* h)
{
	https_string_t method = s_scan(h, ' ');
	if (!method.ptr) return true;
	https_string_t uri = s_scan(h, ' ');
	if (!uri.ptr) return true;
	https_string_t version = s_scan(h, ' ', false);
	if (!version.ptr) return true;

	if (!https_strcmp("HTTP/1.1", version)) {
		h->err = error_failure("Expected HTTP/1.1");
		return true;
	}

	h->next(s_header);
	return false;
}

static bool s_response(https_decoder_t* h) {
	https_string_t version = s_scan(h, ' ');
	if (!version.ptr) return true;
	https_string_t code = s_scan(h, ' ');
	if (!code.ptr) return true;
	https_string_t phrase = s_scan(h, ' ', false);
	if (!phrase.ptr) return true;

	if (https_strcmp("HTTP/1.1", version)) {
		h->err = error_failure("Expected HTTP/1.1");
		return true;
	}

	// RFC7230 section 3.1.2
	uint64_t code_number;
	if (s_int(code, 10, &code_number) || code.len != 3 || code_number > 999) {
		h->err = error_failure("Bad response code.");
		return true;
	}
	h->response_code = (int)code_number;

	h->next(s_header);
	return false;
}

static bool s_decode(https_decoder_t* h, const char* data, size_t size)
{
	bool done = false;

	while (size) {
		size_t bytes_read = 0;
		done = h->decode(h, data, size, &bytes_read);
		if (done) break;
		data += bytes_read;
		size -= bytes_read;
	}

	return done;
}

https_state_t https_state(https_t* https)
{
	return https->state;
}

size_t https_process(https_t* https)
{
	if (https->state != HTTPS_STATE_PENDING) return https->bytes_read;

	if (!https->request_sent) {
		const char* request = https->request.data();
		int result;
		while ((result = mbedtls_ssl_write(&https->ssl, (uint8_t*)request, CUTE_STRLEN(request))) <= 0) {
			if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE) {
				https->state = HTTPS_STATE_FAILED;
				return https->bytes_read;
			}
		}
		https->request_sent = true;
		https->h.decode = s_get_line;
		https->h.process_line = s_response;
	}
;
	https->response_buffer.ensure_count(https->bytes_read + 1024 * 10);

	int result;
	const char* data = https->response_buffer.data() + https->bytes_read;
	result = mbedtls_ssl_read(&https->ssl, (uint8_t*)(https->response_buffer.data() + https->bytes_read), https->response_buffer.count() - https->bytes_read - 1);

	const char* err = mbedtls_high_level_strerr(result);

	if (result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE) return https->bytes_read;
	else if (result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) return https->bytes_read;
	else if (result <= 0) return https->bytes_read;

	https->bytes_read += result;
	bool done = s_decode(&https->h, data, result);
	if (!done) return https->bytes_read;

	https->h.buffer.add(0);
	https->response.code = https->h.response_code;
	https->response.content = https->h.buffer.data();
	https->response.headers.steal_from(https->h.headers);
	https->response.content_len = https->h.buffer.count() - 1;
	https->response.transfer_encoding_flags = https->h.transfer_encoding;
	https->state = HTTPS_STATE_COMPLETED;
	return https->bytes_read;
}

#else // CUTE_EMSCRIPTEN

struct https_t
{
	const char* host = NULL;
	const char* uri = NULL;
	https_state_t state = HTTPS_STATE_PENDING; // TODO - Atomic this.
	const void* data = NULL;
	size_t size = 0;
	const char** unpacked_headers = NULL;
	int response_code = 0;
	https_response_t response;
	bool request_sent = false;
	int bytes_read = 0; // TODO - Atomic this.
};

extern "C" {

EMSCRIPTEN_KEEPALIVE void s_bytes_downloaded(void* https_ptr, size_t size)
{
	https_t* https = (https_t*)https_ptr;
	https->bytes_read = size;
}

EMSCRIPTEN_KEEPALIVE void s_response_code(void* https_ptr, int code)
{
	https_t* https = (https_t*)https_ptr;
	https->response.code = code;
}

static char* s_trim(char* s)
{
	size_t i = CUTE_STRLEN(s);
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
	for (const char* pos = CUTE_STRCHR(headers, '\n'); pos; pos = CUTE_STRCHR(pos + 1, '\n')) {
		count++;
	}

	char **unpacked_headers = (char**)CUTE_ALLOC(sizeof(char*) * ((count * 2) + 1), NULL);
	unpacked_headers[count * 2] = NULL;

	const char* row_start = headers;
	const char* row_end = CUTE_STRCHR(row_start, '\n');
	for (size_t i = 0; row_end; i += 2) {
		const char* split = CUTE_STRCHR(row_start, ':');
		size_t key_size = (size_t)split - (size_t)row_start;
		char* key = (char*)CUTE_ALLOC(key_size + 1, NULL);
		CUTE_STRNCPY(key, row_start, key_size);
		key[key_size] = '\0';

		size_t content_size = (size_t)row_end - (size_t)split - 1;
		char* value = (char*)CUTE_ALLOC(content_size + 1, NULL);
		CUTE_STRNCPY(value, split + 1, content_size);
		value[content_size] = '\0';

		unpacked_headers[i] = CUTE_STRDUP(s_trim(key));
		unpacked_headers[i + 1] = CUTE_STRDUP(s_trim(value));
		CUTE_FREE(key, NULL);
		CUTE_FREE(value, NULL);

		row_start = row_end + 1;
		row_end = CUTE_STRCHR(row_start, '\n');
	}

	return (const char**)unpacked_headers;
}

static void s_free_unpacked_headers(const char** unpacked_headers)
{
	for (int i = 0; unpacked_headers[i]; ++i)
		CUTE_FREE((void*)unpacked_headers[i], NULL);
	CUTE_FREE((void*)unpacked_headers, NULL);
}

EMSCRIPTEN_KEEPALIVE void s_response_headers(void* https_ptr, const char* headers)
{
	https_t* https = (https_t*)https_ptr;

	const char** unpacked_headers = s_unpack_headers(headers);
	https->unpacked_headers = unpacked_headers;

	for (int i = 0; unpacked_headers[i]; i += 2) {
		const char* header_name = unpacked_headers[i];
		const char* header_content = unpacked_headers[i + 1];
		https_header_t header;
		header.name.ptr = header_name;
		header.name.len = CUTE_STRLEN(header_name);
		header.content.ptr = header_content;
		header.content.len = CUTE_STRLEN(header_content);
		https->response.headers.add(header);
	}
}

EMSCRIPTEN_KEEPALIVE void s_content(void* https_ptr, void* data, size_t size)
{
	https_t* https = (https_t*)https_ptr;
	((char*)data)[size - 1] = 0;
	https->response.content = (const char*)data;
	https->response.content_len = size - 1;
}

EMSCRIPTEN_KEEPALIVE void s_loaded(void* https_ptr)
{
	https_t* https = (https_t*)https_ptr;
	https->state = HTTPS_STATE_COMPLETED;
}

EMSCRIPTEN_KEEPALIVE void s_error(void* https_ptr)
{
	https_t* https = (https_t*)https_ptr;
	https->state = HTTPS_STATE_FAILED;
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

https_t* https_get(const char* host, const char* port, const char* uri, error_t* err, bool verify_cert)
{
	https_t* https = CUTE_NEW(https_t, NULL);
	https->host = host;
	https->uri = uri;
	// `port` and `verify_cert` are not used with emscripten.
	if (err) *err = error_success();
	return https;
}

https_t* https_post(const char* host, const char* port, const char* uri, const void* data, size_t size, error_t* err, bool verify_cert)
{
	https_t* https = CUTE_NEW(https_t, NULL);
	https->host = host;
	https->uri = uri;
	https->data = data;
	https->size = size;
	// `port` and `verify_cert` are not used with emscripten.
	if (err) *err = error_success();
	return https;
}

void https_destroy(https_t* https)
{
	s_free_unpacked_headers(https->unpacked_headers);
	free((void*)https->response.content);
	https->~https_t();
	CUTE_FREE(https, NULL);
}

https_state_t https_state(https_t* https)
{
	return https->state;
}

size_t https_process(https_t* https)
{
	if (https->state != HTTPS_STATE_PENDING) return https->bytes_read;

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

const https_response_t* https_response(https_t* https)
{
	if (https->state != HTTPS_STATE_COMPLETED) return NULL;
	return &https->response;
}

#endif // CUTE_EMSCRIPTEN

}
