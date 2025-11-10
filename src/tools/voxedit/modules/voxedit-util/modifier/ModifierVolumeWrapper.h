/**
 * @file
 */

#pragma once

#include "ModifierType.h"
#include "Selection.h"
#include "scenegraph/SceneGraphNode.h"
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

	bool _erase;
	bool _override;
	bool _paint;

	bool skip(const glm::aligned_ivec4 &pos) const {
		if (_selections.empty()) {
			return false;
		}
		for (const Selection &sel : _selections) {
			if (sel.containsPoint(pos)) {
				return false;
			}
		}
		return true;
	}

public:
	class Sampler : public voxel::VolumeSampler<ModifierVolumeWrapper> {
	private:
		using Super = VolumeSampler<ModifierVolumeWrapper>;
	public:
		VOLUMESAMPLERUSING;

		bool setVoxel(const voxel::Voxel& voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			if (!_volume->_override) {
				const voxel::Voxel existingVoxel = this->voxel();
				const bool empty = voxel::isAir(existingVoxel.getMaterial());
				if (_volume->_paint || _volume->_erase) {
					if (empty) {
						return false;
					}
				} else if (!empty) {
					return false;
				}
			}

			if (_volume->skip(_posInVolume)) {
				return false;
			}
			if (_volume->_erase) {
				*_currentVoxel = {};
			} else {
				*_currentVoxel = voxel;
			}
			voxel::Region &dirtyRegion = _volume->_dirtyRegion;
			if (dirtyRegion.isValid()) {
				dirtyRegion.accumulate(_posInVolume);
			} else {
				dirtyRegion = voxel::Region(_posInVolume.x, _posInVolume.y, _posInVolume.z, _posInVolume.x, _posInVolume.y, _posInVolume.z);
			}
			return true;
		}
	};

	ModifierVolumeWrapper(scenegraph::SceneGraphNode &node, ModifierType modifierType, const Selections &selections = {})
		: Super(node.volume()), _selections(selections), _modifierType(modifierType), _node(node) {
		_erase = _modifierType == ModifierType::Erase;
		_override = _modifierType == ModifierType::Override;
		_paint = _modifierType == ModifierType::Paint;
	}

	inline scenegraph::SceneGraphNode &node() const {
		return _node;
	}

	inline ModifierType modifierType() const {
		return _modifierType;
	}

	bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) override {
		Sampler sampler(*this);
		if (!sampler.setPosition(x, y, z)) {
			return false;
		}
		return sampler.setVoxel(voxel);
	}
};

} // namespace voxedit
