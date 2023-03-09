[](../header.md ':include')

# cf_generate_connect_token

Category: [net](/api_reference?id=net)  
GitHub: [cute_networking.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_networking.h)  
---

Generates a connect token, useable by clients to authenticate and securely connect to a server.

```cpp
CF_Result cf_generate_connect_token(uint64_t application_id, uint64_t creation_timestamp, const CF_CryptoKey* client_to_server_key, const CF_CryptoKey* server_to_client_key, uint64_t expiration_timestamp, uint32_t handshake_timeout, int address_count, const char** address_list, uint64_t client_id, const uint8_t* user_data, const CF_CryptoSignSecret* shared_secret_key, uint8_t* token_ptr_out);
```

Parameters | Description
--- | ---
application_id | A unique number to identify your game, can be whatever value you like.
                      This must be the same number as in [cf_make_client](/net/cf_make_client.md) and [cf_make_server](/net/cf_make_server.md).
creation_timestamp | A unix timestamp of the current time.
client_to_server_key | A unique key for this connect token for the client to encrypt packets, and server to
                      decrypt packets. This can be generated with [cf_crypto_generate_key](/net/cf_crypto_generate_key.md) on your web service.
server_to_client_key | A unique key for this connect token for the server to encrypt packets, and the client to
                      decrypt packets. This can be generated with [cf_crypto_generate_key](/net/cf_crypto_generate_key.md) on your web service.
expiration_timestamp | A unix timestamp for when this connect token expires and becomes invalid.
handshake_timeout | The number of seconds the connection will stay alive during the handshake process before
                      the client and server reject the handshake process as failed.
address_count | Must be from 1 to 32 (inclusive). The number of addresses in `address_list`.
address_list | A list of game servers the client can try connecting to, of length `address_count`.
client_id | The unique client identifier (you pick this).
user_data | Can be `NULL`. Optional buffer of data of `CUTE_PROTOCOL_CONNECT_TOKEN_USER_DATA_SIZE` (256) bytes.
shared_secret_key | Only your webservice and game servers know this key.
token_ptr_out | Pointer to your buffer, should be [CUTE_CONNECT_TOKEN_SIZE](/net/cute_connect_token_size.md) bytes large.

## Return Value

Returns any errors as [CF_Result](/utility/cf_result.md).

## Remarks

You can use this function whenever a validated client wants to join your game servers.

It's recommended to setup a web service specifically for allowing players to authenticate
themselves (login). Once authenticated, the webservice can call this function and hand
the connect token to the client. The client can then read the public section of the
connect token and see the `address_list` of servers to try and connect to. The client then
sends the connect token to one of these servers to start the connection handshake. If the
handshake completes successfully, the client will connect to the server.

The connect token is protected by an AEAD primitive (https://en.wikipedia.org/wiki/Authenticated_encryption),
which means the token cannot be modified or forged as long as the `cf_shared_secret_key` is
not leaked. In the event your secret key is accidentally leaked, you can always roll a
new one and distribute it to your webservice and game servers.

## Related Pages

[CF_CryptoKey](/net/cf_cryptokey.md)  
[cf_crypto_generate_key](/net/cf_crypto_generate_key.md)  
[cf_client_connect](/net/cf_client_connect.md)  
