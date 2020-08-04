/**
 * @file
 */

#include "VoxelVolumes.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"
#include "core/Common.h"

namespace voxel {

VoxelVolume::VoxelVolume(voxel::RawVolume *_volume, const core::String &_name, bool _visible)
	: volume(_volume), name(_name), visible(_visible) {
	if (volume != nullptr) {
		pivot = volume->region().getCenter();
	} else {
		pivot = glm::ivec3(0.0f);
	}
}
VoxelVolume::VoxelVolume(voxel::RawVolume *_volume, const core::String &_name, bool _visible, const glm::ivec3 &_pivot)
	: volume(_volume), name(_name), visible(_visible), pivot(_pivot) {
}

VoxelVolumes::~VoxelVolumes() {
	volumes.clear();
}

void VoxelVolumes::push_back(VoxelVolume&& v) {
	volumes.emplace_back(core::forward<VoxelVolume>(v));
}

void VoxelVolumes::resize(size_t size) {
	volumes.resize(size);
}

void VoxelVolumes::reserve(size_t size) {
	volumes.reserve(size);
}

bool VoxelVolumes::empty() const {
	return volumes.empty();
}

size_t VoxelVolumes::size() const {
	return volumes.size();
}

const VoxelVolume &VoxelVolumes::operator[](size_t idx) const {
	return volumes[idx];
}

VoxelVolume& VoxelVolumes::operator[](size_t idx) {
	return volumes[idx];
}

voxel::RawVolume *VoxelVolumes::merge() const {
	if (volumes.empty()) {
		return nullptr;
	}
	if (volumes.size() == 1) {
		if (volumes[0].volume == nullptr) {
			return nullptr;
		}
		return new RawVolume(volumes[0].volume);
	}
	std::vector<const RawVolume *> rawVolumes;
	rawVolumes.reserve(volumes.size());
	for (const auto &v : volumes) {
		if (v.volume == nullptr) {
			continue;
		}
		rawVolumes.push_back(v.volume);
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	return ::voxel::merge(rawVolumes);
}

}
