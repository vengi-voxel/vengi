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
	const core::String& ext = filePtr->extension();
	if (ext == "qbt") {
		voxel::QBTFormat f;
		f.loadGroups(filePtr, newVolumes);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		f.loadGroups(filePtr, newVolumes);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		f.loadGroups(filePtr, newVolumes);
	} else if (ext == "cub") {
		voxel::CubFormat f;
		f.loadGroups(filePtr, newVolumes);
	} else if (ext == "vxm") {
		voxel::VXMFormat f;
		f.loadGroups(filePtr, newVolumes);
	} else if (ext == "binvox") {
		voxel::BinVoxFormat f;
		f.loadGroups(filePtr, newVolumes);
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

bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (volumes.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	const core::String& ext = filePtr->extension();
	if (ext == "qbt") {
		voxel::QBTFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "cub") {
		voxel::CubFormat f;
		return f.saveGroups(volumes, filePtr);
	} else {
		Log::warn("Failed to save file with unknown type: %s - saving as vox instead", ext.c_str());
		voxel::VoxFormat f;
		return f.saveGroups(volumes, filePtr);
	}
	Log::info("Save model file %s with %i layers", filePtr->name().c_str(), (int)volumes.size());
	return false;
}

void clearVolumes(voxel::VoxelVolumes& volumes) {
	for (auto& v : volumes) {
		delete v.volume;
	}
	volumes.volumes.clear();
}

}
