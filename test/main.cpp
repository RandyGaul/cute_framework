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

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <cute_c_runtime.h>
#include <test_harness.h>
#include <internal/cute_crypto_internal.h>

#include <test_handle.h>
#include <test_circular_buffer.h>
#include <test_replay_buffer.h>
#include <test_crypto.h>
#include <test_socket.h>
#include <test_connect_token.h>
#include <test_packets.h>
#include <test_hashtable.h>
#include <test_encryption_map.h>
#include <test_doubly_list.h>
#include <test_connect_token_cache.h>
#include <test_client_server.h>

int main(int argc, const char** argv)
{
	//protocol::log_set_level(CUTE_PROTOCOL_LOG_LEVEL_INFORMATIONAL);

#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	windows_turn_on_console_color();
#endif

	internal::net_init();
	internal::crypto_init();

	test_t tests[] = {
		CUTE_TEST_CASE_ENTRY(test_handle_basic),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop),
		CUTE_TEST_CASE_ENTRY(test_handle_large_loop_and_free),
		CUTE_TEST_CASE_ENTRY(test_handle_alloc_too_many),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_basic),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_fill_up_and_empty),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_overflow),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_underflow),
		CUTE_TEST_CASE_ENTRY(test_circular_buffer_two_threads),
		CUTE_TEST_CASE_ENTRY(test_replay_buffer_valid_packets),
		CUTE_TEST_CASE_ENTRY(test_replay_buffer_old_packet_out_of_range),
		CUTE_TEST_CASE_ENTRY(test_replay_buffer_duplicate),
		CUTE_TEST_CASE_ENTRY(test_crypto_encrypt_decrypt),
		CUTE_TEST_CASE_ENTRY(test_crypto_encrypt_decrypt_associated_data),
		CUTE_TEST_CASE_ENTRY(test_crypto_encrypt_decrypt_big_nonce),
		CUTE_TEST_CASE_ENTRY(test_crypto_encrypt_decrypt_associated_data_big_nonce),
		CUTE_TEST_CASE_ENTRY(test_socket_init_send_recieve_shutdown),
		CUTE_TEST_CASE_ENTRY(test_generate_connect_token),
		CUTE_TEST_CASE_ENTRY(test_packet_connection_accepted),
		CUTE_TEST_CASE_ENTRY(test_packet_connection_denied),
		CUTE_TEST_CASE_ENTRY(test_packet_keepalive),
		CUTE_TEST_CASE_ENTRY(test_packet_disconnect),
		CUTE_TEST_CASE_ENTRY(test_packet_challenge),
		CUTE_TEST_CASE_ENTRY(test_packet_payload),
		CUTE_TEST_CASE_ENTRY(test_hash_table_basic),
		CUTE_TEST_CASE_ENTRY(test_hash_table_hammer),
		CUTE_TEST_CASE_ENTRY(test_hash_table_set),
		CUTE_TEST_CASE_ENTRY(test_encryption_map_basic),
		CUTE_TEST_CASE_ENTRY(test_encryption_map_timeout_and_expiration),
		CUTE_TEST_CASE_ENTRY(test_doubly_list),
		CUTE_TEST_CASE_ENTRY(test_connect_token_cache),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_server),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_no_server_responses),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_server_list),
		CUTE_TEST_CASE_ENTRY(test_protocol_server_challenge_response_timeout),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_expired_token),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_connect_expired_token),
		CUTE_TEST_CASE_ENTRY(test_protocol_server_connect_expired_token),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_bad_keys),
		CUTE_TEST_CASE_ENTRY(test_protocol_server_not_in_list_but_gets_request),
		CUTE_TEST_CASE_ENTRY(test_protocol_connect_a_few_clients),
		CUTE_TEST_CASE_ENTRY(test_protocol_keepalive),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_initiated_disconnect),
		CUTE_TEST_CASE_ENTRY(test_protocol_client_server_payloads),
		CUTE_TEST_CASE_ENTRY(test_protocol_multiple_connections_and_payloads),
	};
	int test_count = sizeof(tests) / sizeof(*tests);
	int fail_count = 0;

	// Run all tests.
	if (argc == 1) {
		for (int i = 0; i < test_count; ++i)
		{
			test_t* test = tests + i;
			if (do_test(test, i + 1)) fail_count++;
		}
		if (fail_count) {
			fprintf(CUTE_TEST_IO_STREAM, "\033[31mFAILED\033[0m %d test case%s.\n\n", fail_count, fail_count > 1 ? "s" : "");
		} else {
			fprintf(CUTE_TEST_IO_STREAM, "All tests \033[32mPASSED\033[0m.\n\n");
		}
	} else if (argc == 2) {
		const char* soak = argv[1];
		if (CUTE_STRCMP(soak, "soak") == 0) {
			while (1)
			{
				for (int i = 0; i < test_count; ++i)
				{
					test_t* test = tests + i;
					int result = do_test(test, i + 1);
					if (result) goto break_soak;
				}
			}
		}

		// Run a specific test function.
		const char* test_name = argv[1];
		int found = 0;
		for (int i = 0; i < test_count; ++i)
		{
			test_t* test = tests + i;
			if (CUTE_STRCMP(test_name, test->test_name) == 0) {
				do_test(test, 1);
				found = 1;
				break;
			}
		}

		if (!found) {
			fprintf(CUTE_TEST_IO_STREAM, "Unable to find test %s.\n", test_name);
			return -1;
		}
	} else {
		fprintf(CUTE_TEST_IO_STREAM, "Invalid number of parameters. Please pass in either no parameters to run all tests, or just the test function name.\nYou may also pass in \"soak\" to run tests in an infinite loop.\n");
		return -1;
	}

	internal::net_cleanup();

break_soak:
	return 0;
}
