/**
 * @file
 */

#include "Loader.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "VoxFormat.h"
#include "QBTFormat.h"
#include "QBFormat.h"
#include "QEFFormat.h"
#include "VXMFormat.h"
#include "VXLFormat.h"
#include "CubFormat.h"
#include "BinVoxFormat.h"
#include "KVXFormat.h"
#include "KV6Format.h"
#include "AoSVXLFormat.h"

namespace voxelformat {

// this is the list of supported voxel volume formats that are have importers implemented
const char *SUPPORTED_VOXEL_FORMATS_LOAD = "vox,qbt,qb,vxm,binvox,cub,kvx,kv6,vxl,qef";
// this is the list of internal formats that are supported engine-wide (the format we save our own models in)
const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[] = { "qb", "vox", nullptr };
// this is the list of supported voxel volume formats that have exporters implemented
const char *SUPPORTED_VOXEL_FORMATS_SAVE = "vox,qbt,qb,binvox,cub,vxl,qef";

bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes) {
	if (!filePtr->exists()) {
		Log::error("Failed to load model file %s", filePtr->name().c_str());
		return false;
	}
	core_trace_scoped(LoadVolumeFormat);
	const core::String& ext = filePtr->extension();
	if (ext == "qb") {
		voxel::QBFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "qbt") {
		voxel::QBTFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "kvx") {
		voxel::KVXFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "kv6") {
		voxel::KV6Format f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "cub") {
		voxel::CubFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxm") {
		voxel::VXMFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxl") {
		voxel::VXLFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
			voxel::AoSVXLFormat f2;
			if (!f2.loadGroups(filePtr, newVolumes)) {
				voxelformat::clearVolumes(newVolumes);
			}
		}
	} else if (ext == "binvox") {
		voxel::BinVoxFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "qef") {
		voxel::QEFFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
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
	if (ext == "qb") {
		voxel::QBFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "qbt") {
		voxel::QBTFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "qef") {
		voxel::QEFFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "cub") {
		voxel::CubFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "vxl") {
		voxel::VXLFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "binvox") {
		voxel::BinVoxFormat f;
		return f.saveGroups(volumes, filePtr);
	} else {
		Log::warn("Failed to save file with unknown type: %s - saving as qb instead", ext.c_str());
		voxel::QBFormat f;
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
