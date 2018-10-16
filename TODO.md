# Shader

## mesh.frag

there is an error in the fog computation - right now everything is in u_fogcolor because fogval == 1
this is due to the fact that fogdistance is a very very high value because gl_FragCoord.w is 0. And this
is related to the bonetrans matrix. Not taking it into account fixes gl_FragCoord.

## world.frag

```
ERROR: (0) Failed to validate: shaders/world
INFO: (0) Validation output: shaders/world
active samplers with a different type refer to the same texture image unit

# Persistence
## Checks

`ALTER TABLE products ADD CHECK (name <> '');`

or even better - do it in code.

## Prepared statements

Better support for prepared statements

## Joins

Add support (auto-generate) for joins for the foreign keys in the models

## Enums

Support enums?

`CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy');`

## Databasetool

* Support `(x, y, z) primarykey` (like we do for unique keys)
* Lock table metainfo whenever we try to update the database structure

# Shadertool

Validate that each `$out` of the vertex shader has a `$in` in the fragment shader and vice versa.

# VoxEdit

Extract meshes in max 32x32x32 boundaries - `RawVolumeRenderer` maybe? This allows us to only re-extract the chunk that was modified, not always the whole volume. See `Model::modified`.
Maybe introduce `PagedVolumeRenderer`.

Voxelizer via assimp

LSystem parameter window needs the voxel options and should show information about the used alphabet.

Move space colonization tree generator into the tree submenu and take the `TreeWindow` parameters into account.

## Improve selections

- implement invert-selection.
- Improve deletion of selected voxels - x should trigger popup like in blender which shows amount of voxels and enter to perform the delete step (or esc to close the dialog without deleting anything).
- Scale/move/rotate selections - not only the cursor or whole model.
- Extrude selection or voxel under cursor.
- Improve replacing of selection with something else - e.g. generators should maybe only operate on the selection?
- Implement selection operations like replace-selection, add-to-selection, remove-from-selection, only-keep-intersection.
- Same-Select mode should get a threshold with a threshold value. So e.g. select all gray voxels, but not the yellow ones.

# EventMgr

## Proper event-end-while-application-is-not-running handling

If an event was stopped, this must be persisted in the event table - skip those in the event provider. if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame this is needed if the server had a downtime while the event would have ended. In such a case, no loot would get hand out to the players. To work around this, we let the event just restore all its states and then stop properly.

## Rename EventProvider to EventSelector

The EventProvider should read the event lua scripts - the `EventProvider` should read the event run configuration (start- and end dates) from the database

# LootMgr

Implement `LootMgr` that is able to hand out or change items via the stock system. Implement lua loot definition 'parsing'.

# Stock

Think about the design again - `Item` shouldn't be a `shared_ptr`, just use the type in the map. That would also make an atomic counter useless, because we aren't operating on a shared data anymore. The `Stock` instance would be the owner and only instance that is able to modify the internal data.

Container move operations with filters - if might e.g. cost currency to move from one container to another.

# World

Implement island style maps. Each `Map` instance gets its own ai `Zone`. There should be portals on the islands to get to other islands. Provide an overview map of where the islands are.
(Theoretically we could scale/cluster per `Map` and each jump could include a server switch)
Maybe also allow to sail to a another island...?

These islands should not only be created by noise - just supported by noise to vary. But they should still be hand crafted to make them more interesting.

The `World` class manages the `Map` classes and should have a threadpool to update the maps in. Use libuv timers and forward the updating into the threads.

The `Map` should have a lua tick - which is e.g. able to spawn new npcs or let stuff happen on the map. It needs access to all the users, all the npcs and must be called on events like user-add/remove-from-map and npc-add/remove-from-map.

## SpawnMgr

Configure entity types and amounts via lua map script

## AttackMgr

The attack manager should get updated in the map tick and should maintain a list of attackers and victims.

## Map editor

Finish implementation of the `mapedit` tool.

# Statsd support

Extend statsd support with more events

# SignUp

Implement signup with email verification, Lost password and co - also see password related point in the persistence section.
Maybe use https://github.com/est31/csrp-gmp

# UI

Get rid of nativefiledialog - e.g. for nuklear use their file dialog implementation.
Currently only voxedit makes use of this library.

# Compute module (opencl)

- Support adding `#pragma unroll` automatically?

# LUA

Add support for lua_check linter

# Worldrenderer

## Culling

see "Silhouette Algorithms" by Bruce Gooch, Mark Hartner, and Nathan Beddes

calculate whether the sides of a chunk are completely filled - can be done with a bitmask of uint8_t for each chunk.
this can help culling later on.

the first few entries should get lesser chunk in the multi query than the ones that are
far away from the camera - it's more likely that the far away chunks are occluded.
doing one query per chunk is most likely a little bit overkill.

# testmesh

- testmesh (`TestMeshApp.cpp`) should render the sunlight and allow to change the position to test lighting
- normal rendering must be fixed
- bones rotation is broken on rendering. Use real bones and maybe even allow to change them.

# Deployment

- build docker images
- set up kubernetes manifests

# VoxEdit

* support multiple RawVolumes - and allow to move/scale/rotate complete volumes
  * the importers support multiple volumes to preserve the groups (and of course also the export)
