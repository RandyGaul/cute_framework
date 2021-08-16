# server_send_to_all_clients

Sends a packet to all client.

## Syntax

```cpp
void server_send(server_t* server, const void* packet, int size, bool send_reliably);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
packet | Pointer to the packet data.
size | Size of the data in bytes at the `packet` pointer.
send_reliably | If true the packet will be sent reliably and in order. False means a typical UDP packet will be sent, and if lost nothing special is done - it will be lost forever, can arrive late, out of order, or even duplicated.

## Related Functions

[server_pop_event](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_pop_event.md)  
[server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send.md)  
[server_send_to_all_but_one_client](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send_to_all_but_one_client.md)  
[server_is_client_connected](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_is_client_connected.md)  
