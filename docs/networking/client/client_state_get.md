# client_state_get

Returns the state of the client.

## Syntax

```cpp
client_state_t client_state_get(const client_t* client);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client.

## Return Value

The state of the client as one of the `client_state_t` enumerations.

Enumeration Entry | Description
--- | ---
CLIENT_STATE_CONNECT_TOKEN_EXPIRED | The connect token itself, set when calling [client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md), has outlived its expiration date.
CLIENT_STATE_INVALID_CONNECT_TOKEN | The connect token provided is somehow invalid, such as failing a security check.
CLIENT_STATE_CONNECTION_TIMED_OUT | No responses from the server arrived during the handshake timeout window.
CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT | No responses from the server arrived during the handshake's challenge/response phase.
CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT | No valid connections were for any servers listed in the connect token.
CLIENT_STATE_CONNECTION_DENIED | A server rejected the connection request.
CLIENT_STATE_DISCONNECTED | The client is not connected to any server, nor attempting to connect.
CLIENT_STATE_SENDING_CONNECTION_REQUEST | The client has initiated a connection handshake with a server.
CLIENT_STATE_SENDING_CHALLENGE_RESPONSE | The client is in the middle of the handshake process with a server.
CLIENT_STATE_CONNECTED | The client has connected successfully to a server, and is ready to send and receive packets.

## Remarks

Use this function to detect when the state of a client has changed.

## Related Functions

[client_connect](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_connect.md)  
[client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_update.md)  
[client_state_string](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_state_string.md)  
