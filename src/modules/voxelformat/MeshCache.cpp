/**
 * @file
 */

#include "MeshCache.h"
#include "core/GLM.h"
#include "core/StringUtil.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "voxel/CubicSurfaceExtractor.h"

namespace voxelformat {

MeshCache::~MeshCache() {
	core_assert_msg(_initCalls == 0, "MeshCache wasn't shut down properly: %i", _initCalls);
}

voxel::Mesh& MeshCache::cacheEntry(const char *fullPath) {
	auto i = _meshes.find(fullPath);
	if (i == _meshes.end()) {
		voxel::Mesh* mesh = new voxel::Mesh();
		_meshes.put(fullPath, mesh);
		Log::debug("New mesh cache entry for path %s", fullPath);
		return *mesh;
	}
	return *i->second;
}

bool MeshCache::removeMesh(const char *fullPath) {
	auto i = _meshes.find(fullPath);
	if (i != _meshes.end()) {
		delete i->second;
		_meshes.erase(i);
		return true;
	}
	return false;
}

const voxel::Mesh* MeshCache::getMesh(const char *fullPath) {
	voxel::Mesh &cachedMesh = cacheEntry(fullPath);
	if (cachedMesh.getNoOfVertices() > 0) {
		return &cachedMesh;
	}
	if (loadMesh(fullPath, cachedMesh)) {
		return &cachedMesh;
	}
	return nullptr;
}

bool MeshCache::loadMesh(const char* fullPath, voxel::Mesh& mesh) {
	Log::debug("Loading volume from %s", fullPath);
	const io::FilesystemPtr& fs = io::filesystem();
	io::FilePtr file;

	for (const char **ext = SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
		file = fs->open(core::string::format("%s.%s", fullPath, *ext));
		if (file->exists()) {
			break;
		}
	}
	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(file, volumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		voxelformat::clearVolumes(volumes);
		return false;
	}
	if ((int)volumes.size() != 1) {
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		voxelformat::clearVolumes(volumes);
		return false;
	}

	voxel::RawVolume* volume = volumes[0].volume;
	voxel::Region region = volume->region();
	region.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, region, &mesh, [] (const voxel::VoxelType& back, const voxel::VoxelType& front, voxel::FaceNames face) {
		return isBlocked(back) && !isBlocked(front);
	}, region.getLowerCorner());
	delete volume;

	Log::info("Generated mesh for %s", fullPath);
	return true;
}

bool MeshCache::init() {
	++_initCalls;
	return true;
}

void MeshCache::shutdown() {
	if (_initCalls == 0) {
		return;
	}
	--_initCalls;
	if (_initCalls > 0) {
		return;
	}
	for (const auto & e : _meshes) {
		delete e->value;
	}
	_meshes.clear();
}

}
