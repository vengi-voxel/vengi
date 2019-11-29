/**
 * @file
 */

#include "CharacterCache.h"
#include "CharacterSkeleton.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Common.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"

namespace animation {

bool CharacterCache::loadGlider(const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]) {
	const int idx = settings.getIdxForName("glider");
	if (idx < 0 || idx >= (int)AnimationSettings::MAX_ENTRIES) {
		return false;
	}
	const char *fullPath = "models/glider.vox";
	voxel::Mesh& mesh = cacheEntry(fullPath);
	if (mesh.getNoOfVertices() > 0) {
		meshes[idx] = &mesh;
		return true;
	}
	if (loadMesh(fullPath, mesh)) {
		meshes[idx] = &mesh;
		return true;
	}
	meshes[idx] = nullptr;
	Log::error("Failed to load glider");
	return false;
}

}
