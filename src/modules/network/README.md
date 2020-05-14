# Network layer

## General

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
