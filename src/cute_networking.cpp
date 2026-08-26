/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_networking.h>
#include <cute_alloc.h>
#include <cute_c_runtime.h>
#include <cute_time.h>
#include <time.h>

// Web builds compile all of this too: a client rides a CF_ClientWire (see cf_client_web_wire at
// the bottom of this file) since browsers have no UDP, while servers -- whose sockets cannot open
// in a browser -- fail cleanly at cf_server_start.
#define CUTE_NET_IMPLEMENTATION
#define CN_ALLOC(size, ctx) cf_alloc(size)
#define CN_FREE(mem, ctx) cf_free(mem)
#include <cute/cute_net.h>
#include <cute/cute_arith.h>

// cute_net is walled off behind the public CF_* API (see cute_networking.h). CF owns its own
// networking types; the ones below are defined so their layouts match cute_net's, letting this file
// bridge the two by reinterpret_cast / field copy. These static asserts are the guardrail that
// keeps the two definitions from ever drifting apart.
CF_STATIC_ASSERT(CF_CONNECT_TOKEN_SIZE == CN_CONNECT_TOKEN_SIZE, "Must be equal.");
CF_STATIC_ASSERT(CF_CONNECT_TOKEN_USER_DATA_SIZE == CN_CONNECT_TOKEN_USER_DATA_SIZE, "Must be equal.");
CF_STATIC_ASSERT(CF_SERVER_MAX_CLIENTS == CN_SERVER_MAX_CLIENTS, "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoKey) == sizeof(cn_crypto_key_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoSignPublic) == sizeof(cn_crypto_sign_public_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_CryptoSignSecret) == sizeof(cn_crypto_sign_secret_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_Address) == sizeof(cn_endpoint_t), "Must be equal.");
CF_STATIC_ASSERT(sizeof(CF_ServerEvent) == sizeof(cn_server_event_t), "Must be equal.");

static CF_INLINE CF_Result cf_wrap(cn_result_t cn_result)
{
	CF_Result result;
	result.code = cn_result.code;
	result.details = cn_result.details;
	return result;
}

// Layout-compatible reinterpretations across the wall.
static CF_INLINE cn_endpoint_t cf_to_cn_endpoint(CF_Address a) { cn_endpoint_t e; CF_MEMCPY(&e, &a, sizeof(e)); return e; }
static CF_INLINE CF_CryptoKey cf_from_cn_key(cn_crypto_key_t k) { CF_CryptoKey out; CF_MEMCPY(&out, &k, sizeof(out)); return out; }

//--------------------------------------------------------------------------------------------------
// MESSAGES + CHANNELS (internals)
//
// CF owns the wire above cute_net, so every packet CF sends starts with one framing byte: RAW for
// user packets from cf_client_send / cf_server_send, or BUNDLE for coalesced channel messages. The
// receive paths demux on that byte, which is why raw pop/free hand out pointers offset by one.

#define CF_NET_PREFIX_RAW    0
#define CF_NET_PREFIX_BUNDLE 1
#define CF_NET_BUNDLE_TARGET 1000               // Keep coalesced bundles inside one transport fragment (1100).
#define CF_NET_CHANNEL_QUEUE_BYTES (4 * 1024 * 1024) // Per-channel cap on locally queued bytes.

// One queued or received message; payload bytes live right after the header.
struct CF_MsgNode
{
	CF_MsgNode* next;
	int client_index; // Which client sent it (receive side on a server; -1 elsewhere).
	uint32_t id;
	int size;
};

static CF_MsgNode* s_msg_make(int client_index, uint32_t id, const void* data, int size)
{
	CF_MsgNode* n = (CF_MsgNode*)cf_alloc(sizeof(CF_MsgNode) + (size > 0 ? size : 1));
	n->next = NULL;
	n->client_index = client_index;
	n->id = id;
	n->size = size;
	if (size > 0) CF_MEMCPY(n + 1, data, size);
	return n;
}

struct CF_MsgList
{
	CF_MsgNode* head;
	CF_MsgNode* tail;
};

static void s_msg_push(CF_MsgList* l, CF_MsgNode* n)
{
	if (l->tail) l->tail->next = n;
	else l->head = n;
	l->tail = n;
}

static CF_MsgNode* s_msg_pop(CF_MsgList* l)
{
	CF_MsgNode* n = l->head;
	if (n) {
		l->head = n->next;
		if (!l->head) l->tail = NULL;
	}
	return n;
}

static void s_msg_clear(CF_MsgList* l)
{
	while (CF_MsgNode* n = s_msg_pop(l)) cf_free(n);
}

// Drop not-yet-popped messages from one client index (a recycled slot must not misattribute the
// previous occupant's messages to the new connection).
static void s_msg_remove_client(CF_MsgList* l, int client_index)
{
	CF_MsgNode** link = &l->head;
	l->tail = NULL;
	while (CF_MsgNode* n = *link) {
		if (n->client_index == client_index) {
			*link = n->next;
			cf_free(n);
		} else {
			l->tail = n;
			link = &n->next;
		}
	}
}

struct CF_ChannelOpts
{
	int priority;  // Higher pumps first.
	bool reliable;
};

// One peer's outgoing channel queues (a client has one; a server has one per client slot).
struct CF_PeerChannels
{
	CF_MsgList q[CF_NET_MAX_CHANNELS];
	int queued_bytes[CF_NET_MAX_CHANNELS];
	double budget; // Token bucket in bytes; may go negative (debt) after an oversized send.
};

static void s_peer_reset(CF_PeerChannels* p)
{
	for (int i = 0; i < CF_NET_MAX_CHANNELS; ++i) {
		s_msg_clear(&p->q[i]);
		p->queued_bytes[i] = 0;
	}
	p->budget = 0;
}

// LEB128-style varints frame each message inside a bundle: [id][size][bytes].
static int s_varint_write(uint8_t* out, uint32_t v)
{
	int n = 0;
	do {
		uint8_t b = v & 0x7F;
		v >>= 7;
		out[n++] = b | (v ? 0x80 : 0);
	} while (v);
	return n;
}

// Returns bytes consumed, or 0 on truncated/overlong input.
static int s_varint_read(const uint8_t* p, const uint8_t* end, uint32_t* v)
{
	uint32_t out = 0;
	for (int n = 0; n < 5 && p + n < end; ++n) {
		out |= (uint32_t)(p[n] & 0x7F) << (n * 7);
		if (!(p[n] & 0x80)) { *v = out; return n + 1; }
	}
	return 0;
}

static uint8_t* s_scratch_fit(uint8_t** buf, int* cap, int size)
{
	if (*cap < size) {
		int new_cap = *cap > 0 ? *cap : 4096;
		while (new_cap < size) new_cap *= 2;
		*buf = (uint8_t*)cf_realloc(*buf, new_cap);
		*cap = new_cap;
	}
	return *buf;
}

static CF_Result s_queue_msg(CF_PeerChannels* peer, uint8_t** scratch, int* scratch_cap, int channel, uint32_t id, const void* data, int size)
{
	if (channel < 0 || channel >= CF_NET_MAX_CHANNELS) return cf_result_error("Invalid channel index.");
	if (size < 0 || size > CF_NET_MAX_MSG_SIZE) return cf_result_error("Message size out of range (see CF_NET_MAX_MSG_SIZE).");
	if (size > 0 && !data) return cf_result_error("NULL data with non-zero size.");
	if (peer->queued_bytes[channel] + size > CF_NET_CHANNEL_QUEUE_BYTES) return cf_result_error("Channel send queue is full.");
	// Grow the bundle scratch now so the pump never has to handle allocation.
	s_scratch_fit(scratch, scratch_cap, size + 16);
	s_msg_push(&peer->q[channel], s_msg_make(-1, id, data, size));
	peer->queued_bytes[channel] += size;
	return cf_result_success();
}

typedef cn_result_t (CF_ChannelSendFn)(void* udata, const void* data, int size, bool reliable);

// Drain a peer's channel queues into the transport, highest priority channel first, coalescing
// messages into bundles, under the byte budget. Called once per update while connected.
static void s_pump(const CF_ChannelOpts* opts, CF_PeerChannels* peer, int rate, double dt, uint8_t* scratch, CF_ChannelSendFn* send, void* udata)
{
	if (rate > 0) {
		peer->budget += (double)rate * dt;
		if (peer->budget > (double)rate) peer->budget = (double)rate; // Cap the burst at one second's worth.
	}
	// Strict priority, ties broken by lower channel index. N is tiny; select rather than sort.
	bool visited[CF_NET_MAX_CHANNELS] = { 0 };
	for (int pass = 0; pass < CF_NET_MAX_CHANNELS; ++pass) {
		int best = -1;
		for (int i = 0; i < CF_NET_MAX_CHANNELS; ++i) {
			if (visited[i]) continue;
			if (best < 0 || opts[i].priority > opts[best].priority) best = i;
		}
		visited[best] = true;
		CF_MsgList* q = &peer->q[best];
		while (q->head) {
			if (rate > 0 && peer->budget <= 0) return; // Out of budget; the rest waits for the next update.
			// Coalesce messages from the front of the queue into one bundle.
			int used = 0;
			scratch[used++] = CF_NET_PREFIX_BUNDLE;
			int packed = 0;
			for (CF_MsgNode* n = q->head; n; n = n->next) {
				uint8_t hdr[10];
				int hn = s_varint_write(hdr, n->id);
				hn += s_varint_write(hdr + hn, (uint32_t)n->size);
				// A lone oversized message still ships (the transport fragments it); otherwise stop
				// packing at the fragment-friendly target.
				if (packed > 0 && used + hn + n->size > CF_NET_BUNDLE_TARGET + 1) break;
				CF_MEMCPY(scratch + used, hdr, hn);
				used += hn;
				if (n->size > 0) CF_MEMCPY(scratch + used, n + 1, n->size);
				used += n->size;
				packed++;
				if (used > CF_NET_BUNDLE_TARGET) break;
			}
			cn_result_t r = send(udata, scratch, used, opts[best].reliable);
			if (cn_is_error(r)) {
				// Reliable messages stay queued and retry next update (backpressure against a full
				// reliable layer); unreliable ones are droppable by contract, so drop rather than
				// let stale state pile up.
				if (!opts[best].reliable) {
					for (int k = 0; k < packed; ++k) {
						CF_MsgNode* d = s_msg_pop(q);
						peer->queued_bytes[best] -= d->size;
						cf_free(d);
					}
				}
				return; // The transport is unhappy; stop pumping until next update.
			}
			peer->budget -= (double)used;
			for (int k = 0; k < packed; ++k) {
				CF_MsgNode* d = s_msg_pop(q);
				peer->queued_bytes[best] -= d->size;
				cf_free(d);
			}
		}
	}
}

// Split a received bundle into messages on the receive queue. Bounds-safe: a malformed tail is
// dropped (can only come from a version-mismatched peer -- the transport authenticates packets).
static void s_parse_bundle(CF_MsgList* rx, int client_index, const uint8_t* p, int n)
{
	const uint8_t* end = p + n;
	while (p < end) {
		uint32_t id, size;
		int a = s_varint_read(p, end, &id);
		if (!a) return;
		p += a;
		int b = s_varint_read(p, end, &size);
		if (!b) return;
		p += b;
		if (size > (uint32_t)(end - p) || size > CF_NET_MAX_MSG_SIZE) return;
		s_msg_push(rx, s_msg_make(client_index, id, p, (int)size));
		p += size;
	}
}

//--------------------------------------------------------------------------------------------------
// ADDRESSES + CRYPTO

int cf_address_init(CF_Address* address, const char* address_and_port_string)
{
	return cn_endpoint_init((cn_endpoint_t*)address, address_and_port_string);
}

void cf_address_to_string(CF_Address address, char* buffer, int buffer_size)
{
	cn_endpoint_to_string(cf_to_cn_endpoint(address), buffer, buffer_size);
}

bool cf_address_equals(CF_Address a, CF_Address b)
{
	return cn_endpoint_equals(cf_to_cn_endpoint(a), cf_to_cn_endpoint(b)) != 0;
}

CF_CryptoKey cf_crypto_generate_key()
{
	return cf_from_cn_key(cn_crypto_generate_key());
}

void cf_crypto_random_bytes(void* data, int byte_count)
{
	cn_crypto_random_bytes(data, byte_count);
}

void cf_crypto_sign_keygen(CF_CryptoSignPublic* public_key, CF_CryptoSignSecret* secret_key)
{
	cn_crypto_sign_keygen((cn_crypto_sign_public_t*)public_key, (cn_crypto_sign_secret_t*)secret_key);
}

CF_Result cf_generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const CF_CryptoKey* client_to_server_key,
	const CF_CryptoKey* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int address_count,
	const char** address_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const CF_CryptoSignSecret* shared_secret_key,
	uint8_t* token_ptr_out
)
{
	cn_result_t result = cn_generate_connect_token(
		application_id,
		creation_timestamp,
		(const cn_crypto_key_t*)client_to_server_key,
		(const cn_crypto_key_t*)server_to_client_key,
		expiration_timestamp,
		handshake_timeout,
		address_count,
		address_list,
		client_id,
		user_data,
		(const cn_crypto_sign_secret_t*)shared_secret_key,
		token_ptr_out);
	return cf_wrap(result);
}

//--------------------------------------------------------------------------------------------------
// CLIENT

// The public CF_Client wraps the walled cute_net client with CF's channel layer.
struct CF_Client
{
	cn_client_t* cn;
	CF_ChannelOpts opts[CF_NET_MAX_CHANNELS];
	CF_PeerChannels send;
	CF_MsgList rx;
	int rate; // Bytes per second budget for the pump; 0 = unlimited.
	uint8_t* scratch;
	int scratch_cap;
	CF_ClientWire wire;      // Optional datagram transport (see cf_client_set_wire); cn_wire installed when wire.send is set.
	cn_wire_t cn_wire;
	int web_wire_handle;     // Nonzero when cf_client_web_wire owns a JS-side connection.
};

static int s_wire_send_thunk(void* udata, const void* data, int size)
{
	CF_Client* client = (CF_Client*)udata;
	return client->wire.send(client->wire.udata, data, size);
}

static int s_wire_recv_thunk(void* udata, void* data, int size)
{
	CF_Client* client = (CF_Client*)udata;
	return client->wire.recv(client->wire.udata, data, size);
}

void cf_client_set_wire(CF_Client* client, CF_ClientWire wire)
{
	client->wire = wire;
	client->cn_wire.udata = client;
	client->cn_wire.send = s_wire_send_thunk;
	client->cn_wire.recv = s_wire_recv_thunk;
	cn_client_set_wire(client->cn, &client->cn_wire);
}

CF_Client* cf_make_client(
	uint16_t port,
	uint64_t application_id,
	bool use_ipv6 /* = false */
)
{
	cn_client_t* cn = cn_client_create(port, application_id, use_ipv6, NULL);
	if (!cn) return NULL;
	CF_Client* client = (CF_Client*)cf_calloc(1, sizeof(CF_Client));
	client->cn = cn;
	return client;
}

#ifdef CF_EMSCRIPTEN
extern "C" void s_js_wire_close(int h); // Defined via EM_JS at the bottom of this file.
#endif

void cf_destroy_client(CF_Client* client)
{
#ifdef CF_EMSCRIPTEN
	if (client->web_wire_handle) s_js_wire_close(client->web_wire_handle);
#endif
	cn_client_destroy(client->cn);
	s_peer_reset(&client->send);
	s_msg_clear(&client->rx);
	cf_free(client->scratch);
	cf_free(client);
}

CF_Result cf_client_connect(CF_Client* client, const uint8_t* connect_token)
{
	// A (re)connect is a fresh conversation: drop anything queued for or from the old one.
	s_peer_reset(&client->send);
	s_msg_clear(&client->rx);
	return cf_wrap(cn_client_connect(client->cn, connect_token));
}

void cf_client_disconnect(CF_Client* client)
{
	s_peer_reset(&client->send);
	cn_client_disconnect(client->cn);
}

void cf_client_update(CF_Client* client, double dt, uint64_t current_time)
{
	if (cn_client_state_get(client->cn) == CN_CLIENT_STATE_CONNECTED) {
		s_scratch_fit(&client->scratch, &client->scratch_cap, CF_NET_BUNDLE_TARGET + 32);
		s_pump(client->opts, &client->send, client->rate, dt,
			client->scratch,
			[](void* udata, const void* data, int size, bool reliable) { return cn_client_send((cn_client_t*)udata, data, size, reliable); },
			client->cn);
	}
	cn_client_update(client->cn, dt, current_time);
}

bool cf_client_pop_packet(CF_Client* client, void** packet, int* size, bool* was_sent_reliably /* = NULL */)
{
	void* p;
	int n;
	bool r;
	while (cn_client_pop_packet(client->cn, &p, &n, &r)) {
		uint8_t* b = (uint8_t*)p;
		if (n >= 1 && b[0] == CF_NET_PREFIX_RAW) {
			*packet = b + 1;
			*size = n - 1;
			if (was_sent_reliably) *was_sent_reliably = r;
			return true;
		}
		if (n >= 1 && b[0] == CF_NET_PREFIX_BUNDLE) s_parse_bundle(&client->rx, -1, b + 1, n - 1);
		cn_client_free_packet(client->cn, p);
	}
	return false;
}

void cf_client_free_packet(CF_Client* client, void* packet)
{
	// Undo the framing-byte offset from cf_client_pop_packet.
	cn_client_free_packet(client->cn, (uint8_t*)packet - 1);
}

CF_Result cf_client_send(CF_Client* client, const void* packet, int size, bool send_reliably)
{
	if (size < 0) return cf_result_error("Negative size.");
	uint8_t* s = s_scratch_fit(&client->scratch, &client->scratch_cap, size + 1);
	s[0] = CF_NET_PREFIX_RAW;
	if (size > 0) CF_MEMCPY(s + 1, packet, size);
	return cf_wrap(cn_client_send(client->cn, s, size + 1, send_reliably));
}

void cf_client_channel_options(CF_Client* client, int channel, int priority, bool reliable)
{
	if (channel < 0 || channel >= CF_NET_MAX_CHANNELS) return;
	client->opts[channel].priority = priority;
	client->opts[channel].reliable = reliable;
}

void cf_client_set_send_rate(CF_Client* client, int bytes_per_second)
{
	client->rate = bytes_per_second;
}

CF_Result cf_client_send_msg(CF_Client* client, int channel, uint32_t id, const void* data, int size)
{
	return s_queue_msg(&client->send, &client->scratch, &client->scratch_cap, channel, id, data, size);
}

bool cf_client_pop_msg(CF_Client* client, uint32_t* id, void** data, int* size)
{
	CF_MsgNode* n = s_msg_pop(&client->rx);
	if (!n) return false;
	*id = n->id;
	*data = (void*)(n + 1);
	*size = n->size;
	return true;
}

void cf_client_free_msg(CF_Client* client, void* data)
{
	(void)client;
	cf_free((CF_MsgNode*)data - 1);
}

CF_ClientState cf_client_state(const CF_Client* client)
{
	return (CF_ClientState)cn_client_state_get(client->cn);
}

float cf_client_rtt(CF_Client* client)
{
	return cn_client_get_rtt_estimate(client->cn);
}

float cf_client_packet_loss(CF_Client* client)
{
	return cn_client_get_packet_loss_estimate(client->cn);
}

float cf_client_incoming_kbps(CF_Client* client)
{
	return cn_client_get_incoming_kbps_estimate(client->cn);
}

float cf_client_outgoing_kbps(CF_Client* client)
{
	return cn_client_get_outgoing_kbps_estimate(client->cn);
}

void cf_client_enable_network_simulator(CF_Client* client, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_client_enable_network_simulator(client->cn, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// SERVER

// The public CF_Server wraps the walled cute_net server with CF's channel layer, one set of
// outgoing queues per client slot.
struct CF_Server
{
	cn_server_t* cn;
	CF_ChannelOpts opts[CF_NET_MAX_CHANNELS];
	CF_PeerChannels peers[CF_SERVER_MAX_CLIENTS];
	CF_MsgList rx;
	int rate; // Bytes per second budget per client; 0 = unlimited.
	uint8_t* scratch;
	int scratch_cap;
};

CF_Server* cf_make_server(CF_ServerConfig config)
{
	cn_server_config_t cn_config;
	cn_config.application_id = config.application_id;
	cn_config.max_incoming_bytes_per_second = 0;
	cn_config.max_outgoing_bytes_per_second = 0;
	cn_config.connection_timeout = config.connection_timeout;
	cn_config.resend_rate = config.resend_rate;
	CF_MEMCPY(&cn_config.public_key, &config.public_key, sizeof(cn_config.public_key));
	CF_MEMCPY(&cn_config.secret_key, &config.secret_key, sizeof(cn_config.secret_key));
	cn_config.user_allocator_context = NULL;
	cn_server_t* cn = cn_server_create(cn_config);
	if (!cn) return NULL;
	CF_Server* server = (CF_Server*)cf_calloc(1, sizeof(CF_Server));
	server->cn = cn;
	return server;
}

void cf_destroy_server(CF_Server* server)
{
	cn_server_destroy(server->cn);
	for (int i = 0; i < CF_SERVER_MAX_CLIENTS; ++i) s_peer_reset(&server->peers[i]);
	s_msg_clear(&server->rx);
	cf_free(server->scratch);
	cf_free(server);
}

CF_Result cf_server_start(CF_Server* server, const char* address_and_port)
{
	return cf_wrap(cn_server_start(server->cn, address_and_port));
}

void cf_server_stop(CF_Server* server)
{
	for (int i = 0; i < CF_SERVER_MAX_CLIENTS; ++i) s_peer_reset(&server->peers[i]);
	s_msg_clear(&server->rx);
	cn_server_stop(server->cn);
}

void cf_server_set_public_ip(CF_Server* server, const char* address_and_port)
{
	cn_server_set_public_ip(server->cn, address_and_port);
}

bool cf_server_pop_event(CF_Server* server, CF_ServerEvent* event)
{
	cn_server_event_t e;
	while (cn_server_pop_event(server->cn, &e)) {
		if (e.type == CN_SERVER_EVENT_TYPE_PAYLOAD_PACKET) {
			uint8_t* b = (uint8_t*)e.u.payload_packet.data;
			int n = e.u.payload_packet.size;
			int ci = e.u.payload_packet.client_index;
			if (n >= 1 && b[0] == CF_NET_PREFIX_RAW) {
				// Strip the framing byte; cf_server_free_packet undoes the offset.
				e.u.payload_packet.data = b + 1;
				e.u.payload_packet.size = n - 1;
				CF_MEMCPY(event, &e, sizeof(e));
				return true;
			}
			if (n >= 1 && b[0] == CF_NET_PREFIX_BUNDLE) s_parse_bundle(&server->rx, ci, b + 1, n - 1);
			cn_server_free_packet(server->cn, ci, b);
			continue;
		}
		if (e.type == CN_SERVER_EVENT_TYPE_NEW_CONNECTION) {
			s_peer_reset(&server->peers[e.u.new_connection.client_index]);
			s_msg_remove_client(&server->rx, e.u.new_connection.client_index);
		} else if (e.type == CN_SERVER_EVENT_TYPE_DISCONNECTED) {
			s_peer_reset(&server->peers[e.u.disconnected.client_index]);
		}
		CF_MEMCPY(event, &e, sizeof(e));
		return true;
	}
	return false;
}

void cf_server_free_packet(CF_Server* server, int client_index, void* data)
{
	// Undo the framing-byte offset from cf_server_pop_event.
	cn_server_free_packet(server->cn, client_index, (uint8_t*)data - 1);
}

void cf_server_update(CF_Server* server, double dt, uint64_t current_time)
{
	s_scratch_fit(&server->scratch, &server->scratch_cap, CF_NET_BUNDLE_TARGET + 32);
	for (int i = 0; i < CF_SERVER_MAX_CLIENTS; ++i) {
		bool any = false;
		for (int c = 0; c < CF_NET_MAX_CHANNELS; ++c) {
			if (server->peers[i].q[c].head) { any = true; break; }
		}
		if (!any) continue;
		if (!cn_server_is_client_connected(server->cn, i)) continue;
		struct Ctx { cn_server_t* cn; int ci; } ctx = { server->cn, i };
		s_pump(server->opts, &server->peers[i], server->rate, dt,
			server->scratch,
			[](void* udata, const void* data, int size, bool reliable) {
				Ctx* c = (Ctx*)udata;
				return cn_server_send(c->cn, data, size, c->ci, reliable);
			},
			&ctx);
	}
	cn_server_update(server->cn, dt, current_time);
}

void cf_server_disconnect_client(CF_Server* server, int client_index, bool notify_client /* = true */)
{
	s_peer_reset(&server->peers[client_index]);
	cn_server_disconnect_client(server->cn, client_index, notify_client);
}

CF_Result cf_server_send(CF_Server* server, const void* packet, int size, int client_index, bool send_reliably)
{
	if (size < 0) return cf_result_error("Negative size.");
	uint8_t* s = s_scratch_fit(&server->scratch, &server->scratch_cap, size + 1);
	s[0] = CF_NET_PREFIX_RAW;
	if (size > 0) CF_MEMCPY(s + 1, packet, size);
	return cf_wrap(cn_server_send(server->cn, s, size + 1, client_index, send_reliably));
}

void cf_server_send_to_all(CF_Server* server, const void* packet, int size, bool send_reliably)
{
	if (size < 0) return;
	uint8_t* s = s_scratch_fit(&server->scratch, &server->scratch_cap, size + 1);
	s[0] = CF_NET_PREFIX_RAW;
	if (size > 0) CF_MEMCPY(s + 1, packet, size);
	for (int i = 0; i < CF_SERVER_MAX_CLIENTS; ++i) {
		if (cn_server_is_client_connected(server->cn, i)) {
			cn_server_send(server->cn, s, size + 1, i, send_reliably);
		}
	}
}

void cf_server_channel_options(CF_Server* server, int channel, int priority, bool reliable)
{
	if (channel < 0 || channel >= CF_NET_MAX_CHANNELS) return;
	server->opts[channel].priority = priority;
	server->opts[channel].reliable = reliable;
}

void cf_server_set_send_rate(CF_Server* server, int bytes_per_second)
{
	server->rate = bytes_per_second;
}

CF_Result cf_server_send_msg(CF_Server* server, int client_index, int channel, uint32_t id, const void* data, int size)
{
	if (client_index < 0 || client_index >= CF_SERVER_MAX_CLIENTS) return cf_result_error("Invalid client index.");
	if (!cn_server_is_client_connected(server->cn, client_index)) return cf_result_error("Client is not connected.");
	return s_queue_msg(&server->peers[client_index], &server->scratch, &server->scratch_cap, channel, id, data, size);
}

bool cf_server_pop_msg(CF_Server* server, int* client_index, uint32_t* id, void** data, int* size)
{
	CF_MsgNode* n = s_msg_pop(&server->rx);
	if (!n) return false;
	*client_index = n->client_index;
	*id = n->id;
	*data = (void*)(n + 1);
	*size = n->size;
	return true;
}

void cf_server_free_msg(CF_Server* server, void* data)
{
	(void)server;
	cf_free((CF_MsgNode*)data - 1);
}

bool cf_server_is_client_connected(CF_Server* server, int client_index)
{
	return cn_server_is_client_connected(server->cn, client_index);
}

float cf_server_rtt(CF_Server* server, int client_index)
{
	return cn_server_get_rtt_estimate(server->cn, client_index);
}

float cf_server_packet_loss(CF_Server* server, int client_index)
{
	return cn_server_get_packet_loss_estimate(server->cn, client_index);
}

float cf_server_incoming_kbps(CF_Server* server, int client_index)
{
	return cn_server_get_incoming_kbps_estimate(server->cn, client_index);
}

float cf_server_outgoing_kbps(CF_Server* server, int client_index)
{
	return cn_server_get_outgoing_kbps_estimate(server->cn, client_index);
}

void cf_server_enable_network_simulator(CF_Server* server, double latency, double jitter, double drop_chance, double duplicate_chance)
{
	cn_server_enable_network_simulator(server->cn, latency, jitter, drop_chance, duplicate_chance);
}

//--------------------------------------------------------------------------------------------------
// COMPRESSION

int cf_snapshot_compress(const void* baseline, const void* current, int size, void* out, int out_capacity)
{
	return cf_arith_delta_compress((const uint8_t*)baseline, (const uint8_t*)current, size, (uint8_t*)out, out_capacity);
}

int cf_snapshot_decompress(const void* baseline, int size, const void* compressed, int compressed_size, void* out)
{
	return cf_arith_delta_decompress((const uint8_t*)baseline, size, (const uint8_t*)compressed, compressed_size, (uint8_t*)out);
}

//--------------------------------------------------------------------------------------------------
// DEV CONVENIENCES

// A fixed, well-known keypair so an insecure client and server interoperate with no key exchange.
// INSECURE BY DESIGN: the secret is compiled into every build, so anyone can forge connect tokens.
// For local development, singleplayer/listen-server, and tests only -- never ship a real server with
// these keys. Generate real keys with cf_crypto_sign_keygen for production.
static const uint8_t CF_DEV_SIGN_PUBLIC[32] = {
	0x3b,0x88,0x0b,0x22,0xb6,0xe7,0x1a,0xee,0x77,0x73,0x90,0xb7,
	0xf6,0xb3,0x03,0x07,0x65,0x34,0x62,0xc9,0x01,0x2c,0x45,0x18,
	0xf4,0xce,0x45,0x08,0xfc,0xa8,0xd7,0x05,
};
static const uint8_t CF_DEV_SIGN_SECRET[64] = {
	0x91,0xa2,0xfa,0xcf,0x3c,0x84,0x13,0x3b,0xa2,0x80,0x2b,0x6b,
	0xd7,0xbf,0x3a,0xc8,0xa2,0xaa,0xbc,0xfe,0xb9,0xdf,0xbf,0x8b,
	0x2f,0x14,0xd4,0x35,0xa3,0xdd,0x81,0x20,0x3b,0x88,0x0b,0x22,
	0xb6,0xe7,0x1a,0xee,0x77,0x73,0x90,0xb7,0xf6,0xb3,0x03,0x07,
	0x65,0x34,0x62,0xc9,0x01,0x2c,0x45,0x18,0xf4,0xce,0x45,0x08,
	0xfc,0xa8,0xd7,0x05,
};
static const uint8_t CF_DEV_C2S_KEY[32] = {
	0x87,0xdc,0xd5,0xbd,0x47,0x51,0x55,0xf2,0xe2,0x8d,0x3e,0x37,
	0x97,0x36,0xef,0x36,0x5b,0xc6,0x90,0x1d,0x4c,0x10,0x3d,0x2f,
	0xb6,0xec,0x34,0x90,0xc4,0x60,0xae,0xed,
};
static const uint8_t CF_DEV_S2C_KEY[32] = {
	0x9f,0x33,0xde,0x4c,0x0d,0x05,0x57,0x0f,0x45,0xe0,0x42,0x41,
	0x20,0x48,0x5f,0x2a,0x3c,0xd0,0x5a,0x36,0x31,0xfb,0x57,0x11,
	0x6c,0xd1,0xcd,0x86,0xce,0xbd,0x8c,0x19,
};

CF_Server* cf_make_server_insecure(uint64_t application_id)
{
	CF_ServerConfig config = cf_server_config_defaults();
	config.application_id = application_id;
	CF_MEMCPY(&config.public_key, CF_DEV_SIGN_PUBLIC, sizeof(CF_DEV_SIGN_PUBLIC));
	CF_MEMCPY(&config.secret_key, CF_DEV_SIGN_SECRET, sizeof(CF_DEV_SIGN_SECRET));
	return cf_make_server(config);
}

CF_Result cf_client_connect_insecure(CF_Client* client, const char* address_and_port, uint64_t application_id)
{
	CF_CryptoKey c2s, s2c;
	CF_CryptoSignSecret sk;
	CF_MEMCPY(&c2s, CF_DEV_C2S_KEY, sizeof(CF_DEV_C2S_KEY));
	CF_MEMCPY(&s2c, CF_DEV_S2C_KEY, sizeof(CF_DEV_S2C_KEY));
	CF_MEMCPY(&sk, CF_DEV_SIGN_SECRET, sizeof(CF_DEV_SIGN_SECRET));
	uint64_t now = (uint64_t)time(NULL);
	uint64_t client_id = 0;
	cf_crypto_random_bytes(&client_id, sizeof(client_id)); // Distinct per client so many can connect.
	const char* addrs[1] = { address_and_port };
	uint8_t token[CF_CONNECT_TOKEN_SIZE];
	CF_Result r = cf_generate_connect_token(application_id, now, &c2s, &s2c, now + 31536000ull, 10, 1, addrs, client_id, NULL, &sk, token);
	if (cf_is_error(r)) return r;
	return cf_client_connect(client, token);
}

void cf_client_tick(CF_Client* client)
{
	cf_client_update(client, CF_DELTA_TIME, (uint64_t)time(NULL));
}

void cf_server_tick(CF_Server* server)
{
	cf_server_update(server, CF_DELTA_TIME, (uint64_t)time(NULL));
}

//--------------------------------------------------------------------------------------------------
// The web wire: WebTransport datagrams with a WebSocket fallback, one datagram per message either
// way. All connection state lives in JS; C polls a per-wire receive queue. Opening is async and
// sends before the pipe is up are dropped -- the protocol's redundant handshake absorbs that.

#ifdef CF_EMSCRIPTEN

#include <emscripten/emscripten.h>

// clang-format off
EM_JS(void, s_js_wire_open, (int h, const char* url_cstr), {
	var url = UTF8ToString(url_cstr);
	if (!Module.cfWires) Module.cfWires = {};
	var w = { q: [], open: false, dead: false, wt: null, ws: null, writer: null };
	Module.cfWires[h] = w;
	var push = function(bytes) {
		if (w.q.length >= 256) w.q.shift(); // Datagram semantics: drop oldest under pressure.
		w.q.push(bytes);
	};
	var fallback = function() {
		if (w.dead || w.ws) return;
		try {
			var ws = new WebSocket(url.replace(/^http/, "ws"));
			ws.binaryType = "arraybuffer";
			ws.onmessage = function(e) { push(new Uint8Array(e.data)); };
			ws.onopen = function() { w.open = true; };
			ws.onclose = function() { if (w.open || !w.wt) w.dead = true; };
			ws.onerror = function() { if (!w.open) w.dead = true; };
			w.ws = ws;
		} catch (e) { w.dead = true; }
	};
	if (typeof WebTransport !== "undefined") {
		try {
			var wt = new WebTransport(url);
			w.wt = wt;
			wt.ready.then(function() {
				w.open = true;
				w.writer = wt.datagrams.writable.getWriter();
				var reader = wt.datagrams.readable.getReader();
				var pump = function() {
					reader.read().then(function(r) {
						if (r.done || w.dead) return;
						push(r.value);
						pump();
					}).catch(function() { w.dead = true; });
				};
				pump();
				wt.closed.then(function() { w.dead = true; }).catch(function() { w.dead = true; });
			}).catch(function() { w.wt = null; fallback(); });
		} catch (e) { w.wt = null; fallback(); }
	} else {
		fallback();
	}
});

EM_JS(int, s_js_wire_send, (int h, const void* data, int size), {
	var w = Module.cfWires && Module.cfWires[h];
	if (!w || w.dead) return -1;
	if (!w.open) return 0; // Still connecting: drop, the handshake retries.
	var bytes = HEAPU8.slice(data, data + size);
	try {
		if (w.writer) w.writer.write(bytes);
		else if (w.ws && w.ws.readyState === 1) w.ws.send(bytes);
		else return 0;
	} catch (e) { return -1; }
	return size;
});

EM_JS(int, s_js_wire_recv, (int h, void* data, int size), {
	var w = Module.cfWires && Module.cfWires[h];
	if (!w) return -1;
	if (!w.q.length) return w.dead ? -1 : 0;
	var bytes = w.q.shift();
	var n = Math.min(bytes.length, size);
	HEAPU8.set(bytes.subarray(0, n), data);
	return n;
});

EM_JS(void, s_js_wire_close, (int h), {
	var w = Module.cfWires && Module.cfWires[h];
	if (!w) return;
	w.dead = true;
	try { if (w.wt) w.wt.close(); } catch (e) {}
	try { if (w.ws) w.ws.close(); } catch (e) {}
	delete Module.cfWires[h];
});
// clang-format on

static int s_web_wire_send(void* udata, const void* data, int size) { return s_js_wire_send((int)(uintptr_t)udata, data, size); }
static int s_web_wire_recv(void* udata, void* data, int size) { return s_js_wire_recv((int)(uintptr_t)udata, data, size); }

CF_Result cf_client_web_wire(CF_Client* client, const char* url)
{
	if (!url || (CF_STRNCMP(url, "https://", 8) && CF_STRNCMP(url, "http://", 7))) {
		return cf_result_error("cf_client_web_wire wants an http(s):// relay URL.");
	}
	static int next_handle = 1;
	int h = next_handle++;
	if (client->web_wire_handle) s_js_wire_close(client->web_wire_handle);
	client->web_wire_handle = h;
	s_js_wire_open(h, url);
	CF_ClientWire wire;
	wire.udata = (void*)(uintptr_t)h;
	wire.send = s_web_wire_send;
	wire.recv = s_web_wire_recv;
	cf_client_set_wire(client, wire);
	return cf_result_success();
}

#else // CF_EMSCRIPTEN

CF_Result cf_client_web_wire(CF_Client* client, const char* url)
{
	CF_UNUSED(client);
	CF_UNUSED(url);
	return cf_result_error("cf_client_web_wire is web (emscripten) machinery; on native builds connect over UDP as usual, or install your own CF_ClientWire.");
}

#endif // CF_EMSCRIPTEN
