#Persistence
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

## Foreign keys

The databasetool should be able to generate foreign keys meta data. We can then use this data to validate
at compile time that the types we are referencing to are matching to the type we are referencing from. And
of course we could also add the db constraints.

# EventMgr

Proper event-end-while-application-is-not-running handling

If an event was stopped, this must be persisted in the event table - skip those in the event provider.
if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame
this is needed if the server had a downtime while the event would have ended. In such a case, no loot would
get hand out to the players. To work around this, we let the event just restore all its states and then stop
properly.

# LootMgr

Implement LootMgr that is able to hand out or change items via the stock system. Implement lua loot definition 'parsing'.

# World

Implement island style worlds. Each 'World' instance gets its own ai 'Zone'. There should be portals on the islands to get to other islands. Provide a map of where the islands are.
(Theoretically we could scale/cluster per 'World' and each jump could include a server switch)
Maybe also allow to sail to a another island...?

These islands should not only be created by noise - just supported by noise to vary. But they should still be hand crafted to make them more interesting.

## Map editor

There should be a map editor to place Point-of-Interests (see PoiProvider) and entities with attributes. E.g. you should be
able to place markers to modifiy the island shape and put attributes into these. The map editor should just allow to modifiy
these markers and place entities. Placing stuff on a per-voxel level should not be supported. The real map building should
still be implemented via procgen.
