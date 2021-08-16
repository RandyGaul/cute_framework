# client_destroy

Destroys a client instance.

## Syntax

```cpp
void client_destroy(client_t* client);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client to destroy.
application_id | A unique number to identify your game, can be whatever value you like.
use_ipv6 | Whether or not the socket should turn on ipv6. Some users will not have ipv6 enabled, so this defaults to false.
user_allocator_context | Used for custom allocators, this can be set to `NULL`.

## Remarks

It's recommended to destroy a client when in a disconnected state. To disconnect gracefully call [client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md).

## Related Functions

[client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md)  
[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)  
[client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md)  
[client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md)  
