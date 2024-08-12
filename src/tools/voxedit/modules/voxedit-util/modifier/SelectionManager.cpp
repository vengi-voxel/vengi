/**
 * @file
 */

#include "SelectionManager.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"

namespace voxedit {

const Selections &SelectionManager::selections() const {
	return _selections;
}

void SelectionManager::start(const glm::ivec3 &startPos) {
	_selectStartPosition = startPos;
	_selectStartPositionValid = true;
}

void SelectionManager::stop() {
	_selectStartPositionValid = false;
}

void SelectionManager::invert(voxel::RawVolume &volume) {
	if (!_selectStartPositionValid) {
		return;
	}
	if (!_selectionValid) {
		select(volume, volume.region().getLowerCorner(), volume.region().getUpperCorner());
	} else {
		// TODO: SELECTION
	}
}

void SelectionManager::unselect(voxel::RawVolume &volume) {
	// _selectionVolume.clear();
	_selections.clear();
	_selectionValid = false;
}

void SelectionManager::reset() {
	_selections.clear();
	_selectionValid = false;
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
	_selectionValid = true;
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

voxel::Region SelectionManager::calcSelectionRegion(const glm::ivec3 &cursorPosition) const {
	const glm::ivec3 &mins = glm::min(_selectStartPosition, cursorPosition);
	const glm::ivec3 &maxs = glm::max(_selectStartPosition, cursorPosition);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
