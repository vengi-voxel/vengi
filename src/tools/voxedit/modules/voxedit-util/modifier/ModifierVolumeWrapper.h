/**
 * @file
 */

#pragma once

#include "ModifierType.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

/**
 * @brief A wrapper for a @c voxel::RawVolume that performs a sanity check for
 * the @c setVoxel() call and uses the @c ModifierType value to perform the
 * desired action for the @c setVoxel() call.
 * The sanity check also includes the voxel's FlagOutline that is used to limit the
 * area of the @c voxel::RawVolume that is affected by the @c setVoxel() call.
 */
class ModifierVolumeWrapper : public voxel::RawVolumeWrapper {
private:
	using Super = voxel::RawVolumeWrapper;
	const ModifierType _modifierType;
	scenegraph::SceneGraphNode &_node;

	bool _erase;
	bool _override;
	bool _paint;
	bool _normalPaint;
	bool _hasSelection;

	// if we have a selection, we only handle voxels inside the selection
	bool skip(const glm::aligned_ivec4 &pos) const {
		if (!_hasSelection) {
			return false;
		}
		// Check if the voxel has the FlagOutline (selected) flag set
		const voxel::Voxel &voxel = _volume->voxel(pos.x, pos.y, pos.z);
		return (voxel.getFlags() & voxel::FlagOutline) == 0;
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
				if (_volume->_paint || _volume->_normalPaint || _volume->_erase) {
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
			// Preserve the FlagOutline (selection) flag when modifying voxels
			const uint8_t existingFlags = _currentVoxel->getFlags();
			if (_volume->_erase) {
				if (_volume->_normalPaint) {
					_currentVoxel->setNormal(NO_NORMAL);
				} else {
					*_currentVoxel = {};
				}
			} else {
				if (_volume->_normalPaint) {
					_currentVoxel->setNormal(voxel.getNormal());
				} else {
					*_currentVoxel = voxel;
				}
			}
			// Restore the FlagOutline if it was set on the original voxel
			if (existingFlags & voxel::FlagOutline) {
				_currentVoxel->setFlags(existingFlags);
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

	ModifierVolumeWrapper(scenegraph::SceneGraphNode &node, ModifierType modifierType)
		: Super(node.volume()), _modifierType(modifierType), _node(node) {
		_erase = _modifierType == ModifierType::Erase;
		_override = _modifierType == ModifierType::Override;
		_paint = _modifierType == ModifierType::Paint;
		_normalPaint = _modifierType == ModifierType::NormalPaint;
		_hasSelection = _node.hasSelection();
	}

	inline scenegraph::SceneGraphNode &node() const {
		return _node;
	}

	inline ModifierType modifierType() const {
		return _modifierType;
	}

	/**
	 * @brief Set flags on voxels in the given region and mark it dirty
	 */
	void setFlags(const voxel::Region &region, uint8_t flags) {
		_volume->setFlags(region, flags);
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(region);
		} else {
			_dirtyRegion = region;
		}
	}

	/**
	 * @brief Remove flags from voxels in the given region and mark it dirty
	 */
	void removeFlags(const voxel::Region &region, uint8_t flags) {
		_volume->removeFlags(region, flags);
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(region);
		} else {
			_dirtyRegion = region;
		}
	}

	/**
	 * @brief Toggle flags on voxels in the given region and mark it dirty
	 */
	void toggleFlags(const voxel::Region &region, uint8_t flags) {
		_volume->toggleFlags(region, flags);
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(region);
		} else {
			_dirtyRegion = region;
		}
	}

	/**
	 * @brief Check if any voxel in the given region has the specified flags
	 */
	bool hasFlags(const voxel::Region &region, uint8_t flags) const {
		return _volume->hasFlags(region, flags);
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
