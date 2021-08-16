# server_free_packet

Frees a packet received from an earlier call to [server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/event/server_pop_event.md) from a payload event.

## Syntax

```cpp
void server_free_packet(server_t* server, int client_index, void* data);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
client_index | The index of the client the packet came from.
data | The packet data.

## Remarks

Packets come from the [server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/event/server_pop_event.md) function, and are freed by calling `server_free_packet`.

## Related Functions

[server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_pop_event.md)  
[server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send.md)  
