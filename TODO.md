# Persistence
## Savable

To collect sql update statements for changed player data, every handler should register itself
somewhere in a way that the handler (e.g. the CooldownMgr or AttribMgr) can answer the question
whether there is something to persist.
Now consider that there are 1000 players logged in and they all have there CooldownMgr instance.
The "collector-instance" would query all of them and asks them whether they are dirty or not. They
collect the data and build a mass update statement. This might happen every minute or something like
that.

## Password

Implement proper password handling (don't ever load passwords from DB into memory - just compare hashes).
(This should also be true for logging in - only transmit the hash - never the real password)

## Enums

Support enums?

    CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy');

## Databasetool

* Support `(x, y, z) primarykey` (like we do for unique keys)

## Schema

* Support schemas for user data and server configuration data (event, user, default, ...)

# EventMgr

## Proper event-end-while-application-is-not-running handling

If an event was stopped, this must be persisted in the event table - skip those in the event provider.
if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame
this is needed if the server had a downtime while the event would have ended. In such a case, no loot would
get hand out to the players. To work around this, we let the event just restore all its states and then stop
properly.

## Rename EventProvider to EventSelector

The EventProvider should read the event lua scripts - the EventSelector should read the event run configruation (start- and
end dates) from the database

# LootMgr

Implement `LootMgr` that is able to hand out or change items via the stock system. Implement lua loot definition 'parsing'.

# UserStockMgr

Implement saving and loading of `Stock` and `Inventory` data.

# Stock

Think about the design again - Item shouldn't be a shared_ptr, just use the type in the map. That would also make an atomic
counter useless, because we aren't operating on a shared data anymore. The `Stock` instance would be the owner and only
instance that is able to modify the internal data.

Container move operations with filters - if might e.g. cost currency to move from one container to another

# World

Implement island style maps. Each 'Map' instance gets its own ai 'Zone'. There should be portals on the islands to get to other islands. Provide an overview map of where the islands are.
(Theoretically we could scale/cluster per 'Map' and each jump could include a server switch)
Maybe also allow to sail to a another island...?

These islands should not only be created by noise - just supported by noise to vary. But they should still be hand crafted to make them more interesting.

The 'World' class manages the 'Map' classes and should have a threadpool to update the maps in. Use libuv timers and forward the updating into the threads.

## Map editor

There should be a map editor to place Point-of-Interests (see PoiProvider) and entities with attributes. E.g. you should be
able to place markers to modifiy the island shape and put attributes into these. The map editor should just allow to modifiy
these markers and place entities. Placing stuff on a per-voxel level should not be supported. The real map building should
still be implemented via procgen.

# Statsd support

Extend statsd support with more events

# EntityStorage

Moved login out of the entity storage
