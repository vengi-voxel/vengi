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

static core::DynamicArray<Selection> subtractBox(const Selection &box, const Selection &sub) {
	core::DynamicArray<Selection> result;
	result.reserve(6);

	// Ensure the subtraction region is inside the box
	if (sub.getLowerCorner().x > box.getUpperCorner().x || sub.getUpperCorner().x < box.getLowerCorner().x ||
		sub.getLowerCorner().y > box.getUpperCorner().y || sub.getUpperCorner().y < box.getLowerCorner().y ||
		sub.getLowerCorner().z > box.getUpperCorner().z || sub.getUpperCorner().z < box.getLowerCorner().z) {
		// No overlap, box remains unchanged
		result.push_back(box);
		return result;
	}

	// Top part (above the selected region)
	if (sub.getUpperCorner().z < box.getUpperCorner().z) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, sub.getUpperCorner().z + 1),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, box.getUpperCorner().z));
	}

	// Bottom part (below the selected region)
	if (sub.getLowerCorner().z > box.getLowerCorner().z) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, box.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, sub.getLowerCorner().z - 1));
	}

	// Front part (in front of the selected region)
	if (sub.getUpperCorner().y < box.getUpperCorner().y) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, sub.getUpperCorner().y + 1, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, box.getUpperCorner().y, sub.getUpperCorner().z));
	}

	// Back part (behind the selected region)
	if (sub.getLowerCorner().y > box.getLowerCorner().y) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, box.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, sub.getLowerCorner().y - 1, sub.getUpperCorner().z));
	}

	// Left part (left of the selected region)
	if (sub.getLowerCorner().x > box.getLowerCorner().x) {
		result.emplace_back(glm::ivec3(box.getLowerCorner().x, sub.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(sub.getLowerCorner().x - 1, sub.getUpperCorner().y, sub.getUpperCorner().z));
	}

	// Right part (right of the selected region)
	if (sub.getUpperCorner().x < box.getUpperCorner().x) {
		result.emplace_back(glm::ivec3(sub.getUpperCorner().x + 1, sub.getLowerCorner().y, sub.getLowerCorner().z),
							glm::ivec3(box.getUpperCorner().x, sub.getUpperCorner().y, sub.getUpperCorner().z));
	}

	return result;
}

void SelectionManager::invert(voxel::RawVolume &volume) {
	if (!hasSelection()) {
		select(volume, volume.region().getLowerCorner(), volume.region().getUpperCorner());
	} else {
		core::DynamicArray<Selection> remainingRegions;
		remainingRegions.reserve(_selections.size() * 6);
		remainingRegions.push_back(volume.region());

		for (const Selection &selection : _selections) {
			core::DynamicArray<Selection> newRegions;
			for (const Selection &region : remainingRegions) {
				const core::DynamicArray<Selection> &subtracted = subtractBox(region, selection);
				newRegions.insert(newRegions.end(), subtracted.begin(), subtracted.end());
			}
			remainingRegions = core::move(newRegions);
		}
		reset();
		for (const Selection &region : remainingRegions) {
			select(volume, region.getLowerCorner(), region.getUpperCorner());
		}
	}
}

void SelectionManager::unselect(voxel::RawVolume &volume) {
	reset();
}

void SelectionManager::reset() {
	_selections.clear();
}

voxel::Region SelectionManager::region() const {
	if (_selections.empty()) {
		return voxel::Region::InvalidRegion;
	}
	voxel::Region r = _selections[0];
	for (const Selection &selection : _selections) {
		r.accumulate(selection);
	}
	return r;
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
	return true;
}

} // namespace voxedit
