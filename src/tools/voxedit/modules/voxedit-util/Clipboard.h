/**
 * @file
 */

#pragma once

#include "modifier/Selection.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"

namespace voxedit {
namespace tool {

class VoxelData {
private:
	bool _disposeAfterUse = false;

public:
	voxel::RawVolume *volume = nullptr;
	palette::Palette *palette = nullptr;

	VoxelData() = default;

	VoxelData(const voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
		: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(*p)) {
	}

	VoxelData(const voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
		: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(p)) {
	}

	VoxelData(voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
		: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(*p)) {
	}

	VoxelData(voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
		: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(p)) {
	}

	VoxelData(const VoxelData &v)
		: _disposeAfterUse(true), volume(new voxel::RawVolume(*v.volume)), palette(new palette::Palette(*v.palette)) {
	}

	VoxelData(VoxelData &&v) : _disposeAfterUse(v._disposeAfterUse), volume(v.volume), palette(v.palette) {
		v.volume = nullptr;
		v.palette = nullptr;
	}

	~VoxelData() {
		if (_disposeAfterUse) {
			delete volume;
		}
		delete palette;
	}

	operator bool() const {
		return volume != nullptr && palette != nullptr;
	}

	inline bool dispose() const {
		return _disposeAfterUse;
	}

	VoxelData &operator=(const VoxelData &v) {
		if (this == &v) {
			return *this;
		}
		if (_disposeAfterUse) {
			delete volume;
		}
		delete palette;

		palette = new palette::Palette(*v.palette);
		if (v._disposeAfterUse) {
			volume = new voxel::RawVolume(*v.volume);
		} else {
			volume = v.volume;
		}
		_disposeAfterUse = v._disposeAfterUse;
		return *this;
	}

	VoxelData &operator=(VoxelData &&v) {
		if (this == &v) {
			return *this;
		}
		if (_disposeAfterUse) {
			delete volume;
		}
		delete palette;
		volume = v.volume;
		palette = v.palette;
		_disposeAfterUse = v._disposeAfterUse;
		v.volume = nullptr;
		v.palette = nullptr;
		return *this;
	}
};

VoxelData copy(const VoxelData &voxelData, const Selections &selections);
VoxelData cut(VoxelData &voxelData, const Selections &selections, voxel::Region &modifiedRegion);
void paste(VoxelData &out, const VoxelData &in, const glm::ivec3 &referencePosition, voxel::Region &modifiedRegion);

} // namespace tool
} // namespace voxedit
