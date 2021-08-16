# client_make

Creates a client in an unconnected state. Once connected, a client can send and receive packets from a game server.

## Syntax

```cpp
client_t* client_make(uint16_t port, uint64_t application_id, bool use_ipv6 = false, void* user_allocator_context = NULL);
```

## Function Parameters

Parameter Name | Description
--- | ---
port | Port for opening a UDP socket.
application_id | A unique number to identify your game, can be whatever value you like.
use_ipv6 | Whether or not the socket should turn on ipv6. Some users will not have ipv6 enabled, so this defaults to false.
user_allocator_context | Used for custom allocators, this can be set to `NULL`.

## Remarks

This function may only be called if the application has audio initialized without a mixing thread. See [app_init_audio](https://github.com/RandyGaul/cute_framework/blob/master/docs/app/app_init_audio.md) for more details.

## Return Value

Returns a pointer to a `client_t` instance. Once disconnected call [client_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_destroy.md) to clean it up. To disconnect gracefully call [client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md).

## Related Functions

[client_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_destroy.md)  
[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)  
[client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md)  
[client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md)  
