/**
 * @file
 */

#include "VoxelVolumes.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"
#include "core/Common.h"

namespace voxel {

VoxelVolume::VoxelVolume(voxel::RawVolume *volume, const core::String &name, bool visible)
	: _name(name), _volume(volume), _visible(visible) {
	if (volume != nullptr) {
		_pivot = volume->region().getCenter();
	} else {
		_pivot = glm::ivec3(0.0f);
	}
}
VoxelVolume::VoxelVolume(voxel::RawVolume *volume, const core::String &name, bool visible, const glm::ivec3 &pivot)
	: _name(name), _volume(volume), _visible(visible), _pivot(pivot) {
}

VoxelVolume::VoxelVolume(const voxel::RawVolume *volume, const core::String &name, bool visible)
	: _name(name), _volume((voxel::RawVolume *)volume), _visible(visible) {
	if (volume != nullptr) {
		_pivot = volume->region().getCenter();
	} else {
		_pivot = glm::ivec3(0.0f);
	}
}
VoxelVolume::VoxelVolume(const voxel::RawVolume *volume, const core::String &name, bool visible, const glm::ivec3 &pivot)
	: _name(name), _volume((voxel::RawVolume *)volume), _visible(visible), _pivot(pivot) {
}

VoxelVolume::VoxelVolume(VoxelVolume&& move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = move._name;
	_visible = move._visible;
	_pivot = move._pivot;
	_volumeOwned = move._volumeOwned;
	move._volumeOwned = false;
}

VoxelVolume &VoxelVolume::operator=(VoxelVolume &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	setVolume(move._volume);
	move._volume = nullptr;
	_name = move._name;
	_visible = move._visible;
	_pivot = move._pivot;
	_volumeOwned = move._volumeOwned;
	move._volumeOwned = false;
	return *this;
}

void VoxelVolume::release() {
	if (_volumeOwned) {
		delete _volume;
	}
	_volume = nullptr;
}

void VoxelVolume::setVolume(voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = volume;
}

void VoxelVolume::setVolume(const voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = (voxel::RawVolume *)volume;
}

const voxel::Region &VoxelVolume::region() const {
	if (_volume == nullptr ) {
		return voxel::Region::InvalidRegion;
	}
	return _volume->region();
}

void VoxelVolume::translate(const glm::ivec3 &v) {
	if (_volume != nullptr) {
		_volume->translate(v);
	}
}

VoxelVolumes::~VoxelVolumes() {
	_volumes.clear();
}

void VoxelVolumes::emplace_back(VoxelVolume&& v) {
	_volumes.emplace_back(core::forward<VoxelVolume>(v));
}

void VoxelVolumes::resize(size_t size) {
	_volumes.resize(size);
}

void VoxelVolumes::reserve(size_t size) {
	_volumes.reserve(size);
}

bool VoxelVolumes::empty() const {
	return _volumes.empty();
}

size_t VoxelVolumes::size() const {
	return _volumes.size();
}

const VoxelVolume &VoxelVolumes::operator[](size_t idx) const {
	return _volumes[idx];
}

VoxelVolume& VoxelVolumes::operator[](size_t idx) {
	return _volumes[idx];
}

voxel::RawVolume *VoxelVolumes::merge() const {
	if (_volumes.empty()) {
		return nullptr;
	}
	if (_volumes.size() == 1) {
		if (_volumes[0].volume() == nullptr) {
			return nullptr;
		}
		return new voxel::RawVolume(_volumes[0].volume());
	}
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(_volumes.size());
	for (const VoxelVolume &v : _volumes) {
		if (v.volume() == nullptr) {
			continue;
		}
		rawVolumes.push_back(v.volume());
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	return ::voxel::merge(rawVolumes);
}

void clearVolumes(VoxelVolumes& volumes) {
	volumes.clear();
}

}
