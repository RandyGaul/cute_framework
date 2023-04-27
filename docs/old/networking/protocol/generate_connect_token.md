# generate_connect_token

Generates a connect token.

## Syntax

```cpp
CUTE_API error_t CUTE_CALL generate_connect_token(
	uint64_t application_id,
	uint64_t creation_timestamp,
	const crypto_key_t* client_to_server_key,
	const crypto_key_t* server_to_client_key,
	uint64_t expiration_timestamp,
	uint32_t handshake_timeout,
	int endpoint_count,
	const char** endpoint_list,
	uint64_t client_id,
	const uint8_t* user_data,
	const crypto_sign_secret_t* shared_secret_key,
	uint8_t* token_ptr_out
);
```

## Function Parameters

Parameter Name | Description
--- | ---
application_id | A unique number to identify your game, can be whatever value you like. This must be the same number as in [client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md). and [server_create](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_create.md).
creation_timestamp | A [unix timestamp](https://en.wikipedia.org/wiki/Unix_time) of the current time.
client_to_server_key | A unique key for this connect token for the client to encrypt packets, and server to decrypt packets. This can be generated with `crypto_generate_key` (TODO link here) on your web service.
server_to_client_key | A unique key for this connect token for the server to encrypt packets, and the client to decrypt packets. This can be generated with `crypto_generate_key` (TODO link here) on your web service.
expiration_timestamp | A [unix timestamp](https://en.wikipedia.org/wiki/Unix_time) for when this connect token expires and becomes invalid.
handshake_timeout | The number of seconds the connection will stay alive during the handshake process before the client and server reject the handshake process as failed.
endpoint_count | The number of endpoints in `endpoint_list`, the next parameter.
endpoint_list | An array of strings containins endpoint addresses and ports. These endpoints are the list of game servers clients will try connecting to.
client_id | The unique client identifier.
user_data | Optional space for any kind of data you like, of size `CUTE_CONNECT_TOKEN_USER_DATA_SIZE`, which is 256 bytes.
shared_secret_key | The secret key shared between the web service and dedicated servers. This must *never* be shared publicly and kept a complete secret.
token_ptr_out | Where the connect token will be written to, must be a buffer of size `CUTE_CONNECT_TOKEN_SIZE`.

## Return Value

Returns any errors upon failure.

## Remarks

This function should be called within your web service. For more details see the [networking section](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking) of the docs.

## Related Functions

[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)  
