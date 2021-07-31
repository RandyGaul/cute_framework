# Cute Protocol Standard

The Cute Protocol implements a client-server based transport layer for securely connecting and authenticating with backend game servers over UDP. The purpose of this document is to aid users in making educated decisions about how to use the Cute Protocol in their own game, or act as a reference during implementation. The intent is not for average users of Cute Framework (CF) to read or care about this document. Instead, if you happen to find yourself here randomly, you're probably looking for the Client/Server API (TODO - link here), which is the recommended API for implementing games.

Thanks to Glenn Fiedler for his blog and especially his implementation of [netcode.io](https://github.com/networkprotocol/netcode.io) - the majority of the knowledge needed to create Cute Protocol was gleaned from Glenn's online learning resources.

## High Level Overview

The main pieces of the Cute Protocol are:

1. Connect tokens.
2. Web service.
3. Dedicated servers.
4. Clients.

The web service provides an authentication mechanism via [REST](https://en.wikipedia.org/wiki/Representational_state_transfer) call ([HTTPS](https://en.wikipedia.org/wiki/HTTPS) is recommended, but not required). Any authentication technique can be used since authentication with a web service is: A) very well understood with many good pre-built solutions (like OAuth/2 or OpenID); B) easily isolated away from this document without a strong conceptual dependency. The details of the web service and its exact API are out of scope of the standard, except for how they produce a connect token.

The connect token is the mechanism that allows clients to securely authenticate with a dedicated server. Dedicated servers are the servers running the game. Clients are the players who connect to a dedicated server to play. The dedicated servers need not only provide a game service - it can be any online service that wants to run over UDP with a secure and live connection. However, for this document the backend servers are assumed to host a game service.

For this document, the user is the one who owns the web service and dedicated servers.

## Connecting as a Client

The steps for a client to connect to a dedicated game server are:

1. Client wants to authenticate with the web service and issues a REST call to obtain a connect token.
2. Web service generates and returns a connect token to the client. The `client to server key` and the `server to client key` must be uniquely generated for each connect token.
3. Client sends the token to a dedicated server instance to securely setup a connection and play.
4. The dedicated server processes the connect token, and if valid, starts the connection handshake with the client.
5. If the handshake succeeds, the player is connected and begins to play over a secure UDP channel.

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

## Connect Token Format

The connect token has three major sections.

1. PUBLIC SECTION
2. SECRET SECTION
3. REST SECTION

Once a client receives a connect token from the web service, the REST SECTION and the PUBLIC SECTION are read by the client. These sections contain a server IP list of dedicated game servers to attempt to connect to, along with some other data. Once read, the client deletes the REST SECTION. The remaining data in the connect token consists of 1024 bytes. The final 1024 bytes are called the *connect token packet*.

The entire *connect token packet* is not modifiable or forge-able, and the SECRET SECTION is not readable by anyone except the web service and dedicated game servers. The entire *connect token packet* is protected by a cryptographically secure AEAD ([Authenticated Encryption with Associated Data](https://en.wikipedia.org/wiki/Authenticated_encryption#Authenticated_encryption_with_associated_data)) primitive. The key used for the AEAD primitive is a shared secret known by all dedicated game servers, and the web service. It is recommended to implement a mechanism to rotate this key periodically, though the mechanism to do so is out of scope for this document.

##### Note:
> The AEAD primitive is a function that encrypts a chunk of data, and computes an HMAC ([keyed-hash message authentication code](https://en.wikipedia.org/wiki/HMAC)). The HMAC is a multi-byte value used to authenticate the message, and prevent tampering/modification of the message (i.e. maintain integrity of the message). The encryption ensures only those who know the key can read the message. The HMAC provides authentication (know who sent the message, and that it wasn't tampered). The HMAC is stored within the `signature` of the *connect token packet*. The `signature` is a 64 byte zone used to store the HMAC, along with any other data such as an initialization vector or nonce. How the signature bits are defined is up to the implementor to decide, and as such different implementations of Cute Protocol are _by design not necessarily compatible_. The `signature` can be treated as a black box depending on which crypto API the implementor uses, and is designed flexibly for ease of implementation.

The PUBLIC SECTION of the *connect token packet* can be used as Associated Data for the AEAD, where the SECRET SECTION is encrypted by the AEAD. Once the AEAD is used, the output *signature* is appended to the token, thus completing the full 1024 bytes *connect token packet*. This means the PUBLIC SECTION is not modifiable or forgeable by anyone except the web service or the dedicated backend servers, since they share the secret key used for the AEAD to encrypt the SECRET SECTION, using the PUBLIC SECTION as the Associated Data.

### The Connect Token Format
```
---  BEGIN REST SECTION  ---
version info                   10         "Cute 1.00" ASCII, including nul byte.
application id                 uint64_t   User chosen value to identify the game.
creation timestamp             uint64_t   Unix timestamp of when the connect token was created.
client to server key           32 bytes   Client uses to encrypt packets, server uses to decrypt packets.
server to client key           32 bytes   Server uses to encrypt packets, client uses to decrypt packets.
----  END REST SECTION  ----
--  BEGIN PUBLIC SECTION  --
packet type                    1 byte     Represents packet type of *connect token packet*; value of zero.
version info                   10         "Cute 1.00" ASCII, including nul byte.
application id                 uint64_t   User chosen value to identify the game.
expiration timestamp           uint64_t   Unix timestamp of when the connect token becomes invalid.
handshake timeout              uint32_t   Seconds of how long a connection handshake will wait before timing out.
number of server endpoints     uint32_t   The number of servers in the following list in the range of [1, 32].
<for each server endpoint>
    address type               uint8_t    1 = IPv4, 2 = IPv6 address, in the range of [1, 2].
    <if IPv4 address>          6 bytes    Format: a.b.c.d:port
        a                      uint8_t
        b                      uint8_t
        c                      uint8_t
        d                      uint8_t
        port                   uint16_t
    <else if IPv6 address>     18 bytes   Format: [a:b:c:d:e:f:g:h]:port
        a                      uint16_t
        b                      uint16_t
        c                      uint16_t
        d                      uint16_t
        e                      uint16_t
        f                      uint16_t
        g                      uint16_t
        h                      uint16_t
        port                   uint16_t
    <end if>
<end for>
<zeroes padded to 624 bytes>              Counting from the beginning of the PUBLIC SECTION.
---  END PUBLIC SECTION  ---
--  BEGIN SECRET SECTION  --
encryption signature           64 bytes   Optional storage to make certain APIs easier to use -- clear to zero if unused.
client id                      uint64_t   Unique identifier for a particular client.
client to server key           32 bytes   Client uses to encrypt packets, server uses to decrypt packets.
server to client key           32 bytes   Server uses to encrypt packets, client uses to decrypt packets.
user data                      256 bytes  Space for the user to store whatever auxiliary data they need.
---  END SECRET SECTION  ---
signature                      64 bytes
```

The *connect token packet* is sent as-is without modification to a game server in the list. All game servers are valid choices, but it is recommended to start by making an attempt to connect to the first server in the list. If a failure occurs (for example the server is full), the client can move onto the next server in the list. Storing only a single server in the list is acceptable, but it is recommended to have more than one if possible, to improve the chances a client can successfully connect to a game server and play.

The server list is included in the *connect token packet*, instead of the REST SECTION, in order to mitigate a malicious client from spawning a connection with all possible dedicated servers simultaneously with a single connect token. More information is found in the [Protection Against Various Attacks](#protection-against-various-attacks) section.

Once the client sends the *connect token packet* to a game server, the connection handshake begins.

The `application id` is a value the user chooses to uniquely represent their application.

##### Note:
> During development, for insecure connections, the connect token can be generated by the client as long as it knows the secret key of the dedicated game servers.

##### Note:
> The Unix timestamps can, for example, be created with something similar to `(uint64_t)time(NULL)` in the C language.

## Connection Handshake

The connection handshake begins once a client starts sending a *connect token packet* to a game server. The client runs a state machine to fulfill the handshake. Once the state machine completes successfully, the connection is complete and both the client and game server are free to send *payload packet*'s to each other, containing game-specific data.

##### Note:
> There is a description of all packet types and their formats later in this document in the [Packet Formats](#packet-formats) section.

## Client Handshake State Machine

The various client states.

State Name | Value
--- | ---
*connect token expired* | -6
*invalid connect token* | -5
*connection timed out* | -4
*challenge response timed out* | -3
*connection request timed out* | -2
*connection denied* | -1
*disconnected* | 0
*sending connection request* | 1
*sending challenge response* | 2
*connected* | 3

Each state only accepts a subset of packet types, and all extraneous packets received are ignored.

### Disconnected

The default state is *disconnected*. The client transitions to the *sending connection request* state once they receive a valid connect token from the web service and have recorded data from the PUBLIC SECTION of the connect token.

Exactly how the client requests and receives the connect token is out of scope of this document, besides requiring a secure REST API call. Exactly how the web service generates the server list of the connect token is also out of scope of this document, as this is the realm of match-making or other similar mechanisms.

The client reads the PUBLIC SECTION and the REST SECTION of the connect token and records all the data. If the data is invalid the client transitions to the *invalid connect token* state. The connect token is deemed invalid if the number of server addresses is outside the range of [1, 32], or if an IP address type is not in the range [1, 2], or if the creation timestamp is more recent than the expiration timestamp.

### Sending Connection Request

The client repeatedly sends the *connect token packet* at PACKET_SEND_FREQUENCY (see [Tuning Parameters](#tuning-parameters)), to a game server from the list and waits for a response. This packet is **not** encrypted by the client, as it is already processed by the AEAD primitive by the web service. If the server responds with a *challenge request packet*, the client transitions to the *sending challenge response*, after decrypting and recording the contents of the *challenge request packet*.

It is recommended the client moves onto the next server in the server list in the event of failure cases. If no more servers are left in the list, the client transitions to one of the error states.

If the last server in the list responds with a *connection denied packet*, the client transitions to the *connection denied* state. If the last server in the server does not send a *challenge request packet* or a *connection denied packet*, the client transitions to the *connection request timed out* state.

### Sending Challenge Response

During the *sending connection request* the client stored the contents of *challenge request packet*. The contents are used to form the *challenge response packet*. The client repeatedly sends the server the *challenge response packet* at the PACKET_SEND_FREQUENCY (see [Tuning Parameters](#tuning-parameters)). The purpose is to verify a few things about the client:

1. That the client is not spoofing their IP address, and can successfully respond to the server by reflecting the *challenge request packet* with a *challenge response packet*.
2. That the client can decrypt the *challenge request packet* and encrypt the *challenge response packet*. This asserts that the client was the **only** user the web service gave the original connect token to.

If successful, the server will respond with a *connection accepted packet*, and the client transitions to the *connected* state after recording data from the *connection accepted packet*. The client records the *client id* field, the *max clients* field, and the `connection timeout` field from the *connection accepted packet*.

It is recommended the client moves onto the next server in the server list in the event of failure cases. If no more servers are left in the list, the client transitions to one of the error states.

If the last server in the list responds with a *connection denied packet*, the client transitions to the *connection denied* state. If the last server in the server does not send a *connection accepted packet* or a *connection denied packet*, the client transitions to the *challenge response timed out* state.

### Connected

The purpose of the connected state is to allow the user to send and receive *payload packet*'s containing game-specific data. In the absence of any payload packets, the client generates and sends a *keepalive packet* at the KEEPALIVE_FREQUENCY (see [Tuning Parameters](#tuning-parameters)).

If no *payload packet*'s or *keepalive packet*'s are received from the server within a `connection timeout` timespan, the client transitions to the *connection timed out* state.

If the client receives a *disconnect packet* from the server it transitions to the *disconnected* state after performing the [Disconnect Sequence](#disconnect-sequence).

### Connect Token Expired

If at any time the client's connect token expires during the handshake process, the client transitions to the *connect token expired*. This can happen if the client must try connecting to multiple servers in the server list, and continually fails to establish a connection.

The time to live for the connect token is calculated as:

    expiration timestamp - creation timestamp

## Packet Formats

Packet Type | Packet Type Value
--- | ---
*connect token packet* | 0
*keepalive packet* | 1
*connection denied packet* | 2
*payload packet* | 3
*connection accepted packet* | 4
*challenge request packet* | 5
*challenge response packet* | 6
*disconnect packet* | 7

The Packet Type Value is used to write the `packet type` byte as described in the next section.

## Encrypted Packets

All packets except for the *connect token packet* are encrypted before sent. Each encrypted packet has the following form.

```
packet type       1 byte
sequence nonce    8 bytes
signature         64 bytes
encrypted bytes   <variable length>
```

All packets are 1280 bytes or smaller, so the maximum size of the `encrypted bytes` is 1207 bytes.

The *connect token packet* does not get encrypted by the client before being sent, as it is already encrypted by the web service using a secret key only known to the web service and the backend dedicated game servers. This means the client cannot read, modify, or generate the SECRET SECTION of the *connect token packet*.

## Unencrypted Packets

This section describes the payload of each packet type prior to encryption.

### Keepalive Packet

```
<no data>           0 bytes
```

The *keepalive packet* is only sent if no *keepalive packet*'s, *disconnect packet*'s, or *payload packet*'s have been sent within KEEPALIVE_FREQUENCY seconds during the client *connected* state. If none of these packets are received by either the client or the server, the connection is terminated after `connection timeout` seconds.

### Connection Denied Packet

```
<no data>           0 bytes
```

The *connection denied packet* can be sent by the server during the connection handshake. It is sent whenever an error occurs (like no room on the server, or received packets fail to decrypt), or if the client fails to comply with the proper handshake process.

### Payload Packet

```
payload size        uint16_t
payload data        In the range of [1, 1207] bytes.
```

The *payload packet* can be sent by the client or the server during the *connected* state of the client. They contain game specific user data.

### Connection Accepted Packet

```
client handle       uint64_t
max clients         uint32_t
connection timeout  uint32_t
```

The *connection accepted packet* is sent from the server once a client has successfully connected to the server. It is repeatedly sent before each *payload packet* as long as the client is not *confirmed*. More details can be found in the [Connected and Confirmed Clients](#connected-and-confirmed-clients) section.

### Challenge Request Packet

```
challenge nonce     uint64_t
client id           uint64_t
challenge bytes     256 bytes
```

The *challenge request packet* is sent from the server as a part of the connection handshake process. This challenge response sequence is used to prevent IP spoofing and *connect token packet* sniffing. For more information about how, see the [Sending Challenge Response](#sending-challenge-response) section.

The `challenge nonce` is an incrementing counter starting at 0, initialized upon server restart. Increment the nonce once after each challenge packet is created.

The `challenge bytes` are simply 256 bytes of data. The data can be anything, including randomized bits. Exactly what bits are the `challenge bytes` is left to the implementation. To fulfill the purpose of this packet, the client merely needs to decrypt the packet with the `server to client` key, encrypt it with the `client to server key`, and send it back to the server. The contents of the `challenge bytes` are ignored by the client. This packet is intentionally smaller than the *connect token packet* to prevent [DDoS amplification](https://en.wikipedia.org/wiki/Denial-of-service_attack#Amplification).

### Challenge Response Packet

```
challenge nonce     uint64_t
challenge bytes     256 bytes
```

The *challenge response packet* is the reflected version of the *challenge request packet* and is sent by the client as a part of the connection handshake process. All the client must do is copy the entire decrypted *challenge request packet*, encrypt it, and send it back to the server for verification. For more information see the [Sending Challenge Response](#sending-challenge-response), or the [Challenge Request Packet](#challenge-request-packet) section.

### Disconnect Packet

```
<no data>           0 bytes
```

The *disconnect packet* can be sent by the client or the server during the client *connected* state. Once sent, the connection is considered terminated. Once received, the connection is also considered terminated. Sending the *disconnect packet* must be done by the [Disconnect Sequence](#disconnect-sequence) in order to statistically assure clean disconnects, without waiting for unnecessary timeouts.

## Decrypting Packets

When decrypting packets the following steps must occur, in order, before a packet can be considered valid for further processing (NOTE - further processing includes updating any replay buffer implementation you might have).

1. If an incoming packet is less than 45 bytes, ignore the packet.
2. If the `packet type` byte is greater than 7, ignore the packet.
3. The server ignores packets of types *challenge response packet*, *connection denied packet*, and *connection accepted packet*.
4. The client ignores packets of types *challenge request packet* and *connect token packet*.
5. If the packet's `encrypted bytes` are not within acceptable range, as defined by the packet type, ignore the packet. A table of acceptable sizes is given just below the end of these 7 steps.
6. If the packet fails replay protection, ignore the packet. See the [Replay Protection](#replay-protection) section for more info.
7. If the packet fails to decrypt with the AEAD primitive, ignore the packet.

Packet Type | `encrypted bytes` Size
--- | ---
*connect token packet* | 960 bytes
*keepalive packet* | 0 bytes
*connection denied packet* | 0 bytes
*payload packet* | In the range of [1-1207] bytes
*connection accepted packet* | 16 bytes
*challenge request packet* | 264 bytes
*challenge response packet* | 264 bytes
*disconnect packet* | 0 bytes

## Server Handshake and Connection Process

The server should be on a publicly available IP address and port. This way clients can initiate the connection handshake by sending the *connect token packet* to the server, who is listening on a UDP socket + port combo.

The server maintains a set of clients, capped at SERVER_MAX_CLIENTS tunable (see [Tuning Parameters](#tuning-parameters) for more info). Each client is represented by an opaque *client handle*. Max clients can be set to any number as deemed acceptable according to the implementation. For many first-person shooter games, somewhere from 8-32 players often makes sense. For larger games, such as MMOs, multiple thousands of players can be acceptable for the Cute Protocol, so long as the implementation appropriately scales to that level. The Cute Protocol is quite agnostic to connection scale, since different implementations are free to represent clients in a manner suitable to their specific needs.

The servers goals are:
* Only respond when absolutely necessary.
* Early out packet validation as quickly as possible (for example, perform any available checks pre-decryption on the *connect token packet*).
* Only allow clients with valid connect tokens to instantiate a connection.
* **Never** write any sensitive data based on the contents of any packet before the decryption check succeeds. Data pre-decryption should be thought of as dangerous, and never be used as a predicate to mutate sensitive server state.
* Always respond during the handshake process with much smaller packet sizes than were initially sent, in order to stay far away from being available for a [DDoS amplification attack](https://en.wikipedia.org/wiki/Denial-of-service_attack#Amplification).

### Server Handshake Process

The server runs a state machine for each potential client attempting to get through the handshake process. Instead of writing down the state machine formally, as was done for the [Client Handshake State Machine](#client-handshake-state-machine) section, the steps are listed here in order for simplicity. The server follows exactly these steps, in order, once a *connect token packet* is received. These are the steps for setting up a secure connection with a client in the *connected* state.

#### Processing the *connect token packet*

Here are the steps for processing the *connect token packet*.

1. A *connect token packet* is received on the listener UDP socket and port. This means a potential client wants to start the connection handshake process.
2. If the *connect token packet* is not exactly 1024 bytes, ignore the packet.
3. If the `packet type` byte at the beginning of the packet is not zero (*connect token packet*), ignore the packet.
4. Read and make sure the protocol string `version info` "CUTE 1.00", including the nul byte, comes right after the `packet type` byte.
5. Read and make sure the `application id` matches the expected id for the user's application.
6. Read the `expiration timestamp`. If the token has expired, ignore the packet.
7. The connect token is deemed invalid if the number of server addresses is outside the range of [1, 32], or if an IP address type is not in the range [1, 2], or if the creation timestamp is more recent than the expiration timestamp. If any of these checks fail, ignore the packet.
8. Decrypt and read the *connect token packet* SECRET SECTION. Please remember that the PUBLIC SECTION must be used within the Associated Data in the AEAD primitive. If the *connect token packet* fails to decrypt, ignore the packet. The HMAC and nonce for your AEAD primitive are stored within the `signature`.

#### Setting Up an *encryption state*

The next series of steps is for setting up an *encryption state* with the potential client. An *encryption state* must contain the following information from the *connect token packet*:

* `sequence nonce`
* `expiration timestamp`
* `handshake timeout`
* `client to server key`
* `server to client key`

The `client to server key` and `server to client key` are used to perform encrypted communication with the potential client, and come from the SECRET SECTION of the *connect token packet*. The `handshake timeout` is used to time out the handshake process in the event the client takes too long to respond at any stage. `expiration timestamp` is when the associated connect token expires.

1. If the server is not in the list of IP addresses in the *connect token packet*, ignore the packet.
2. If a client is already connected with the same IP address and port, ignore the packet.
3. If a matching `client id` already connected, ignore the packet.
4. Lookup in the *connect token cache* with the *connect token packet* `signature` as the key. If it is in the cache this means the token was already used; ignore the packet.
	* The *connect token cache* is a [set data structure](https://en.wikipedia.org/wiki/Set_(abstract_data_type)) where the `signature` of the *connect token packet* is used as the unique values for keys. It is recommended to use a rolling cache to automatically evict old connect token entries that will have likely already expired.
5. If the server is full, respond with a *connection denied packet*.
6. Setup an *encryption state* with the client, keyed by the client IP address and port.

The encryption state simply maps a client's IP address and port to the state stored within the *encryption state* (as described above). Exactly how this data is stored and with what data structure is left up to the implementation. It is recommended to allow more *encryption state*'s than the maximum capacity of clients, in order to effectively handle invalid connection attempts along with valid connection attempts gracefully.

The *encryption state* should be deleted or recycled whenever a connection or handshake terminates. Once a handshake completes successfully and is promoted to a connection, the associated *encryption state* no longer needs to periodically check the `expiration timestamp`.

#### Challenge Request and Response Sequence

Once the connect token has been validated, and the encryption state is successfully setup, the next steps are to complete the challenge request and response sequence with the client. The purpose of these steps is to prevent IP spoofing, and to prevent *connect token packet* sniffing. From here on all packets are encrypted or decrypted with the *encryption state*.

1. Send the client a *challenge request packet* periodically through the PACKET_SEND_FREQUENCY tunable (see [Tuning Parameters](#tuning-parameters)). Before sending the *challenge request packet*, fill in the `challenge bytes` with a unique bit pattern (optionaL and remember the bit pattern for later). Use the `sequence nonce` from the *encryption state* to fill in the `sequence nonce` of the *challenge request packet*.
2. If a *challenge response packet* is received, first read in the `sequence nonce` and try decrypting the packet. If decryption fails, ignore the packet.
3. This step is optional. Test to make sure the bit pattern post-decryption matches the bit pattern sent in the *challenge request packet*.
4. If a client is already connected with the same IP address and port, ignore the packet.
5. If a matching `client id` already connected, ignore the packet.
6. If all checks passed, make sure there is still space available for the client to connect. If the server is full, respond with a *connection denied packet* and terminate the handshake.
7. The client is now considered *connected*, but not *confirmed*. Insert the `signature` from the *encryption state* into the *connect token cache*. Increment the `sequence nonce` of the *encryption state*. Construct a new client entry. Periodically send the client the *connection accepted packet* with the data referencing the newly created client entry. Send the *connection accepted packet* at the rate of PACKET_SEND_FREQUENCY.
8. Once the client responds with a *payload packet*, or a *keepalive packet*, consider the client *confirmed*.
9. Once a client is *connected* the server may start sending *payload packet*'s. If the client is not yet confirmed, the server **must** send an additional *connection accepted packet* just before sending a *payload packet*. Once the client is *confirmed* preceding *connection accepted packet*'s are no longer necessary. The purpose of extra *connection accepted packet*'s is an optimization: the server can start streaming payload packets earlier, but also ensures the client receives a *connection accepted packet*.

### Connected and Confirmed Clients

Once a client is *connected* (even if they are not yet *confirmed*) they are assigned a unique 64-bit identifier. This identifier is used to fill in the *connection accepted packet*'s `client handle`. Incoming packets for a client's IP address and port are mapped to the associated `client handle`. Once *connected*, the server and client can send the following packet types.

* *keepalive packet*
* *payload packet*
* *disconnect packet*

It is recommended that incoming packets are pulled off the UDP stack as fast as possible, especially as the number of maximum clients becomes higher. It is best to pull packets off the UDP stack from a dedicated thread, whose sole purpose is to move packets from the UDP stack to an internal queue. The queue can then be polled by the user's calling thread on an as-needed basis. It is also recommended to cap the size of this queue, and drop packets once filled. Optionally the payload packet can be utilized to implement a *backpressure packet*, which is intended to inform the opposing endpoint to send data at a slower rate.

If no packets are sent to a client within the KEEPALIVE_FREQUENCY to the client, send a *keepalive packet*.

## Disconnect Sequence

In order to gracefully disconnect, either the client or the server can perform the Disconnect Sequence, which means to fire off a series of *disconnect packet*'s in quick succession (e.g. in a for loop). The number of packets is defined by the DISCONNECT_SEQUENCE_PACKET_COUNT tuning parameter (see [Tuning Parameters](#tuning-parameters)). The purpose of the redundancy is to be statistically likely that one of the packets gets through to the endpoint, even in the face of packet loss.

## Protection Against Various Attacks

Cute Protocol takes measures to defend itself against many common attacks. This list is not meant to be exhaustive, but instead is included for posterity.

### Replay Protection

Replay protection is to guard against [replay attacks](https://en.wikipedia.org/wiki/Replay_attack). Cute Protocol uses incrementing sequence numbers as the nonce for the AEAD primitive to guard against replay attacks. For all encrypted packets, and the *connect token packet*, the `signature` bytes should be used to store the nonce.

Replay protection is enabled for the *disconnect packet*, *keepalive packet*, and *payload packet*. All other packet types are already protected by replay attacks by other means (like the connect token handling process, or replay attacks simply do nothing in other cases and are safely ignored).

Here is an _example_ replay protection algorithm. The replay algorithm uses an array of `uint64_t` elements called the *replay buffer*. The size of the *replay buffer* is tunable by REPLAY_BUFFER_SIZE tunable (see [Tuning Parameters](#tuning-parameters)). Here is the replay protection algorithm.

1. All encrypted packets are prefixed with a `uint64_t` sequence number, starting at zero and incrementing, and resides within the `signature` bytes. This sequence number is the `sequence nonce` stored within the associated *encryption state*. In this way, there is a different incrementing counter and encryption key-set for each connection.
2. The sequence number of a received packet is read prior to decryption. It cannot be modified without detection by the AEAD primitive since it is used as a nonce in the AEAD.
3. The maximum sequence number is tracked, called *max sequence*.
4. If the sequence number + REPLAY_BUFFER_SIZE is less than *max sequence*, ignore the packet. This means either the packet is very old, and should be dropped (since this UDP), or it was an attempted replay attack/duplicated packet.
5. If the sequence number has already been received, as determined by a lookup into the *replay buffer*, ignore the packet as it is an attempted replay attack/duplicated packet.
6. After a successful decryption, update *max sequence* and set it to the sequence number if the sequence number is larger than *max sequence*. Be careful and **do not perform this step before decryption successfully completes**. Updating the max sequence is a sensitive operation, and can only occur if the sequence number is validated by the decryption AEAD primitive.

Here is an example implementation of the replay protection buffer and culling algorithm written in C.

```c
#define REPLAY_BUFFER_SIZE

struct replay_buffer_t
{
    uint64_t max;
    uint64_t entries[REPLAY_BUFFER_SIZE];
};

void replay_buffer_init(replay_buffer_t* buffer)
{
    buffer->max = 0;
    memset(buffer->entries, ~0, sizeof(uint64_t) * REPLAY_BUFFER_SIZE);
}

int replay_buffer_cull_duplicate(replay_buffer_t* buffer, uint64_t sequence)
{
    if (sequence + REPLAY_BUFFER_SIZE < buffer->max) {
        // This is UDP - just drop old packets.
        return -1;
    }

    int index = (int)(sequence % REPLAY_BUFFER_SIZE);
    uint64_t val = buffer->entries[index];
    int empty_slot = val == ~0ULL;
    int outdated = val >= sequence;
    if (empty_slot | !outdated) {
        return 0;
    } else {
        // Duplicate or replayed packet detected.
        return -1;
    }
}

void replay_buffer_update(replay_buffer_t* buffer, uint64_t sequence)
{
    if (buffer->max < sequence) {
        buffer->max = sequence;
    }

    int index = (int)(sequence % REPLAY_BUFFER_SIZE);
    uint64_t val = buffer->entries[index];
    int empty_slot = val == ~0ULL;
    int outdated = val >= sequence;
    if (empty_slot | !outdated) {
        buffer->entries[index] = sequence;
    }
}
```

Here is some an example use-case scenario for using replay protection on an incoming packet, written in C.


```c
int read_packet(
    uint8_t* packet,
    int size,
    replay_buffer_t*
    replay_buffer,
    /* ... other params ... */
)
{
    // ...

    // Perform replay protection
    uint8_t packet_type = read_uint8(&packet);
    uint8_t sequence = read_uint64(&packet);
    if (replay_buffer_cull_duplicate(replay_buffer, sequence) {
        return -1;
    }

    // Continue on with decryption steps ...

    replay_buffer_update(replay_buffer, sequence);

    // Continue with any other optional processing ...
}
```

### Packet Sniffing

One may wonder if the *connect token packet* is susceptible to packet sniffing. What if someone grabs the packet and sends a copy to the server before the valid client's copy reaches the server? In this case the server will start up two *encryption state*'s, one for each potential client. The server will cache the *connect token packet* keyed by the `signature` of the packet. This will ensure only one copy of the connect token is cached.

The server will also setup two *encryption state*'s, one for each IP address. In the case where the malicious potential client spoofed their IP to match the valid user's IP, only one *encryption state* will be setup.

The server then sends out the *challenge request packet*, which in the case of no IP spoofing will go to both potential clients. Since the valid client received the initial connect token and read the REST SECTION, it knows the `client to server key` and the `server to client key` that the *encryption state* is using, and be able to properly respond with a well-formed and valid *challenge response packet*. The malicious client never had access to the encryption keys, and will not be able to respond with a valid packet. The server will ignore all packets that fail to decrypt from the malicious client.

In the case of IP spoofing, only the valid client will receive a *challenge request packet*.

After a connection is established the client and server continue to use the *encryption state* setup during the handshake process, and the shared secret keyset prevents anyone from being able to read, modify, or forge any packet data.

### Malicious Client Connection Spam and Zombie Clients

A valid but malicious client may attempt to use a valid connect token to connect to multiple different servers, and may attempt to spoof their IP address to try and establish multiple connections to one server to spinup "zombie clients" (invalid clients with valid connections that do nothing but wait to timeout).

The first attack is prevented by the server list within the connect token. This list is protected by the AEAD primitive since it is used as Associated Data. This means that nobody but the web service and the dedicated servers can modify or forge the connect token, or read the SECRET SECTION of the connect token. The strategy is to only include *some* of the available servers in this list, this way if a client tries to connect to all servers, only some of them will accept the token (the ones it he list).

The second attack is prevented by the challenge request and response packets in the handshake process.

### Man in the Middle

Protection against [Man in the Middle](https://en.wikipedia.org/wiki/Man-in-the-middle_attack) (MitM) attacks is quite straightforward for the Cute Protocol. The web service and the dedicated backend servers all hold a shared secret. This secret is the key used to encrypt and decrypt connect tokens. This key is never exposed to clients, and in this way a MitM can not read, modify or forge the SECRET SECTION of the connect token without somehow getting a hold of the shared secret key. Additionally, the PUBLIC SECTION of the connect token is protected by the AEAD since it is used as Associated Data.

A MitM can potentially get a hold of connect tokens but is unable to do anything with them. If an attempt is made to use a valid connect token to join a game server, the challenge and response sequence prevents them from connecting since they do not have access to the `client to server key`, or the `server to client key`. These keys are uniquely generated by the web service for each connect token, and are given only to valid authenticated clients via the REST SECTION of the connect token, over a secure REST API call (like HTTPS, which uses the TLS/SSL protocol to setup a secure TCP tunnel). This means every connection a client makes to a dedicated server uses a shared secret key-set between the client and the server, making it impossible for a MitM to read, modify, or forge any encrypted packets.

### DDoS Amplification

UDP protocols are susceptible to [DDoS amplification](https://en.wikipedia.org/wiki/Denial-of-service_attack#Amplification) attacks if they are not designed with mitigations in mind. The Cute Protocol is a terrible candidate for DDoS amplification since the handshake process **always** has the server respond with a significantly smaller packet than was received by potential clients. The servers ignore any packets that do not strictly abide by size requirements. In this way, the Cute Protocol servers can only minimize UDP traffic, which means the protocol is not prone to DDoS amplification.

## Tuning Parameters

* KEEPALIVE_FREQUENCY
	* Value in seconds of when to send a *keepalive packet* during a connection when no payload packets are sent.
* DISCONNECT_SEQUENCE_PACKET_COUNT
	* The number of packets to redundantly send upon the Disconnect Sequence.
* REPLAY_BUFFER_SIZE
	* Number of recorded sequence numbers into the past to keep around in a rolling cache. Typically, 5-10 seconds can work well, depending on latency concerns and application context.
* SERVER_MAX_CLIENTS
	* The capacity of the server in terms of how many different simultaneous *connected* clients can be held at once.
* PACKET_SEND_FREQUENCY
	* Rate to send packets, in seconds, during the connection handshake process.
