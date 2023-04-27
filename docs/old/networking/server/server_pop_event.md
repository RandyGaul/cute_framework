# server_pop_event

Pops a server event off of the internal event queue. See Remarks below for details on what events there are.

## Syntax

```cpp
bool server_pop_event(server_t* server, server_event_t* event);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
event | An event of type `server_event_t`, as described in the Remarks section below. The pointer can point to an instance of `server_event_t` on the stack as a temporary local variable.

## Return Value

Returns true if an event was successfully popped, false otherwise. You should not read from your `event` pointer if false is returned.

## Remarks

The `server_event_t` is a union of different possible server event types. The type is specified as a `server_event_type_t` enumeration. Details for the union and enum are below.

### server_event_type_t

Enumeration Entry | Description
--- | ---
SERVER_EVENT_TYPE_NEW_CONNECTION | A new incoming connection.
SERVER_EVENT_TYPE_DISCONNECTED | A disconnecting client.
SERVER_EVENT_TYPE_PAYLOAD_PACKET | An incoming packet from a client.

### server_event_t

Member Variable Name | Type | Description
--- | --- | ---
type | `server_event_type_t` | The type of the event.
new_connection | anonymous struct | Relevant information of a new incoming connection.
disconnected | anonymous struct | The `client_index` of a disconnecting client.
payload_packet | anonymous struct | An incoming packet from a client.

#### new_connection

Member Variable Name | Type | Description
--- | --- | ---
client_index | `int` | An index representing this particular client.
client_id | `uint64_t` | A unique identifier for this particular client, as read from the [connect token](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/).
endpoint | `endpoint_t` | The address and port of the incoming connection, see [endpoint_t](https://github.com/RandyGaul/cute_framework/tree/master/docs/networking/endpoint_t.md).

#### disconnected

Member Variable Name | Type | Description
--- | --- | ---
client_index | `int` | An index representing this particular client.

#### payload_packet

Member Variable Name | Type | Description
--- | --- | ---
client_index | `int` | An index representing this particular client.
data | `void*` | Pointer to the packet's payload data.
size | int | Size of the packet at the `data` pointer.

## Related Functions

[server_free_packet](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_free_packet.md)  
[server_send](https://github.com/RandyGaul/cute_framework/blob/master/docs/networking/server/server_send.md)  
