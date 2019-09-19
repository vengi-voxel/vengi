/**
 * @file
 */

#include "MeshCache.h"
#include "core/GLM.h"
#include "core/String.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"
#include "io/Filesystem.h"
#include "core/App.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"

namespace voxelformat {

voxel::Mesh& MeshCache::cacheEntry(const char *path) {
	auto i = _meshes.find(path);
	if (i == _meshes.end()) {
		voxel::Mesh* mesh = new voxel::Mesh();
		_meshes.insert(std::make_pair(path, mesh));
		Log::debug("New mesh cache entry for path %s", path);
		return *mesh;
	}
	return *i->second;
}

bool MeshCache::loadMesh(const char* fullPath, voxel::Mesh& mesh) {
	Log::info("Loading volume from %s", fullPath);
	const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
	const io::FilePtr& file = fs->open(fullPath);
	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(file, volumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		return false;
	}
	if ((int)volumes.size() != 1) {
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		return false;
	}

	voxel::RawVolume* volume = volumes[0].volume;
	voxel::Region region = volume->region();
	region.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, region, &mesh, [] (const voxel::VoxelType& back, const voxel::VoxelType& front, voxel::FaceNames face) {
		return isBlocked(back) && !isBlocked(front);
	});

	return true;
}

bool MeshCache::init() {
	return true;
}

void MeshCache::shutdown() {
	for (auto & e : _meshes) {
		delete e.second;
	}
	_meshes.clear();
}

}
