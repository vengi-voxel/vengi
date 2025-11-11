/**
 * @file
 */

#include "ClipboardData.h"

namespace voxel {

ClipboardData::ClipboardData(const voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(*p)) {
}

ClipboardData::ClipboardData(const voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(p)) {
}

ClipboardData::ClipboardData(voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(*p)) {
}

ClipboardData::ClipboardData(voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(p)) {
}

ClipboardData::ClipboardData(const ClipboardData &v)
	: _disposeAfterUse(true), volume(new voxel::RawVolume(*v.volume)), palette(new palette::Palette(*v.palette)) {
}

ClipboardData::ClipboardData(ClipboardData &&v) : _disposeAfterUse(v._disposeAfterUse), volume(v.volume), palette(v.palette) {
	v.volume = nullptr;
	v.palette = nullptr;
}

ClipboardData::~ClipboardData() {
	if (_disposeAfterUse) {
		delete volume;
	}
	delete palette;
}

bool ClipboardData::dispose() const {
	return _disposeAfterUse;
}

ClipboardData &ClipboardData::operator=(const ClipboardData &v) {
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

ClipboardData &ClipboardData::operator=(ClipboardData &&v) {
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

} // namespace voxel
