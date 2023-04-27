# client_send

Sends a packet to the server.

## Syntax

```cpp
error_t client_send(client_t* client, const void* packet, int size, bool send_reliably);
```

## Function Parameters

Parameter Name | Description
--- | ---
client | The client.
packet | A packet to send to the server.
size | Size of the packet in bytes.
send_reliably | True means the packet will be sent reliably an in-order relative to other reliable packets. Under packet loss the packet will continually be sent until an acknowledgement from the server is received. False means to send a typical UDP packet, with no special mechanisms regarding packet loss.

## Return Value

Returns error upon failure.

## Remarks

Reliable packets are significantly more expensive than unreliable packets, so try to send any data that can be lost due to packet loss as an unreliable packet. Of course, some packets are required to be sent, and so reliable is appropriate. As an optimization some kinds of data, such as frequent transform updates, can be sent unreliably.

## Related Functions

[client_update](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_update.md)  
[client_pop_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_pop_packet.md)  
[client_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/client/client_free_packet.md)  
