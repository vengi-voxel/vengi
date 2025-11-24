/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "app/App.h"
#include "core/ScopedPtr.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class SceneManager;

/**
 * @brief A brush that places other volumes as stamps into the target volume.
 * @ingroup Brushes
 *
 * You first have to load a volume into the brush - otherwise you can only place one voxel at a time.
 */
class StampBrush : public Brush {
private:
	using Super = Brush;

protected:
	core::ScopedPtr<voxel::RawVolume> _volume;
	palette::Palette _palette;
	glm::ivec3 _lastCursorPosition{0};
	glm::ivec3 _offset{0};
	bool _center = true;
	bool _continuous = false;
	SceneManager *_sceneMgr;
	core::VarPtr _maxVolumeSize;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	StampBrush(SceneManager *sceneMgr) : Super(BrushType::Stamp), _sceneMgr(sceneMgr) {
	}
	virtual ~StampBrush() = default;
	void construct() override;
	void setVolume(const voxel::RawVolume &volume, const palette::Palette &palette);
	voxel::RawVolume *volume() const;
	bool active() const override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	voxel::Region calcRegion(const BrushContext &ctx) const override;

	void setCenterMode(bool center);
	bool centerMode() const;

	void setContinuousMode(bool single);
	bool continuousMode() const;

	/**
	 * @brief Either convert all voxels of the stamp to the given voxel, or create a new stamp with the given voxel
	 */
	void setVoxel(const voxel::Voxel &voxel, const palette::Palette &palette);
	void convertToPalette(const palette::Palette &palette);

	void setSize(const glm::ivec3 &size);
	void setOffset(const glm::ivec3 &offset);
	const glm::ivec3 &offset() const;

	bool load(const core::String &filename);
};

inline bool StampBrush::centerMode() const {
	return _center;
}

inline void StampBrush::setCenterMode(bool center) {
	if (center != _center) {
		markDirty();
	}
	_center = center;
}

inline bool StampBrush::continuousMode() const {
	return _continuous;
}

inline void StampBrush::setContinuousMode(bool continuous) {
	_continuous = continuous;
}

inline voxel::RawVolume *StampBrush::volume() const {
	return _volume;
}

inline bool StampBrush::active() const {
	return _volume != nullptr;
}

inline void StampBrush::setOffset(const glm::ivec3 &offset) {
	if (offset == _offset) {
		return;
	}
	_offset = offset;
	markDirty();
}

inline const glm::ivec3 &StampBrush::offset() const {
	return _offset;
}

} // namespace voxedit
