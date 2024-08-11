/**
 * @file
 */

#pragma once

#include "Selection.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

class SelectionManager {
private:
	Selections _selections;
	bool _selectionValid = false;
	bool _selectStartPositionValid = false;
	glm::ivec3 _selectStartPosition{0};
	voxel::Region _maxRegion = voxel::Region::InvalidRegion;
	// voxel::SparseVolume _selectionVolume;

public:
	// TODO: SELECTION: reduce access to this as much as possible
	const Selections &selections() const;
	bool hasSelection() const;

	// TODO: SELECTION: this maybe belongs into the modifier itself... let's see how the api evolves
	void start(const voxel::Region &maxRegion, const glm::ivec3 &startPos);
	bool active() const;
	void stop();

	glm::ivec3 selectionLowerCorner() const;

	// TODO: SELECTION: the plan here is to move the selected voxels into the sparse volume to allow copy/cut/move operations
	void invert(voxel::RawVolume &volume);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	void unselect();

	// TODO: SELECTION: this is no longer working or should not be used once you can select single voxels
	voxel::Region calcSelectionRegion(const glm::ivec3 &cursorPosition) const;
};

inline bool SelectionManager::hasSelection() const {
	return _selectionValid;
}

inline bool SelectionManager::active() const {
	return _selectStartPositionValid;
}

} // namespace voxedit
