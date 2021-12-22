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

VoxelVolume::VoxelVolume(VoxelVolume&& move) noexcept {
	volume = move.volume;
	move.volume = nullptr;
	name = move.name;
	visible = move.visible;
	pivot = move.pivot;
}

VoxelVolume &VoxelVolume::operator=(VoxelVolume &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	volume = move.volume;
	move.volume = nullptr;
	name = move.name;
	visible = move.visible;
	pivot = move.pivot;
	return *this;
}

void VoxelVolume::release() {
	delete volume;
	volume = nullptr;
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
		return new voxel::RawVolume(volumes[0].volume);
	}
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(volumes.size());
	for (const VoxelVolume &v : volumes) {
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

void clearVolumes(VoxelVolumes& volumes) {
	for (VoxelVolume& v : volumes) {
		v.release();
	}
	volumes.volumes.clear();
}

}
