# MapEdit

The `mapedit` tool is the tool to build maps. Not by placing voxel per voxel, but just
by putting entities into the void with parameters to procedurally generate the surface.

Besides the procgen parameter entities, there can also be point of interests.

The points are taken as a base to blend the noise parameters, place trees and so on. For example if
an entity with a tree option is coming in range, it is slowly blended in. The blend factor can take
incluence on e.g. the tree type or size.

The same is true for the noise parameters (or biomes) - they are just blended in.

# Entity options

## Terrain-building

- Blend mode
- Range
- NoiseType and related input parameters

## Plant-building

- Blend mode
- Plant types and related parameters
- Density

## Point-of-Interest

There are special entities that e.g. spawn stuff at the given location. This can e.g. be used to
place portals to get to other maps.

- Point-of-Interest type
- Relevance factor

## Voxel-Model placement

Instead of placing single voxels, we must be able to place volume data at a given location (without
modifying it - the `voxedit` tool is responsible for this).

# Assemble the world

To be able to assemble the world from this, the world-building-entities are organized in an octree to
perform queries about `what-do-I-influence` and `by-whom-am-I-influenced`. This is blended by the blend modes
and the distance relative to the world-building-entity and the final voxel position that is currently
evaluated.

All of this should happen in the `voxel` module in the `voxel::World` class.
