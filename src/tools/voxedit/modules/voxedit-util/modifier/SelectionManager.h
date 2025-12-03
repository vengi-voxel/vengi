/**
 * @file
 */

#pragma once

#include "Selection.h"
#include "core/DirtyState.h"
#include "core/SharedPtr.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

class SelectionManager : public core::DirtyState {
private:
	Selections _selections;
	// when moving selected voxels, don't do it in a region larger than this
	voxel::Region _maxRegion = voxel::Region::InvalidRegion;
	voxel::Region _cachedRegion = voxel::Region::InvalidRegion;

public:
	// TODO: SELECTION: reduce access to this as much as possible
	const Selections &selections() const;

	void setMaxRegionSize(const voxel::Region &maxRegion);
	const voxel::Region& region();
	bool hasSelection() const;

	// TODO: SELECTION: the plan here is to move the selected voxels into the sparse volume to allow copy/cut/move operations
	//                  see https://github.com/vengi-voxel/vengi/issues/580
	void invert(voxel::RawVolume &volume);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &pos);
	void unselect(voxel::RawVolume &volume);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &pos);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool isSelected(const glm::ivec3 &pos) const;
	voxel::RawVolume *cut(voxel::RawVolume &volume);
	voxel::RawVolume *copy(const voxel::RawVolume &volume);

	void reset();
};

inline bool SelectionManager::hasSelection() const {
	return !_selections.empty();
}

using SelectionManagerPtr = core::SharedPtr<SelectionManager>;

} // namespace voxedit
