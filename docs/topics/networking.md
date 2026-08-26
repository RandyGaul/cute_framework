# Networking

> [!NOTE]
> CF's networking is implemented by [cute_net.h](https://github.com/RandyGaul/cute_headers/blob/master/cute_net.h), a low level networking header. This code has not yet reached stable maturity -- use at your own risk! CF would like to release networking features officially in a future release.

CF's networking model uses a [client server networking architecture](https://en.wikipedia.org/wiki/Client%E2%80%93server_model). The underlying protocol works over UDP packets. There is no TCP support or peer-to-peer connections (web builds ride a relay instead -- see [Web Clients](#web-clients) below). However, CF does provide an [https API](../api_reference.md#web) for sending requests to an HTTP server. Here are the features of the client server API.

* Out-of-the-box security model based on state of the art connect tokens.
* Optional reliable and in-order packets.
* Packet fragmentation and reassembly, for larger packets over the [MTU](https://en.wikipedia.org/wiki/Maximum_transmission_unit).
* Basic client and server abstractions.

For those curious, the lower level guts of the client server API are implemented on top of the [Cute Protocol](protocol.md).

## Connect Tokens

Clients use connect tokens to connect to game servers. This only allows clients who authenticate themselves to connect and play on your game servers, granting completel control over who can or cannot play. This is important as dedicated game servers are typically fairly expensive to run, and usually only players who have paid for the game are able to obtain connect tokens.

You will have to distribute connect tokens to clients. The recommendation is to setup a web service to provide a REST API, like a simple HTTP server. The client can send an HTTP request, and the server responds with a connect token.

The client then reads the connect token, which contains a list of game servers to try and connect to along with other needed security info. Here's a diagram describing the process.

```
      +-----------+
      |    Web    |
      |  Service  |
      +-----------+
          ^  |
          |  |                            +-----------+              +-----------+
        REST Call                         | Dedicated |              | Dedicated |
        returns a                         | Server  1 |              | Server  2 |
      Connect Token                       +-----------+              +-----------+
          |  |                                  ^                          ^
          |  v                                  |                          |
       +--------+   *connect token packet* ->   |   if fail, try next ->   |
       | Client |-------------------------------+--------------------------+----------> ... Token timeout!
       +--------+
```

Once you get a connect token make a call to [`cf_client_connect`](../net/function/cf_client_connect.md).

## Web Service

The web service distributes connect tokens. CF does not provide an implementation of a web service because there are many different good solutions out there already. The goal is to only respond and provide connect tokens to clients who have authenticated themselves over a secure connection with the web service, such as through HTTPS. For example: this is how CF can be used to filter out players who have no purchased the game or signed up with an account.

## Generating Connect Tokens

The function [`cf_generate_connect_token`](../net/function/cf_generate_connect_token.md) can be used to generate connect tokens for the web service to distribute.

## Dedicated Game Servers

The game server itself is an instance of [`CF_Server`](../net/struct/cf_server.md) created by calling [`cf_make_server`](../net/function/cf_make_server.md). The server mediates connections from all clients; the server can at any moment force a client to disconnect, and only accepts new connections with clients that provide a valid connect token and pass the security handshake.

## Example Client and Server

Here is a [quick and dirty demonstration](https://github.com/RandyGaul/cf_net_test) showing how to setup a client and server for testing purposes. This shows basic usage of the client and server API, where multiple clients can connect to a single server. The connect tokens are generated on the dedicated server instead of using a web service, which is a great way to test things out during development.

## Web Clients

Browsers cannot open UDP sockets, so a client compiled with emscripten routes its packets through a *wire* instead: a pluggable datagram transport ([`CF_ClientWire`](../net/struct/cf_clientwire.md)) that replaces the client's internal socket. Each call to the wire moves one whole packet, with datagram semantics -- packets may arrive dropped, duplicated, or reordered, and the protocol handles all of that exactly as it does over UDP. The server keeps its ordinary UDP socket and cannot tell wired clients from native ones.

The ready-made browser wire is [`cf_client_web_wire`](../net/function/cf_client_web_wire.md): give it a URL like `https://mygame.example/wire/5601` after `cf_make_client` and before `cf_client_connect`. It tries [WebTransport](https://developer.mozilla.org/en-US/docs/Web/API/WebTransport) datagrams first (true unreliable datagrams over HTTP/3 -- no head-of-line blocking), and falls back to a WebSocket at the same URL (`https` -> `wss`) where WebTransport is unavailable or blocked. Both carry one datagram per message.

The URL points at a **relay**: a small service running next to your game server that forwards each browser message to the server's UDP port and each UDP response back, 1:1. The relay needs almost nothing:

* Accept WebTransport sessions and/or WebSocket upgrades at some path carrying the target port (e.g. `/wire/<port>`), allowlisted to your server's port range so it is not an open proxy.
* Per connection, open one UDP socket to the game server and pump datagrams both ways until either side closes (an idle timeout is wise).
* Nothing else. The relay is *untrusted by design*: every datagram is already encrypted and authenticated end-to-end by the connect-token handshake, so the relay only ever sees ciphertext, and a malicious relay can do nothing but drop packets -- which the protocol already tolerates.

Since the relay typically lives on the same host that serves the web build, the page can derive the wire URL from `location.origin`. Note the connect token still needs a parseable IP endpoint even though a wired client never dials it -- `127.0.0.1:<port>` works fine as a placeholder.

Serving a WebTransport endpoint requires an HTTP/3 stack and real TLS certificates (browsers only expose WebTransport in secure contexts); libraries like Go's `webtransport-go` plus automatic Let's Encrypt issuance make this a small, self-contained daemon. The WebSocket fallback needs only an ordinary HTTP(S) server. Servers themselves never run in browsers: on web, `cf_server_start` returns an error.
