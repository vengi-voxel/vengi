/**
 * @file
 */

#include "SelectionManager.h"
#include "core/Common.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"

namespace voxedit {

const Selections &SelectionManager::selections() const {
	return _selections;
}

void SelectionManager::setMaxRegionSize(const voxel::Region &maxRegion) {
	_maxRegion = maxRegion;
}

void SelectionManager::invert(voxel::RawVolume &volume) {
	if (!hasSelection()) {
		select(volume, volume.region().getLowerCorner(), volume.region().getUpperCorner());
	} else {
		const Selections &remainingSelections = voxel::Region::subtract(volume.region(), _selections);
		reset();
		for (const Selection &selection : remainingSelections) {
			select(volume, selection.getLowerCorner(), selection.getUpperCorner());
		}
	}
}

void SelectionManager::unselect(voxel::RawVolume &volume) {
	reset();
}

void SelectionManager::reset() {
	_selections.clear();
	markDirty();
}

const voxel::Region &SelectionManager::region() {
	if (!dirty()) {
		return _cachedRegion;
	}
	if (_selections.empty()) {
		return voxel::Region::InvalidRegion;
	}
	voxel::Region r = _selections[0];
	for (const Selection &selection : _selections) {
		r.accumulate(selection);
	}
	_cachedRegion = r;
	markClean();
	return _cachedRegion;
}

bool SelectionManager::select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	const Selection sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	for (size_t i = 0; i < _selections.size(); ++i) {
		const Selection &s = _selections[i];
		if (s.containsRegion(sel)) {
			return true;
		}
	}

	for (size_t i = 0; i < _selections.size();) {
		Selection &s = _selections[i];
		if (sel.containsRegion(s)) {
			_selections.erase(i);
		} else if (voxel::intersects(sel, s)) {
			// TODO: SELECTION: slice
			++i;
		} else {
			++i;
		}
	}
	_selections.push_back(sel);
	markDirty();
	return true;
}

bool SelectionManager::unselect(voxel::RawVolume &volume, const glm::ivec3 &pos) {
	return false;
}

bool SelectionManager::select(voxel::RawVolume &volume, const glm::ivec3 &pos) {
	return select(volume, pos, pos);
}

bool SelectionManager::isSelected(const glm::ivec3 &pos) const {
	if (!dirty()) {
		if (!_cachedRegion.containsPoint(pos)) {
			return false;
		}
	}

	for (const Selection &sel : _selections) {
		if (sel.containsPoint(pos)) {
			return true;
		}
	}
	return false;
}

voxel::RawVolume *SelectionManager::cut(voxel::RawVolume &volume) {
	if (!hasSelection()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(volume, _selections);
	for (const Selection &selection : _selections) {
		const glm::ivec3 &mins = selection.getLowerCorner();
		const glm::ivec3 &maxs = selection.getUpperCorner();
		static constexpr voxel::Voxel AIR;
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(mins);
		for (int32_t z = mins.z; z <= maxs.z; ++z) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int32_t y = mins.y; y <= maxs.y; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int32_t x = mins.x; x <= maxs.x; ++x) {
					sampler3.setVoxel(AIR);
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	}
	return v;
}

voxel::RawVolume *SelectionManager::copy(const voxel::RawVolume &volume) {
	if (!hasSelection()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(volume, _selections);
	return v;
}

} // namespace voxedit
