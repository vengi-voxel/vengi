#pragma once

#include "Color.h"
#include "Cubiquity.h"
#include "CubiquityForwardDeclarations.h"
#include "Volume.h"

namespace Cubiquity {

class ColoredCubesVolume: public Volume<Color> {
public:
	typedef Color VoxelType;

	ColoredCubesVolume(const Region& region, const std::string& pathToNewVoxelDatabase, unsigned int baseNodeSize) :
			Volume<Color>(region, pathToNewVoxelDatabase, baseNodeSize) {
		_voxelDatabase->setProperty("VoxelType", "Color");

		_octree = new Octree<VoxelType>(this, OctreeConstructionModes::BoundVoxels, baseNodeSize);
	}

	ColoredCubesVolume(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission, unsigned int baseNodeSize) :
			Volume<Color>(pathToExistingVoxelDatabase, writePermission, baseNodeSize) {
		std::string voxelType = _voxelDatabase->getPropertyAsString("VoxelType", "");
		core_assert_msg(voxelType == "Color", "VoxelDatabase does not have the expected VoxelType of 'Color'");

		_octree = new Octree<VoxelType>(this, OctreeConstructionModes::BoundVoxels, baseNodeSize);
	}

	virtual ~ColoredCubesVolume() {
		delete _octree;
	}
};

}
