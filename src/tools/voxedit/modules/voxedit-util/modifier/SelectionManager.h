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
	// voxel::SparseVolume _selectionVolume;

public:
	// TODO: SELECTION: reduce access to this as much as possible
	const Selections &selections() const;

	template <typename F>
	void visitSelections(F &&f) const {
		for (const auto &selection : _selections) {
			f(selection);
		}
	}

	voxel::Region region() const;
	bool hasSelection() const;

	// TODO: SELECTION: the plan here is to move the selected voxels into the sparse volume to allow copy/cut/move operations
	void invert(voxel::RawVolume &volume);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	void unselect(voxel::RawVolume &volume);

	void reset();
};

inline bool SelectionManager::hasSelection() const {
	return !_selections.empty();
}

} // namespace voxedit
