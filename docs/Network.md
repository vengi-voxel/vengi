# Network layer

The network layer is based on udp (enet) and shared between client and server.

It uses flatbuffers to generate C++ classes from fbs files that defines the protocol.
See the `generate_protocol` cmake macro on how to integrate them.

Values that should be shared between client and server - for example our cooldown ids - are
part of the protocol to always have them in sync with each other.

## Connection

* [client] connects
* [connection established]
* [client] sends `UserConnect` message
* [server] `UserConnectHandler`
* [server] performs auth
* [auth failed] => [server] sends `AuthFailed` message
* [auth successful] => [server] sends Seed [server] broadcasts to visible `UserSpawn`

## CVar replication

There are cvars that are replicated to all players on a server. The flag for this is `CV_REPLICATE`. If the server changes a value of such a cvar, the change
is automatically transfered to all connected clients. This cvar value is also initially for each new connection to ensure that the server and the client share
the same values for these cvars.

User related cvars that are broadcasted to other players are marked with `CV_BROADCAST`. These values are also submitted to any other player that can see the
entity.
