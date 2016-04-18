#pragma once

#include "BackgroundTaskProcessor.h"
#include "CubiquityForwardDeclarations.h"
#include "Octree.h"
#include "CVector.h"
#include "VoxelTraits.h"
#include "WritePermissions.h"

#include "PolyVox/Array.h"
#include "PolyVox/Material.h"
#include "PolyVox/RawVolume.h"
#include "PolyVox/PagedVolume.h"
#include "PolyVox/CubicSurfaceExtractor.h"
#include "PolyVox/MarchingCubesSurfaceExtractor.h"
#include "PolyVox/LowPassFilter.h"
#include "PolyVox/MaterialDensityPair.h"
#include "PolyVox/Raycast.h"
#include "PolyVox/VolumeResampler.h"
#include "PolyVox/Utility.h" //Should we include from Impl?

#include "Clock.h"
#include "BackgroundTaskProcessor.h"
#include "MainThreadTaskProcessor.h"
#include "MaterialSet.h"
#include "Raycasting.h"
#include "SQLiteUtils.h"
#include "VoxelDatabase.h"

#include <stdlib.h>
#include <time.h>

#include <sqlite3.h>

namespace Cubiquity {

template<typename _VoxelType>
class Volume {
public:
	typedef _VoxelType VoxelType;

	Volume(const Region& region, const std::string& pathToNewVoxelDatabase, uint32_t baseNodeSize);
	Volume(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission, uint32_t baseNodeSize);

	virtual ~Volume();

	// These functions just forward to the underlying PolyVox volume.
	uint32_t getWidth(void) const {
		return _polyVoxVolume->getWidth();
	}

	uint32_t getHeight(void) const {
		return _polyVoxVolume->getHeight();
	}

	uint32_t getDepth(void) const {
		return _polyVoxVolume->getDepth();
	}

	const Region& getEnclosingRegion(void) const {
		return _enclosingRegion;
	}

	// Note this adds a border rather than calling straight through.
	VoxelType getVoxel(int32_t x, int32_t y, int32_t z) const;

	// This one's a bit of a hack... direct access to underlying PolyVox volume
	::PolyVox::PagedVolume<VoxelType>* _getPolyVoxVolume(void) const {
		return _polyVoxVolume;
	}

	// Octree access
	Octree<VoxelType>* getOctree(void) {
		return _octree;
	}

	OctreeNode<VoxelType>* getRootOctreeNode(void) {
		return _octree->getRootNode();
	}

	// Set voxel doesn't just pass straight through, it also validates the position and marks the voxel as modified.
	void setVoxel(int32_t x, int32_t y, int32_t z, VoxelType value, bool markAsModified);

	// Marks a region as modified so it will be regenerated later.
	void markAsModified(const Region& region);

	void acceptOverrideChunks(void) {
		_polyVoxVolume->flushAll();
		_voxelDatabase->acceptOverrideChunks();
	}

	void discardOverrideChunks(void) {
		_polyVoxVolume->flushAll();
		_voxelDatabase->discardOverrideChunks();
	}

	// Should be called before rendering a frame to update the meshes and octree structure.
	virtual bool update(const Vector3F& viewPosition, float lodThreshold);

	// It's a bit ugly that the background task processor is part of the volume class.
	// We do this because we want to clear it when the volume is destroyed, to avoid
	// the situation wher it continues to process tasks from the destroyed volume.
	// A better solution is needed here (smart pointers?).
	BackgroundTaskProcessor* _backgroundTaskProcessor;

protected:
	Octree<VoxelType>* _octree = nullptr;
	VoxelDatabase<VoxelType>* _voxelDatabase;

private:
	Volume& operator=(const Volume&);

	::PolyVox::Region _enclosingRegion;
	::PolyVox::PagedVolume<VoxelType>* _polyVoxVolume;

	//sqlite3* mDatabase;

	// Friend functions
	friend class Octree<VoxelType>;
	friend class OctreeNode<VoxelType>;
};

template<typename VoxelType>
Volume<VoxelType>::Volume(const Region& region, const std::string& pathToNewVoxelDatabase, uint32_t baseNodeSize) {
	core_assert_msg(region.getWidthInVoxels() > 0, "Volume width must be greater than zero");
	core_assert_msg(region.getHeightInVoxels() > 0, "Volume height must be greater than zero");
	core_assert_msg(region.getDepthInVoxels() > 0, "Volume depth must be greater than zero");

	//m_pVoxelDatabase = new VoxelDatabase<VoxelType>;
	//m_pVoxelDatabase->create(pathToNewVoxelDatabase);

	_enclosingRegion = region;

	_voxelDatabase = VoxelDatabase<VoxelType>::createEmpty(pathToNewVoxelDatabase);

	// Store the volume region to the database.
	_voxelDatabase->setProperty("lowerX", region.getLowerX());
	_voxelDatabase->setProperty("lowerY", region.getLowerY());
	_voxelDatabase->setProperty("lowerZ", region.getLowerZ());
	_voxelDatabase->setProperty("upperX", region.getUpperX());
	_voxelDatabase->setProperty("upperY", region.getUpperY());
	_voxelDatabase->setProperty("upperZ", region.getUpperZ());

	_polyVoxVolume = new ::PolyVox::PagedVolume<VoxelType>(_voxelDatabase, 256 * 1024 * 1024, 32);

	_backgroundTaskProcessor = new BackgroundTaskProcessor();
}

template<typename VoxelType>
Volume<VoxelType>::Volume(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission, uint32_t baseNodeSize) {
	_voxelDatabase = VoxelDatabase<VoxelType>::createFromVDB(pathToExistingVoxelDatabase, writePermission);

	// Get the volume region from the database. The default values
	// are fairly arbitrary as there is no sensible choice here.
	int32_t lowerX = _voxelDatabase->getPropertyAsInt("lowerX", 0);
	int32_t lowerY = _voxelDatabase->getPropertyAsInt("lowerY", 0);
	int32_t lowerZ = _voxelDatabase->getPropertyAsInt("lowerZ", 0);
	int32_t upperX = _voxelDatabase->getPropertyAsInt("upperX", 512);
	int32_t upperY = _voxelDatabase->getPropertyAsInt("upperY", 512);
	int32_t upperZ = _voxelDatabase->getPropertyAsInt("upperZ", 512);

	_enclosingRegion = Region(lowerX, lowerY, lowerZ, upperX, upperY, upperZ);

	_polyVoxVolume = new ::PolyVox::PagedVolume<VoxelType>(_voxelDatabase, 256 * 1024 * 1024, 32);

	_backgroundTaskProcessor = new BackgroundTaskProcessor();
}

template<typename VoxelType>
Volume<VoxelType>::~Volume() {
	Log::trace("Entering ~Volume()");

	delete _backgroundTaskProcessor;
	_backgroundTaskProcessor = 0;

	// NOTE: We should really delete the volume here, but the background task processor might still be using it.
	// We need a way to shut that down, or maybe smart pointers can help here. Just flush until we have a better fix.
	_polyVoxVolume->flushAll();

	//delete mPolyVoxVolume;

	delete _voxelDatabase;

	Log::trace("Exiting ~Volume()");
}

template<typename VoxelType>
VoxelType Volume<VoxelType>::getVoxel(int32_t x, int32_t y, int32_t z) const {
	return _polyVoxVolume->getVoxel(x, y, z);
}

template<typename VoxelType>
void Volume<VoxelType>::setVoxel(int32_t x, int32_t y, int32_t z, VoxelType value, bool markAsModified) {
	// Validate the voxel position
	core_assert_msg(_enclosingRegion.containsPoint(x, y, z), "Attempted to write to a voxel which is outside of the volume");

	_polyVoxVolume->setVoxel(x, y, z, value);
	if (markAsModified) {
		_octree->markDataAsModified(x, y, z, Clock::getTimestamp());
	}
}

template<typename VoxelType>
void Volume<VoxelType>::markAsModified(const Region& region) {
	_octree->markDataAsModified(region, Clock::getTimestamp());
}

template<typename VoxelType>
bool Volume<VoxelType>::update(const Vector3F& viewPosition, float lodThreshold) {
	return _octree->update(viewPosition, lodThreshold);
}

}
