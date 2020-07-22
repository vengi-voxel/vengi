# Purpose

The attrib module manages containers with attributes that can be attached to any item or entity in the world.

Attributes are things like speed, strength, intelligence (see the network definition for a full list).

The attributes are then calculated by the different containers an entity has assigned (via equipment, or directly
to the character). The absolute values are summed up, and then the percentage modifiers are added on top.

If you e.g. get a debuff or debuff this is just applying a container to your entity. The values are communicated
to the client and some are also broadcasted to other clients that are seeing you.

# LUA

The containers are defined in lua scripts. See the attribute files that are coming with the tests, or with the server.
