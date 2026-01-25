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

// TODO: SELECTION: see https://github.com/vengi-voxel/vengi/issues/580
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

	void invert(voxel::RawVolume &volume);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &pos);
	void selectAll(voxel::RawVolume &volume);
	/**
	 * @brief Unselect all selected voxels in the given volume
	 */
	void unselect(voxel::RawVolume &volume);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &pos);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool isSelected(const glm::ivec3 &pos) const;
	/**
	 * @brief Cut selected voxels from the given volume and return a new volume containing them
	 */
	voxel::RawVolume *cut(voxel::RawVolume &volume);
	/**
	 * @brief Copy selected voxels from the given volume and return a new volume containing them
	 */
	voxel::RawVolume *copy(const voxel::RawVolume &volume);

	/**
	 * @brief Unselect everything
	 */
	void reset();
};

inline bool SelectionManager::hasSelection() const {
	return !_selections.empty();
}

using SelectionManagerPtr = core::SharedPtr<SelectionManager>;

} // namespace voxedit
