/**
 * @file
 */

#pragma once

#include "ModifierType.h"
#include "Selection.h"
#include "core/Color.h"
#include "core/RGBA.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxedit {

/**
 * @brief A wrapper for a @c voxel::RawVolume that performs a sanity check for
 * the @c setVoxel() call and uses the @c ModifierType value to perform the
 * desired action for the @c setVoxel() call.
 * The sanity check also includes the @c Selections that are used to limit the
 * area of the @c voxel::RawVolume that is affected by the @c setVoxel() call.
 */
class ModifierVolumeWrapper : public voxel::RawVolumeWrapper {
private:
	using Super = voxel::RawVolumeWrapper;
	const Selections _selections;
	const ModifierType _modifierType;
	scenegraph::SceneGraphNode &_node;

	bool _eraseVoxels;
	bool _overwrite;
	bool _update;
	bool _force;

	bool skip(int x, int y, int z) const {
		if (!_region.containsPoint(x, y, z)) {
			return true;
		}
		if (_selections.empty()) {
			return false;
		}
		return !contains(_selections, x, y, z);
	}

public:
	ModifierVolumeWrapper(scenegraph::SceneGraphNode &node, ModifierType modifierType, const Selections &selections)
		: Super(node.volume()), _selections(selections), _modifierType(modifierType), _node(node) {
		_eraseVoxels = (_modifierType & ModifierType::Erase) == ModifierType::Erase;
		_overwrite = (_modifierType & ModifierType::Place) == ModifierType::Place && _eraseVoxels;
		_update = (_modifierType & ModifierType::Paint) == ModifierType::Paint;
		_force = _overwrite || _eraseVoxels;
	}

	inline scenegraph::SceneGraphNode &node() const {
		return _node;
	}

	inline ModifierType modifierType() const {
		return _modifierType;
	}

	void addDirtyRegion(const voxel::Region &region) {
		if (!region.isValid()) {
			return;
		}
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(region);
		} else {
			_dirtyRegion = region;
		}
	}

	bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) override {
		if (!_force) {
			const voxel::Voxel existingVoxel = this->voxel(x, y, z);
			const bool empty = voxel::isAir(existingVoxel.getMaterial());
			if (_update) {
				if (empty) {
					return false;
				}
			} else if (!empty) {
				return false;
			}
		}
		voxel::Voxel placeVoxel = voxel;
		if (!_overwrite && _eraseVoxels) {
			placeVoxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
		}
		if (skip(x, y, z)) {
			return false;
		}
		return Super::setVoxel(x, y, z, placeVoxel);
	}
};

} // namespace voxedit
