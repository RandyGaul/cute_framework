# client_connect

Attempts to connect a client to a dedicated game server via [Connect Token](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/).

## Syntax

```cpp
error_t client_connect(client_t* client, const uint8_t* connect_token);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client to connect.
connect_token | A unique connect token returned from your web service. Connect tokens can be created on your web service by calling [generate_connect_token](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/protocol/generate_connect_token.md).

## Return Value

A number of different errors can occur when attempting to connect; it's recommended to check the return value here carefully when debugging.

## Remarks

The client will make an attempt to connect to all servers listed in the connect token, one after another. If no server can be connected to the client's state will be set to an error state. Call [client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md) to get the client's state. Once `client_connect` is called then successive calls to [client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_update.md) is expected, where `client_update` will perform the connection handshake and make connection attempts to your servers.

## Related Functions

[client_make](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_make.md)  
[client_destroy](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_destroy.md)  
[client_disconnect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_disconnect.md)  
[client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_update.md)  
[client_state_get](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_get.md)  
