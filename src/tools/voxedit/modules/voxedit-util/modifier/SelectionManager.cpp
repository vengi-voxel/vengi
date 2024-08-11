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

void SelectionManager::start(const voxel::Region &maxRegion, const glm::ivec3 &startPos) {
	_selectStartPosition = startPos;
	_selectStartPositionValid = true;
	_maxRegion = maxRegion;
}

void SelectionManager::stop() {
	_selectStartPositionValid = false;
}

void SelectionManager::invert(voxel::RawVolume &volume) {
	if (!_selectStartPositionValid) {
		return;
	}
	if (!_selectionValid) {
		// TODO: SELECTION: use the volume region and remove _maxRegion member?
		select(volume, _maxRegion.getLowerCorner(), _maxRegion.getUpperCorner());
	} else {
		// TODO: SELECTION
	}
}

void SelectionManager::unselect() {
	// _selectionVolume.clear();
	_selections.clear();
	_selectionValid = false;
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

glm::ivec3 SelectionManager::selectionLowerCorner() const {
	if (!hasSelection()) {
		return glm::ivec3(0);
	}
	voxel::Region r = _selections[0];
	for (const Selection &region : _selections) {
		r.accumulate(region);
	}
	return r.getLowerCorner();
}

} // namespace voxedit
