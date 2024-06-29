[](../header.md ':include')

<br>


!> **Note** CF's networking is implemented by [cute_net.h](https://github.com/RandyGaul/cute_headers/blob/master/cute_net.h), a low level networking header. This code has not yet reached stable maturity -- use at your own risk! CF would like to release networking features officially in a future release.

CF's networking model uses a [client server networking architecture](https://en.wikipedia.org/wiki/Client%E2%80%93server_model). The underlying protocol works over UDP packets. There is no TCP support or peer-to-peer connections. However, CF does provide an [https API](https://randygaul.github.io/cute_framework/#/api_reference?id=web) for sending requests to an HTTP server. Here are the features of the client server API.

* Out-of-the-box security model based on state of the art connect tokens.
* Optional reliable and in-order packets.
* Packet fragmentation and reassembly, for larger packets over the [MTU](https://en.wikipedia.org/wiki/Maximum_transmission_unit).
* Basic client and server abstractions.

For those curious, the lower level guts of the client server API are implemented on top of the [Cute Protocol](/topics/protocol.md).

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

Once you get a connect token make a call to [`cf_client_connect`](https://randygaul.github.io/cute_framework/#/net/cf_client_connect).

## Web Service

The web service distributes connect tokens. CF does not provide an implementation of a web service because there are many different good solutions out there already. The goal is to only respond and provide connect tokens to clients who have authenticated themselves over a secure connection with the web service, such as through HTTPS. For example: this is how CF can be used to filter out players who have no purchased the game or signed up with an account.

## Generating Connect Tokens

The function [`cf_generate_connect_token`](https://randygaul.github.io/cute_framework/#/net/cf_generate_connect_token) can be used to generate connect tokens for the web service to distribute.

## Dedicated Game Servers

The game server itself is an instance of [`CF_Server`](https://randygaul.github.io/cute_framework/#/net/cf_server) created by calling [`cf_make_server`](https://randygaul.github.io/cute_framework/#/net/cf_make_server). The server mediates connections from all clients; the server can at any moment force a client to disconnect, and only accepts new connections with clients that provide a valid connect token and pass the security handshake.

## Example Client and Server

Here is a [quick and dirty demonstration](https://github.com/RandyGaul/cf_net_test) showing how to setup a client and server for testing purposes. This shows basic usage of the client and server API, where multiple clients can connect to a single server. The connect tokens are generated on the dedicated server instead of using a web service, which is a great way to test things out during development.
