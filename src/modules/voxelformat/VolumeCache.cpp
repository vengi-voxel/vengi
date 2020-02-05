/**
 * @file
 */

#include "VolumeCache.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/command/Command.h"

namespace voxelformat {

VolumeCache::~VolumeCache() {
	shutdown();
}

voxel::RawVolume* VolumeCache::loadVolume(const char* fullPath) {
	const core::String filename = fullPath;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		auto i = _volumes.find(filename);
		if (i != _volumes.end()) {
			return i->second;
		}
	}
	Log::info("Loading volume from %s", filename.c_str());
	const io::FilesystemPtr& fs = io::filesystem();
	const io::FilePtr& file = fs->open(filename);
	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(file, volumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		for (auto& v : volumes) {
			delete v.volume;
		}
		std::lock_guard<std::mutex> lock(_mutex);
		_volumes.put(filename, nullptr);
		return nullptr;
	}
	voxel::RawVolume* v = volumes.merge();
	for (auto& v : volumes) {
		delete v.volume;
	}
	std::lock_guard<std::mutex> lock(_mutex);
	_volumes.put(filename, v);
	return v;
}

void VolumeCache::construct() {
	core::Command::registerCommand("volumecachelist", [&] (const core::CmdArgs& argv) {
		Log::info("Cache content");
		std::lock_guard<std::mutex> lock(_mutex);
		for (const auto& e : _volumes) {
			Log::info(" * %s", e->key.c_str());
		}
	});
	core::Command::registerCommand("volumecacheclear", [&] (const core::CmdArgs& argv) {
		std::lock_guard<std::mutex> lock(_mutex);
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
	std::lock_guard<std::mutex> lock(_mutex);
	for (const auto & e : _volumes) {
		delete e->value;
	}
	_volumes.clear();
}

}
