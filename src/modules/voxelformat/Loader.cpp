/**
 * @file
 */

#include "Loader.h"
#include "core/Log.h"
#include "VoxFormat.h"
#include "QBTFormat.h"
#include "QBFormat.h"
#include "VXMFormat.h"
#include "CubFormat.h"
#include "BinVoxFormat.h"

namespace voxelformat {

bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes) {
	if (!filePtr->exists()) {
		Log::error("Failed to load model file %s", filePtr->name().c_str());
		return false;
	}
	const std::string& ext = filePtr->extension();
	if (ext == "qbt") {
		voxel::QBTFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "cub") {
		voxel::CubFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "vxm") {
		voxel::VXMFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "binvox") {
		voxel::BinVoxFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else {
		Log::error("Failed to load model file %s - unsupported file format for extension '%s'",
				filePtr->name().c_str(), ext.c_str());
		return false;
	}
	if (newVolumes.empty()) {
		Log::error("Failed to load model file %s", filePtr->name().c_str());
		return false;
	}
	Log::info("Load model file %s with %i layers", filePtr->name().c_str(), (int)newVolumes.size());
	return true;
}

}
