# General

The network layer is based on udp (enet) and shared between client and server.

It uses flatbuffers to generate C++ classes from fbs files that defines the protocol.
See the `generate_protocol` cmake macro on how to integrate them.

Values that should be shared between client and server - for example our cooldown ids - are
part of the protocol to always have them in sync with each other.

# Connection

* [client] connects
* [connection established]
* [client] send `UserConnect` message
* [server] `UserConnectHandler`
* [server] perform auth
* [auth failed] => [server] send `AuthFailed` message
* [auth successful] => [server] send Seed [server] broadcast to visible `UserSpawn`
