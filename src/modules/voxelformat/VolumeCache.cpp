/**
 * @file
 */

#include "VolumeCache.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/command/Command.h"
#include "core/Log.h"

namespace voxelformat {

VolumeCache::~VolumeCache() {
	core_assert_msg(_volumes.empty(), "VolumeCache wasn't shut down properly");
}

voxel::RawVolume* VolumeCache::loadVolume(const char* fullPath) {
	const core::String filename = fullPath;
	{
		core::ScopedLock lock(_mutex);
		auto i = _volumes.find(filename);
		if (i != _volumes.end()) {
			return i->second;
		}
	}
	Log::info("Loading volume from %s", fullPath);
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
		core::ScopedLock lock(_mutex);
		_volumes.put(filename, nullptr);
		return nullptr;
	}
	voxel::RawVolume* v = volumes.merge();
	voxelformat::clearVolumes(volumes);

	core::ScopedLock lock(_mutex);
	_volumes.put(filename, v);
	return v;
}

void VolumeCache::construct() {
	core::Command::registerCommand("volumecachelist", [&] (const core::CmdArgs& argv) {
		Log::info("Cache content");
		core::ScopedLock lock(_mutex);
		for (const auto& e : _volumes) {
			Log::info(" * %s", e->key.c_str());
		}
	});
	core::Command::registerCommand("volumecacheclear", [&] (const core::CmdArgs& argv) {
		core::ScopedLock lock(_mutex);
		for (const auto & e : _volumes) {
			delete e->value;
		}
		_volumes.clear();
	});
}

bool VolumeCache::init() {
	return true;
}

void VolumeCache::shutdown() {
	core::ScopedLock lock(_mutex);
	for (const auto & e : _volumes) {
		delete e->value;
	}
	_volumes.clear();
}

}
