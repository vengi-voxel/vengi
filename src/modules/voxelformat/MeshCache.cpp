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
#include "voxel/CubicSurfaceExtractor.h"

namespace voxelformat {

voxel::Mesh& MeshCache::cacheEntry(const char *path) {
	auto i = _meshes.find(path);
	if (i == _meshes.end()) {
		voxel::Mesh* mesh = new voxel::Mesh();
		_meshes.put(path, mesh);
		Log::debug("New mesh cache entry for path %s", path);
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

bool MeshCache::putMesh(const char* fullPath, const voxel::Mesh& mesh) {
	removeMesh(fullPath);
	_meshes.put(fullPath, new voxel::Mesh(mesh));
	return true;
}

bool MeshCache::loadMesh(const char* fullPath, voxel::Mesh& mesh) {
	Log::info("Loading volume from %s", fullPath);
	const io::FilesystemPtr& fs = io::filesystem();
	const io::FilePtr& file = fs->open(fullPath);
	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(file, volumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		for (auto& v : volumes) {
			delete v.volume;
		}
		return false;
	}
	if ((int)volumes.size() != 1) {
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		for (auto& v : volumes) {
			delete v.volume;
		}
		return false;
	}

	voxel::RawVolume* volume = volumes[0].volume;
	voxel::Region region = volume->region();
	region.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, region, &mesh, [] (const voxel::VoxelType& back, const voxel::VoxelType& front, voxel::FaceNames face) {
		return isBlocked(back) && !isBlocked(front);
	});
	delete volume;

	Log::info("Generated mesh for %s", fullPath);
	return true;
}

bool MeshCache::init() {
	return true;
}

void MeshCache::shutdown() {
	for (const auto & e : _meshes) {
		delete e->value;
	}
	_meshes.clear();
}

}
