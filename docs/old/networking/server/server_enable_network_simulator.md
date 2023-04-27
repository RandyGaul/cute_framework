# server_enable_network_simulator

Turns on the network simulator.

## Syntax

```cpp
void server_enable_network_simulator(server_t* server, double latency, double jitter, double drop_chance, double duplicate_chance);
```

## Function Parameters

Parameter Name | Description
--- | ---
server | The server.
latency | The delay to wait, in seconds, before sending a packet.
jitter | Number of seconds to randomly offset the latency, for each packet.
drop_chance | A number from 0 to 1. 0 means no packets are dropped, 1 means 100% of packets are dropped.
duplicate_chance | A number from 0 to 1. 0 means no packets are ever duplicated, 1 means 100% of packets are duplicated.

## Remarks

This function is for testing out how your networking performs under poor connection quality. This can be very useful for testing out networking over a very fast connection (like [localhost](https://en.wikipedia.org/wiki/Localhost)) during development.
