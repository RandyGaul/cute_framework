# server_create

Creates an instance of a server.

## Syntax

```cpp
server_t* server_create(server_config_t* config, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
config | A `server_config_t` containing all server parameters (see below).
user_allocator_context | Optional pointer to a custom allocator, can be set to `NULL`.

## Return Value

A new server instance.

## Remarks

The `server_config_t` must be filled out to create a server instance. It contains the server settings.

Member Variable Name | Type | Description
--- | --- | ---
application_id | `uint64_t` | A unique number to identify your game, can be whatever value you like. This must be the same number as in [client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md).
connection_timeout | `int` | The number of seconds before consider a connection as timed out when not receiving any packets on the connection.
resend_rate | `double` | The number of seconds to wait before resending a packet that has not been acknowledge as received by a client.
public_key | `crypto_sign_public_t` | The public part of your public key cryptography used for [connect tokens](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/). This can be safely shared with your players publicly.
secret_key | `crypto_sign_secret_t` | The secret part of your public key cryptography used for [connect tokens](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/). This must *never* be shared publicly and remain a complete secret only know to your servers.

## Related Functions

[server_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_destroy.md)  
[server_start](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_start.md)  
[server_stop](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_stop.md)  
[server_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_update.md)  
