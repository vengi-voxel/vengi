#pragma once

#include "Cubiquity.h"
#include "CubiquityForwardDeclarations.h"
#include "Volume.h"
#include "core/Common.h"

namespace Cubiquity {

class TerrainVolume: public Volume<MaterialSet> {
public:
	typedef MaterialSet VoxelType;

	TerrainVolume(const Region& region, const std::string& pathToNewVoxelDatabase, unsigned int baseNodeSize) :
			Volume<MaterialSet>(region, pathToNewVoxelDatabase, baseNodeSize) {
		_voxelDatabase->setProperty("VoxelType", "MaterialSet");
		_octree = new Octree<VoxelType>(this, OctreeConstructionModes::BoundCells, baseNodeSize);
	}

	TerrainVolume(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission, unsigned int baseNodeSize) :
			Volume<MaterialSet>(pathToExistingVoxelDatabase, writePermission, baseNodeSize) {
		std::string voxelType = _voxelDatabase->getPropertyAsString("VoxelType", "");
		core_assert_msg(voxelType == "MaterialSet", "VoxelDatabase does not have the expected VoxelType of 'MaterialSet'");
		_octree = new Octree<VoxelType>(this, OctreeConstructionModes::BoundCells, baseNodeSize);
	}

	virtual ~TerrainVolume() {
		delete _octree;
	}
};

}
