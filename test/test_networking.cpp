/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute_networking.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_time.h>
#include <time.h>
#include <string.h>

// No UDP on web builds; the suite compiles away to nothing there.
#ifndef CF_EMSCRIPTEN

// Real loopback sockets through the full stack (crypto handshake included), no app required.
// Each test uses its own port so a lingering socket from one can't trip the next.
#define TEST_NET_APP_ID 0xC0FFEEull

struct NetPair
{
	CF_Server* server;
	CF_Client* client;
	int client_index;
};

static void s_net_free(NetPair* np)
{
	if (np->client) { cf_client_disconnect(np->client); cf_destroy_client(np->client); }
	if (np->server) { cf_server_stop(np->server); cf_destroy_server(np->server); }
	np->client = NULL;
	np->server = NULL;
}

// Pump one side of the handshake/update loop. Frees any stray payload events.
static void s_drain_events(NetPair* np)
{
	CF_ServerEvent e;
	while (cf_server_pop_event(np->server, &e)) {
		if (e.type == CF_SERVER_EVENT_TYPE_NEW_CONNECTION) np->client_index = e.u.new_connection.client_index;
		else if (e.type == CF_SERVER_EVENT_TYPE_PAYLOAD_PACKET) cf_server_free_packet(np->server, e.u.payload_packet.client_index, e.u.payload_packet.data);
	}
}

static bool s_net_start_wired(NetPair* np, const char* addr, CF_ClientWire* wire)
{
	memset(np, 0, sizeof(*np));
	np->client_index = -1;
	np->server = cf_make_server_insecure(TEST_NET_APP_ID);
	if (!np->server) return false;
	if (cf_is_error(cf_server_start(np->server, addr))) return false;
	np->client = cf_make_client(0, TEST_NET_APP_ID, false);
	if (!np->client) return false;
	if (wire) cf_client_set_wire(np->client, *wire);
	if (cf_is_error(cf_client_connect_insecure(np->client, addr, TEST_NET_APP_ID))) return false;
	for (int i = 0; i < 5000; ++i) {
		uint64_t now = (uint64_t)time(NULL);
		cf_client_update(np->client, 1.0 / 60.0, now);
		cf_server_update(np->server, 1.0 / 60.0, now);
		s_drain_events(np);
		if (cf_client_state(np->client) == CF_CLIENT_STATE_CONNECTED && np->client_index >= 0) return true;
		cf_sleep(1);
	}
	return false;
}

static bool s_net_start(NetPair* np, const char* addr)
{
	return s_net_start_wired(np, addr, NULL);
}

static void s_net_update(NetPair* np)
{
	uint64_t now = (uint64_t)time(NULL);
	cf_client_update(np->client, 1.0 / 60.0, now);
	cf_server_update(np->server, 1.0 / 60.0, now);
}

/* Raw packets still round-trip (they carry a framing byte now), and messages flow both ways with
 * ids, payloads, ordering, and zero-size messages intact. */
TEST_CASE(test_net_raw_and_msgs)
{
	NetPair np;
	REQUIRE(s_net_start(&np, "127.0.0.1:5801"));

	// Raw client -> server.
	const char raw[] = "raw packet says hi";
	REQUIRE(!cf_is_error(cf_client_send(np.client, raw, sizeof(raw), true)));
	bool got_raw = false;
	for (int i = 0; i < 5000 && !got_raw; ++i) {
		s_net_update(&np);
		CF_ServerEvent e;
		while (cf_server_pop_event(np.server, &e)) {
			if (e.type == CF_SERVER_EVENT_TYPE_PAYLOAD_PACKET) {
				REQUIRE(e.u.payload_packet.size == (int)sizeof(raw));
				REQUIRE(!memcmp(e.u.payload_packet.data, raw, sizeof(raw)));
				cf_server_free_packet(np.server, e.u.payload_packet.client_index, e.u.payload_packet.data);
				got_raw = true;
			}
		}
		cf_sleep(1);
	}
	REQUIRE(got_raw);

	// Raw server -> client.
	const char raw2[] = "server raw";
	REQUIRE(!cf_is_error(cf_server_send(np.server, raw2, sizeof(raw2), np.client_index, true)));
	got_raw = false;
	for (int i = 0; i < 5000 && !got_raw; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		void* pkt;
		int size;
		while (cf_client_pop_packet(np.client, &pkt, &size, NULL)) {
			REQUIRE(size == (int)sizeof(raw2));
			REQUIRE(!memcmp(pkt, raw2, sizeof(raw2)));
			cf_client_free_packet(np.client, pkt);
			got_raw = true;
		}
		cf_sleep(1);
	}
	REQUIRE(got_raw);

	// Messages client -> server on a reliable channel: ids, payloads, FIFO order, and a zero-size
	// message. All coalesce into one bundle.
	cf_client_channel_options(np.client, 0, 0, true);
	const char* words[] = { "alpha", "beta", "gamma" };
	for (uint32_t k = 0; k < 3; ++k) {
		REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 0, 10 + k, words[k], (int)CF_STRLEN(words[k]) + 1)));
	}
	REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 0, 99, NULL, 0))); // An id alone is a message.
	int got = 0;
	bool got_empty = false;
	for (int i = 0; i < 5000 && (got < 3 || !got_empty); ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		int ci;
		uint32_t id;
		void* data;
		int size;
		while (cf_server_pop_msg(np.server, &ci, &id, &data, &size)) {
			REQUIRE(ci == np.client_index);
			if (id == 99) {
				REQUIRE(size == 0);
				got_empty = true;
			} else {
				REQUIRE(id == 10 + (uint32_t)got); // FIFO order per channel.
				REQUIRE(!CF_STRCMP((const char*)data, words[got]));
				got++;
			}
			cf_server_free_msg(np.server, data);
		}
		cf_sleep(1);
	}
	REQUIRE(got == 3);
	REQUIRE(got_empty);

	// Messages server -> client.
	cf_server_channel_options(np.server, 2, 0, true);
	REQUIRE(!cf_is_error(cf_server_send_msg(np.server, np.client_index, 2, 777, "pong", 5)));
	got = 0;
	for (int i = 0; i < 5000 && !got; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		void* pkt;
		int size;
		while (cf_client_pop_packet(np.client, &pkt, &size, NULL)) cf_client_free_packet(np.client, pkt);
		uint32_t id;
		void* data;
		while (cf_client_pop_msg(np.client, &id, &data, &size)) {
			REQUIRE(id == 777);
			REQUIRE(!CF_STRCMP((const char*)data, "pong"));
			cf_client_free_msg(np.client, data);
			got++;
		}
		cf_sleep(1);
	}
	REQUIRE(got == 1);

	s_net_free(&np);
	return true;
}

/* With a tight send budget, a high-priority channel's messages beat previously-queued low-priority
 * ones over the wire, while both channels stay FIFO internally. */
TEST_CASE(test_net_channel_priority)
{
	NetPair np;
	REQUIRE(s_net_start(&np, "127.0.0.1:5802"));

	cf_client_channel_options(np.client, 0, 0, true);  // Low priority "bulk".
	cf_client_channel_options(np.client, 1, 10, true); // High priority "urgent".
	cf_client_set_send_rate(np.client, 500); // Tiny budget so the queues can't drain in one update.

	// Queue ALL the bulk first, then the urgent -- priority must reorder them on the wire.
	char blob[50];
	memset(blob, 0xAB, sizeof(blob));
	for (uint32_t k = 0; k < 10; ++k) REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 0, 200 + k, blob, sizeof(blob))));
	for (uint32_t k = 0; k < 5; ++k) REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 1, 100 + k, blob, sizeof(blob))));

	uint32_t order[15];
	int got = 0;
	for (int i = 0; i < 10000 && got < 15; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		int ci;
		uint32_t id;
		void* data;
		int size;
		while (cf_server_pop_msg(np.server, &ci, &id, &data, &size)) {
			REQUIRE(got < 15);
			order[got++] = id;
			cf_server_free_msg(np.server, data);
		}
		cf_sleep(1);
	}
	REQUIRE(got == 15);
	// Every urgent message arrived before every bulk message, each channel in FIFO order. (Both
	// channels are reliable, so the arrival order IS the pump order.)
	for (int k = 0; k < 5; ++k) REQUIRE(order[k] == 100 + (uint32_t)k);
	for (int k = 0; k < 10; ++k) REQUIRE(order[5 + k] == 200 + (uint32_t)k);

	s_net_free(&np);
	return true;
}

/* A reliable channel delivers every message in order through heavy simulated packet loss, and a
 * message far bigger than one transport fragment survives fragmentation. */
TEST_CASE(test_net_channel_reliable_loss)
{
	NetPair np;
	REQUIRE(s_net_start(&np, "127.0.0.1:5803"));
	cf_client_enable_network_simulator(np.client, 0, 0, 0.25, 0);
	cf_server_enable_network_simulator(np.server, 0, 0, 0.25, 0);

	cf_client_channel_options(np.client, 3, 0, true);
	int n = 60;
	for (uint32_t k = 0; k < (uint32_t)n; ++k) {
		uint8_t payload[64];
		int size = 1 + (int)(k % 60);
		for (int b = 0; b < size; ++b) payload[b] = (uint8_t)(k + b);
		REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 3, k, payload, size)));
	}
	// One big fragmented message rides behind them on the same channel.
	int big_size = 100 * 1024;
	uint8_t* big = (uint8_t*)cf_alloc(big_size);
	for (int b = 0; b < big_size; ++b) big[b] = (uint8_t)(b * 7);
	REQUIRE(!cf_is_error(cf_client_send_msg(np.client, 3, 0xB16, big, big_size)));

	uint32_t expect = 0;
	bool got_big = false;
	for (int i = 0; i < 30000 && !got_big; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		int ci;
		uint32_t id;
		void* data;
		int size;
		while (cf_server_pop_msg(np.server, &ci, &id, &data, &size)) {
			if (id == 0xB16) {
				REQUIRE(expect == (uint32_t)n); // The big one comes last: FIFO held through loss.
				REQUIRE(size == big_size);
				REQUIRE(!memcmp(data, big, big_size));
				got_big = true;
			} else {
				REQUIRE(id == expect); // In order, no gaps, despite 25% loss each way.
				REQUIRE(size == 1 + (int)(expect % 60));
				REQUIRE(((uint8_t*)data)[0] == (uint8_t)expect);
				expect++;
			}
			cf_server_free_msg(np.server, data);
		}
		cf_sleep(1);
	}
	REQUIRE(expect == (uint32_t)n);
	REQUIRE(got_big);
	cf_free(big);

	s_net_free(&np);
	return true;
}

/* Bad sends fail loudly instead of corrupting the stream: oversized messages, bad channel indices,
 * unconnected client slots, and a flooded local queue. */
TEST_CASE(test_net_msg_errors)
{
	NetPair np;
	REQUIRE(s_net_start(&np, "127.0.0.1:5804"));

	char byte = 7;
	REQUIRE(cf_is_error(cf_client_send_msg(np.client, -1, 0, &byte, 1)));
	REQUIRE(cf_is_error(cf_client_send_msg(np.client, CF_NET_MAX_CHANNELS, 0, &byte, 1)));
	REQUIRE(cf_is_error(cf_client_send_msg(np.client, 0, 0, NULL, 1)));
	REQUIRE(cf_is_error(cf_client_send_msg(np.client, 0, 0, &byte, CF_NET_MAX_MSG_SIZE + 1)));
	REQUIRE(cf_is_error(cf_server_send_msg(np.server, np.client_index + 1, 0, 0, &byte, 1))); // Nobody there.
	REQUIRE(cf_is_error(cf_server_send_msg(np.server, -1, 0, 0, &byte, 1)));

	// Flood one channel's local queue without pumping (rate 0 pumps on update, so just don't
	// update): the cap eventually pushes back with an error instead of eating memory.
	cf_client_set_send_rate(np.client, 1); // Effectively frozen.
	uint8_t* chunk = (uint8_t*)cf_alloc(CF_NET_MAX_MSG_SIZE);
	memset(chunk, 3, CF_NET_MAX_MSG_SIZE);
	bool full = false;
	for (int i = 0; i < 64 && !full; ++i) {
		full = cf_is_error(cf_client_send_msg(np.client, 5, 1, chunk, CF_NET_MAX_MSG_SIZE));
	}
	REQUIRE(full);
	cf_free(chunk);

	s_net_free(&np);
	return true;
}

// A UDP socket dressed as a CF_ClientWire: in-process, this is exactly what a web build's
// relay does from the server's point of view -- datagrams arrive on its normal UDP socket
// while the client rides the wire.
#ifdef _WIN32
#	include <winsock2.h>
#	pragma comment(lib, "ws2_32.lib")
	typedef SOCKET WireSocketHandle;
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <fcntl.h>
#	include <unistd.h>
	typedef int WireSocketHandle;
#endif

struct WireSocket
{
	WireSocketHandle s;
	sockaddr_in to;
};

static bool s_wire_socket_open(WireSocket* w, uint16_t port)
{
#ifdef _WIN32
	WSADATA wsa; // Refcounted; harmless alongside cn's own WSAStartup.
	if (WSAStartup(MAKEWORD(2, 2), &wsa)) return false;
#endif
	w->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset(&w->to, 0, sizeof(w->to));
	w->to.sin_family = AF_INET;
	w->to.sin_port = htons(port);
	w->to.sin_addr.s_addr = htonl(0x7F000001); // 127.0.0.1
#ifdef _WIN32
	if (w->s == INVALID_SOCKET) return false;
	u_long yes = 1;
	ioctlsocket(w->s, FIONBIO, &yes);
#else
	if (w->s < 0) return false;
	fcntl(w->s, F_SETFL, fcntl(w->s, F_GETFL, 0) | O_NONBLOCK);
#endif
	return true;
}

static void s_wire_socket_close(WireSocket* w)
{
#ifdef _WIN32
	closesocket(w->s);
#else
	close(w->s);
#endif
}

static int s_wire_send(void* udata, const void* data, int size)
{
	WireSocket* w = (WireSocket*)udata;
	return (int)sendto(w->s, (const char*)data, size, 0, (sockaddr*)&w->to, sizeof(w->to));
}

static int s_wire_recv(void* udata, void* data, int size)
{
	WireSocket* w = (WireSocket*)udata;
	int n = (int)recvfrom(w->s, (char*)data, size, 0, NULL, NULL);
	return n < 0 ? 0 : n; // Nonblocking: a would-block error means nothing pending.
}

/* The whole protocol -- crypto handshake, raw packets, reliable channel messages -- flows
 * through a CF_ClientWire while the server keeps its ordinary UDP socket, never able to tell
 * the difference. This is the seam web builds ride (cf_client_web_wire). */
TEST_CASE(test_net_client_wire)
{
	WireSocket w;
	REQUIRE(s_wire_socket_open(&w, 5805));
	CF_ClientWire wire;
	wire.udata = &w;
	wire.send = s_wire_send;
	wire.recv = s_wire_recv;

	NetPair np;
	REQUIRE(s_net_start_wired(&np, "127.0.0.1:5805", &wire));

	// Raw client -> server through the wire.
	const char raw[] = "datagrams in disguise";
	REQUIRE(!cf_is_error(cf_client_send(np.client, raw, sizeof(raw), true)));
	bool got_raw = false;
	for (int i = 0; i < 5000 && !got_raw; ++i) {
		s_net_update(&np);
		CF_ServerEvent e;
		while (cf_server_pop_event(np.server, &e)) {
			if (e.type == CF_SERVER_EVENT_TYPE_PAYLOAD_PACKET) {
				REQUIRE(e.u.payload_packet.size == (int)sizeof(raw));
				REQUIRE(!memcmp(e.u.payload_packet.data, raw, sizeof(raw)));
				cf_server_free_packet(np.server, e.u.payload_packet.client_index, e.u.payload_packet.data);
				got_raw = true;
			}
		}
		cf_sleep(1);
	}
	REQUIRE(got_raw);

	// A reliable message server -> client through the wire.
	cf_server_channel_options(np.server, 0, 0, true);
	REQUIRE(!cf_is_error(cf_server_send_msg(np.server, np.client_index, 0, 42, "wired", 6)));
	bool got_msg = false;
	for (int i = 0; i < 5000 && !got_msg; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		void* pkt;
		int size;
		while (cf_client_pop_packet(np.client, &pkt, &size, NULL)) cf_client_free_packet(np.client, pkt);
		uint32_t id;
		void* data;
		while (cf_client_pop_msg(np.client, &id, &data, &size)) {
			REQUIRE(id == 42);
			REQUIRE(!CF_STRCMP((const char*)data, "wired"));
			cf_client_free_msg(np.client, data);
			got_msg = true;
		}
		cf_sleep(1);
	}
	REQUIRE(got_msg);

	s_net_free(&np);
	s_wire_socket_close(&w);
	return true;
}

/* Reliable messages deliver both ways on a SECOND connection of the same client object. The
 * transport's reliability state (sequence windows, acks, fragment reassembly) must start over
 * with every connect: carried over, the client silently discards the new server's reliable
 * stream (and the server stashes the client's) while unreliable traffic still flows -- a
 * half-alive session on every instance switch or server bounce. */
TEST_CASE(test_net_reconnect_reliable)
{
	NetPair np;
	REQUIRE(s_net_start(&np, "127.0.0.1:5806"));
	cf_server_channel_options(np.server, 0, 0, true);
	cf_client_channel_options(np.client, 0, 0, true);

	auto exchange = [&](uint32_t id_down, uint32_t id_up) -> bool {
		int payload = 7;
		if (cf_is_error(cf_server_send_msg(np.server, np.client_index, 0, id_down, &payload, sizeof(payload)))) return false;
		if (cf_is_error(cf_client_send_msg(np.client, 0, id_up, &payload, sizeof(payload)))) return false;
		bool down = false, up = false;
		for (int i = 0; i < 5000 && !(down && up); ++i) {
			s_net_update(&np);
			s_drain_events(&np);
			void* pkt;
			int size;
			while (cf_client_pop_packet(np.client, &pkt, &size, NULL)) cf_client_free_packet(np.client, pkt);
			uint32_t id;
			void* data;
			while (cf_client_pop_msg(np.client, &id, &data, &size)) {
				if (id == id_down) down = true;
				cf_client_free_msg(np.client, data);
			}
			int ci;
			while (cf_server_pop_msg(np.server, &ci, &id, &data, &size)) {
				if (id == id_up) up = true;
				cf_server_free_msg(np.server, data);
			}
			cf_sleep(1);
		}
		return down && up;
	};

	// First connection: a welcome down, an action up.
	REQUIRE(exchange(100, 101));

	// Drop the connection and reconnect the SAME client object (an instance switch, a bounce).
	cf_client_disconnect(np.client);
	for (int i = 0; i < 100; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		cf_sleep(1);
	}
	np.client_index = -1;
	REQUIRE(!cf_is_error(cf_client_connect_insecure(np.client, "127.0.0.1:5806", TEST_NET_APP_ID)));
	bool reconnected = false;
	for (int i = 0; i < 5000 && !reconnected; ++i) {
		s_net_update(&np);
		s_drain_events(&np);
		reconnected = cf_client_state(np.client) == CF_CLIENT_STATE_CONNECTED && np.client_index >= 0;
		cf_sleep(1);
	}
	REQUIRE(reconnected);

	// The same exchange on the new conversation.
	REQUIRE(exchange(200, 201));

	s_net_free(&np);
	return true;
}

TEST_SUITE(test_networking)
{
	RUN_TEST_CASE(test_net_raw_and_msgs);
	RUN_TEST_CASE(test_net_channel_priority);
	RUN_TEST_CASE(test_net_channel_reliable_loss);
	RUN_TEST_CASE(test_net_msg_errors);
	RUN_TEST_CASE(test_net_client_wire);
	RUN_TEST_CASE(test_net_reconnect_reliable);
}

#else // CF_EMSCRIPTEN

TEST_SUITE(test_networking)
{
}

#endif // CF_EMSCRIPTEN
